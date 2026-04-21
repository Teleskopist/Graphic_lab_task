#include "sdf/build/scene_converter.h"
#include "sdf/build/sparse_octree_builder.h"
#include "utils/common/position_hash.h"
#include "sdf/common/global_octree.h"
#include "CrossRT/CrossRT.h"

#include <algorithm>
#include <chrono>
#include <atomic>

void load_from_blk(scene_converter::Settings &settings, const Block *block)
{
  settings = scene_converter::Settings();

  settings.compound_levels = block->get_int("compound_levels", settings.compound_levels);
  settings.target_compound_size = block->get_int("target_compound_size", settings.target_compound_size);
  settings.levels_size_ratio = block->get_double("levels_size_ratio", settings.levels_size_ratio);
  settings.max_detail_increase = block->get_int("max_detail_increase", settings.max_detail_increase);
  settings.is_vox_merge = block->get_bool("is_vox_merge", settings.is_vox_merge);
  settings.save_debug_info = block->get_bool("save_debug_info", settings.save_debug_info);
  settings.union_depth_parts = block->get_int("union_depth_parts", settings.union_depth_parts);
  settings.union_threshold = block->get_double("union_threshold", settings.union_threshold);
  load_from_blk(settings.octree_settings, block->get_block("octree_settings"));
}

void save_to_blk(const scene_converter::Settings &settings, Block *block)
{
  block->clear();

  block->add_int("compound_levels", settings.compound_levels);
  block->add_int("target_compound_size", settings.target_compound_size);
  block->add_int("max_detail_increase", settings.max_detail_increase);
  block->add_double("levels_size_ratio", settings.levels_size_ratio);
  block->add_bool("is_vox_merge", settings.is_vox_merge);
  block->add_bool("save_debug_info", settings.save_debug_info);
  block->add_int("union_depth_parts", settings.union_depth_parts);
  block->add_double("union_threshold", settings.union_threshold);

  Block ch_b;
  save_to_blk(settings.octree_settings, &ch_b);
  block->add_block("octree_settings", &ch_b);
}

namespace scene_converter
{
  struct CADPart
  {
    enum class Type
    {
      MESH,
      ELEMENTARY_CONE,
      ELEMENTARY_CYLINDER,
      ELEMENTARY_SPHERE,
      ELEMENTARY_TORUS,
      ELEMENTARY_PLANE,
    };

    Type type;
    CRT_AABB bbox;
    union 
    {
      const cmesh4::SimpleMesh *mesh;
    } ;
  };

  struct PartInfo
  {
    const cmesh4::SimpleMesh *mesh;
    CRT_AABB bbox;
    float level_metric;
    int level;
  };

  struct Group
  {
    std::vector<int> part_ids;
    CRT_AABB bbox;
    int prim_count = 0;
  };

  struct ClusterOctreeBuildStat
  {
    int parts = 0;
    int triangles = 0;
    int rbeziers = 0;
    sdf_converter::GlobalOctreeBuildStat octree_stat_sum;
    
    //all time in ms
    float time_find_codes = 0;
    float time_transform_meshes = 0;
    float time_merge_octrees = 0;
    float time_to_multi_octree = 0;
  };

  void print_cluster_octree_build_stat(const ClusterOctreeBuildStat &stat)
  {
    printf("parts:          %d\n", stat.parts);
    printf("triangles:      %d\n", stat.triangles);
    printf("rbeziers:       %d\n", stat.rbeziers);
    sdf_converter::print_octree_build_stat(stat.octree_stat_sum);
    printf("find_codes:             %.1f\n", stat.time_find_codes);
    printf("transform_meshes:       %.1f\n", stat.time_transform_meshes);
    printf("merge_octrees:          %.1f\n", stat.time_merge_octrees);
    printf("to_multi_octree:        %.1f\n", stat.time_to_multi_octree);
  }

  CRT_AABB union_box(const CRT_AABB &a, const CRT_AABB &b) 
  { 
    return CRT_AABB{min(a.boxMin, b.boxMin), max(a.boxMax, b.boxMax)}; 
  }

  CRT_AABB intersect_box(const CRT_AABB &a, const CRT_AABB &b) 
  { 
    return CRT_AABB{max(a.boxMin, b.boxMin), min(a.boxMax, b.boxMax)}; 
  }

  bool intersect(const CRT_AABB &a, const CRT_AABB &b) 
  { 
    return a.boxMin.x <= b.boxMax.x && a.boxMax.x >= b.boxMin.x &&
           a.boxMin.y <= b.boxMax.y && a.boxMax.y >= b.boxMin.y &&
           a.boxMin.z <= b.boxMax.z && a.boxMax.z >= b.boxMin.z; 
  }

  bool contains(const CRT_AABB &a, const CRT_AABB &b) 
  { 
    return a.boxMin.x <= b.boxMin.x && a.boxMax.x >= b.boxMax.x &&
           a.boxMin.y <= b.boxMin.y && a.boxMax.y >= b.boxMax.y &&
           a.boxMin.z <= b.boxMin.z && a.boxMax.z >= b.boxMax.z; 
  }

  uint4 octree_start_code(const CRT_AABB &a)
  {
    CRT_AABB node;
    node.boxMin = float4(-1, -1, -1, 0);
    node.boxMax = float4( 1,  1,  1, 0);
    float4 size = float4( 1,  1,  1, 0);
    float4 code = float4(0, 0, 0, 1);
  
    bool node_contains = contains(node, a);
    if (!node_contains)
    {
      printf("Initial node doesn't contain a\n");
      return uint4(code);
    }

    while (node_contains)
    {
      node_contains = false;

      for (int i = 0; i < 8; i++)
      {
        float4 child_code = 2.0f*code + float4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0);
        CRT_AABB child_node;
        child_node.boxMin = float4(-1, -1, -1, 0) + 2.0f*float4(child_code.x/child_code.w, child_code.y/child_code.w, child_code.z/child_code.w, 0);
        child_node.boxMax = child_node.boxMin + 2.0f*float4(1.0f/child_code.w, 1.0f/child_code.w, 1.0f/child_code.w, 0);

        if (contains(child_node, a))
        {
          node = child_node;
          code = child_code;
          node_contains = true;
          break;
        }
      }
    }
    //printf("code %f %f %f %f\n", code.x, code.y, code.z, code.w);
    return uint4(code);
  }

  //transforms octree node with code to octree node with code (0,0,0,1)
  float4x4 get_code_transform(uint4 code)
  {
    float4 code_f = float4(code);
    float4x4 transform;
    float4x4 shift = LiteMath::translate4x4(-2.0f*float3(code_f.x/code_f.w, code_f.y/code_f.w, code_f.z/code_f.w));
    float4x4 scale = LiteMath::scale4x4(float3(code_f.w));
    return LiteMath::translate4x4(float3(-1,-1,-1)) * scale * LiteMath::translate4x4(float3(1,1,1)) * shift;
  }

  // converts one part if multithreading == should_be_multithreaded for it
  void convert_part(bool multithreading,
                    const CADPart &part, const float size_threshold_mult, float part_sq, SparseOctreeSettings &settings, 
                    scene_converter::Settings &scene_settings, std::vector<sdf_converter::GlobalOctree> &part_octrees, 
                    std::vector<LiteMath::uint4> &part_codes, scene_converter::ClusterOctreeBuildStat &stat)
  {
    std::chrono::steady_clock::time_point t1, t2, t3, t4, t5;
    sdf_converter::GlobalOctreeBuildStat go_stat;

    float3 bbox_size = to_float3(part.bbox.boxMax - part.bbox.boxMin);
    float bbox_sq = std::max(bbox_size.x * bbox_size.y, std::max(bbox_size.x * bbox_size.z, bbox_size.y * bbox_size.z));

    // this part is so small, we should not bother to use it
    if (bbox_sq < size_threshold_mult * part_sq)
    {
      return;
    };

    t1 = std::chrono::steady_clock::now();
    uint4 code = octree_start_code(part.bbox);
    float4x4 transform = get_code_transform(code);
    int level_decrease = round(log2(code.w));
    t2 = std::chrono::steady_clock::now();

    // no reason to build 0-level octree, so skip this part
    if (level_decrease >= settings.depth)
    {
      return;
    };

    sdf_converter::GlobalOctree part_octree;
    SparseOctreeSettings part_settings = settings;
    // small parts are converted to SDF with higher detalization, limited by max_detail_increase
    part_settings.depth = min(min(settings.depth + scene_settings.max_detail_increase, sdf_converter::GlobalOctree::MAX_OCTREE_DEPTH) - level_decrease,
                              settings.depth - (level_decrease + 1) / 2); // WTF
    part_settings.min_depth = std::max<int>(1, settings.min_depth - level_decrease);

    // if we have really large parts, we should use multithreading when building an octree from them
    bool should_be_multithreaded = part.mesh->TrianglesNum() > 100000 || level_decrease <= 2 || part_settings.depth >= 9;
    if (multithreading != should_be_multithreaded)
      return;

    if (part.type == CADPart::Type::MESH)
    {
      t3 = std::chrono::steady_clock::now();
      cmesh4::SimpleMesh t_mesh = *part.mesh;
      cmesh4::transform_mesh(t_mesh, transform);

      t4 = std::chrono::steady_clock::now();
      auto plo = sdf_converter::create_triangle_list_octree(t_mesh, sdf_converter::PLOSettings(part_settings.depth), should_be_multithreaded);

      t5 = std::chrono::steady_clock::now();
      if (scene_settings.save_debug_info)
        part_octree.debug_info = new sdf_converter::GlobalOctreeDebugInfo();
      sdf_converter::mesh_octree_to_global_octree(part_settings, t_mesh, plo, part_octree, &go_stat, should_be_multithreaded);
    }

    for (float &v : part_octree.values_f)
      v *= 1.0f / float(code.w); // LOL

#pragma omp critical
    {
      // if we built a valid octree
      if (part_octree.nodes.size() > 0)
      {
        part_octrees.push_back(part_octree);
        part_codes.push_back(code);
      }
      else if (part_octree.debug_info)
        delete part_octree.debug_info;

      stat.time_find_codes += std::chrono::duration<float, std::milli>(t2 - t1).count();
      stat.time_transform_meshes += std::chrono::duration<float, std::milli>(t4 - t3).count();
      go_stat.time_build_PLO = std::chrono::duration<float, std::milli>(t5 - t4).count();
      stat.octree_stat_sum.time_build_PLO += go_stat.time_build_PLO;
      stat.octree_stat_sum.time_build_flooded_octree += go_stat.time_build_flooded_octree;
      stat.octree_stat_sum.time_sign_analysis += go_stat.time_sign_analysis;
      stat.octree_stat_sum.time_calculate_distances += go_stat.time_calculate_distances;
      stat.octree_stat_sum.time_decide_to_divide += go_stat.time_decide_to_divide;
      stat.octree_stat_sum.time_mark_empty_nodes += go_stat.time_mark_empty_nodes;
      stat.octree_stat_sum.time_eliminate_empty_nodes += go_stat.time_eliminate_empty_nodes;
      stat.octree_stat_sum.time_pruning += go_stat.time_pruning;

      stat.triangles += part.mesh->TrianglesNum();
      stat.octree_stat_sum.active_nodes += part_octree.nodes.size();
    }
  }

  void convert_compound_to_sdf(const std::vector<CADPart> &parts, Settings scene_settings, CompoundScene &out_scene)
  {
    const float size_threshold_mult = 1.0f;

    std::vector<uint4> part_codes;
    std::vector<sdf_converter::GlobalOctree> part_octrees;
    ClusterOctreeBuildStat stat;
    SparseOctreeSettings settings = scene_settings.octree_settings;
    float minimal_part_size = 2.0f / pow(2, settings.depth);
    float part_sq = minimal_part_size * minimal_part_size;

    stat.parts = parts.size();
    stat.octree_stat_sum.settings = settings;

    std::chrono::steady_clock::time_point t_build_1 = std::chrono::steady_clock::now();

    // first convert all small parts that do not require multithreading (parallelism by parts)
    #pragma omp parallel for schedule(dynamic)
    for (int part_id = 0; part_id < parts.size(); part_id++)
      convert_part(false, parts[part_id], size_threshold_mult, part_sq, settings, scene_settings, part_octrees, part_codes, stat);

    // then convert a few large parts that require multithreading (parallelism inside each part)
    for (int part_id = 0; part_id < parts.size(); part_id++)
      convert_part(true, parts[part_id], size_threshold_mult, part_sq, settings, scene_settings, part_octrees, part_codes, stat);

    std::chrono::steady_clock::time_point t_build_2 = std::chrono::steady_clock::now();
    float t_build_total = std::chrono::duration<float, std::milli>(t_build_2 - t_build_1).count();
    float total_in_cycle = stat.time_find_codes + stat.time_transform_meshes + stat.octree_stat_sum.time_build_PLO +
                           stat.octree_stat_sum.time_build_flooded_octree + stat.octree_stat_sum.time_sign_analysis +
                           stat.octree_stat_sum.time_calculate_distances + stat.octree_stat_sum.time_decide_to_divide +
                           stat.octree_stat_sum.time_mark_empty_nodes + stat.octree_stat_sum.time_eliminate_empty_nodes +
                           stat.octree_stat_sum.time_pruning;
    float mult = t_build_total / total_in_cycle;
    stat.time_find_codes *= mult;
    stat.time_transform_meshes *= mult;
    stat.octree_stat_sum.time_build_PLO *= mult;
    stat.octree_stat_sum.time_build_flooded_octree *= mult;
    stat.octree_stat_sum.time_sign_analysis *= mult;
    stat.octree_stat_sum.time_calculate_distances *= mult;
    stat.octree_stat_sum.time_decide_to_divide *= mult;
    stat.octree_stat_sum.time_mark_empty_nodes *= mult;
    stat.octree_stat_sum.time_eliminate_empty_nodes *= mult;
    stat.octree_stat_sum.time_pruning *= mult;

    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
    out_scene.octrees.emplace_back();
    if (scene_settings.save_debug_info)
    {
      out_scene.debug_infos.emplace_back();
      out_scene.octrees.back().debug_info = &(out_scene.debug_infos.back());
    }
    sdf_converter::merge_global_octrees(out_scene.octrees.back(), part_octrees, part_codes, scene_settings.is_vox_merge);
    for (int i = 0; i < part_octrees.size(); i++)
    {
      if (part_octrees[i].debug_info)
        delete part_octrees[i].debug_info;
    }

    std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point t3 = std::chrono::steady_clock::now();
    stat.time_merge_octrees = std::chrono::duration<float, std::milli>(t2 - t1).count();
    stat.time_to_multi_octree = std::chrono::duration<float, std::milli>(t3 - t2).count();
    print_cluster_octree_build_stat(stat);
  }

  float volume(const CRT_AABB &a) 
  { 
    float raw_volume = (a.boxMax.x - a.boxMin.x) * (a.boxMax.y - a.boxMin.y) * (a.boxMax.z - a.boxMin.z);
    return std::max(0.0f, raw_volume); 
  }

  float get_level_metric(const Settings &settings, const cmesh4::SimpleMesh &mesh)
  {
    switch (settings.level_decision_function)
    {
    case LevelDecisionFunction::AVERAGE_POLYGON_SIZE:
      {
        double sum = 0;
        for (int i = 0; i < mesh.TrianglesNum(); i++)
        {
          float3 a = to_float3(mesh.vPos4f[mesh.indices[i * 3 + 0]]);
          float3 b = to_float3(mesh.vPos4f[mesh.indices[i * 3 + 1]]);
          float3 c = to_float3(mesh.vPos4f[mesh.indices[i * 3 + 2]]);
          sum += 0.5f*length(cross(b - a, c - a));
        }
        if (mesh.TrianglesNum() == 0) return 0;
        return sum / mesh.TrianglesNum();
      }
      break;
    case LevelDecisionFunction::MAX_POLYGON_SIZE:
      {
        float max = 0;
        for (int i = 0; i < mesh.TrianglesNum(); i++)
        {
          float3 a = to_float3(mesh.vPos4f[mesh.indices[i * 3 + 0]]);
          float3 b = to_float3(mesh.vPos4f[mesh.indices[i * 3 + 1]]);
          float3 c = to_float3(mesh.vPos4f[mesh.indices[i * 3 + 2]]);
          float area = 0.5f*length(cross(b - a, c - a));
          max = std::max<float>(max, area);
        }
        return max;
      }
    
    default:
      printf("Unknown level decision function\n");
      assert(false);
      return 0;
      break;
    }
  }
  std::vector<Group> clusterize_parts_greedy_try(const std::vector<PartInfo> &parts, int target_prim_count, int target_group_num)
  {
    assert(target_group_num > 0 && target_group_num <= parts.size());
    int free_parts = parts.size();
    std::vector<int> group_ids(parts.size(), -1);
    std::vector<Group> groups(target_group_num);
    
    //random initialization of groups
    for (int i = 0; i < target_group_num; i++)
    {
      while (groups[i].part_ids.empty())
      {
        int rnd = rand() % parts.size();
        if (group_ids[rnd] == -1)
        {
          group_ids[rnd] = i;
          groups[i].part_ids = {rnd};
          groups[i].bbox = parts[rnd].bbox;
          groups[i].prim_count = parts[rnd].mesh->TrianglesNum();
          free_parts--;
        }
      }
    }

    bool force_distribution = false;
    while (free_parts > 0)
    {
      //find free part
      int cur_part = 0;
      while (group_ids[cur_part] != -1)
        cur_part++;
    
      //find best groups for it - one that will have the least increase in volume
      int best_group = -1;
      float best_increase = MAXFLOAT;

      int lucky_group = rand() % target_group_num;
      for (int i = 0; i < target_group_num; i++)
      {
        //we allow slightly bigger groups
        if (groups[i].prim_count + 1.5f*parts[cur_part].mesh->TrianglesNum() > target_prim_count &&
            !force_distribution)
          continue;

        float v_prev = volume(groups[i].bbox);
        auto union_bbox = union_box(groups[i].bbox, parts[cur_part].bbox);
        float v_next = volume(union_bbox);
        float increase = (v_next - v_prev) / std::max(1e-6f, volume(parts[cur_part].bbox));
        if (increase < best_increase)
        {
          best_group = i;
          best_increase = increase;
        }
      }
      if (best_group == -1)
      {
        force_distribution = true;
        //printf("free_parts = %d\n", free_parts);
        continue;
      }
      else
        force_distribution = false;

      assert(best_group != -1);
      group_ids[cur_part] = best_group;
      groups[best_group].part_ids.push_back(cur_part);
      groups[best_group].bbox = union_box(groups[best_group].bbox, parts[cur_part].bbox);
      groups[best_group].prim_count += parts[cur_part].mesh->TrianglesNum();
      free_parts--;
    }

    return groups;
  }

  std::vector<Group> clusterize_parts_greedy(const std::vector<PartInfo> &parts, int target_prim_count, int target_group_num)
  {
    int tries = 100;
    float best_res = -1;
    std::vector<Group> best_groups;
    for (int i = 0; i < tries; i++)
    {
      std::vector<Group> groups = clusterize_parts_greedy_try(parts, target_prim_count, target_group_num);
      float res = 0;
      for (int j = 0; j < groups.size(); j++)
        res += volume(groups[j].bbox);
      //printf("try %d, res = %f best res = %f\n", i, res, best_res);
      if (best_groups.empty() || res < best_res)
      {
        best_res = res;
        best_groups = groups;
      }
    }
    return best_groups;
  }

  void union_small_parts(std::vector<PartInfo> &parts, std::vector<cmesh4::SimpleMesh> &mesh_parts, const Settings &settings)
  {
    float thr = settings.union_threshold;
    uint32_t max_merge_depth = settings.union_depth_parts;
    CodeMap<std::vector<int>> small_parts_by_code;
    CodeMap<int> big_parts_by_code;
    std::vector<uint32_t> deleting;
    for (int i = parts.size() - 1; i >= 0; --i)
    {
      auto code = octree_start_code(parts[i].bbox);
      while (code.w > pow(2, max_merge_depth))
      {
        code /= 2;
      }
      float3 bbox_size = to_float3(parts[i].bbox.boxMax - parts[i].bbox.boxMin);
      float bbox_sq = std::max(bbox_size.x * bbox_size.y, std::max(bbox_size.x * bbox_size.z, bbox_size.y * bbox_size.z));
      if (bbox_sq < thr)
      {
        deleting.push_back(i);
        if (small_parts_by_code.find(code) != small_parts_by_code.end())
        {
          small_parts_by_code[code].push_back(i);
        }
        else
        {
          small_parts_by_code[code] = {i};
        }
      }
      else
      {
        if (big_parts_by_code.find(code) == big_parts_by_code.end())
        {
          big_parts_by_code[code] = i;
        }
        else
        {
          float3 bbox_size_2 = to_float3(parts[big_parts_by_code[code]].bbox.boxMax - parts[big_parts_by_code[code]].bbox.boxMin);
          float bbox_sq_2 = std::max(bbox_size_2.x * bbox_size_2.y, std::max(bbox_size_2.x * bbox_size_2.z, bbox_size_2.y * bbox_size_2.z));
          if (bbox_sq < bbox_sq_2)
          {
            big_parts_by_code[code] = i;
          }
        }
      }
    }
    mesh_parts.reserve(small_parts_by_code.size());
    std::vector <PartInfo> not_inside;
    for (auto &i : small_parts_by_code)
    {
      std::vector<cmesh4::SimpleMesh> merge_parts;
      PartInfo part = parts[i.second[0]];
      for (auto &j : i.second)
      {
        part.bbox = union_box(part.bbox, parts[j].bbox);
        merge_parts.push_back(*parts[j].mesh);
      }
      auto code = i.first;
      while (big_parts_by_code.find(code) == big_parts_by_code.end() && code.w != 1)
      {
        code /= 2;
      }
      if (big_parts_by_code.find(code) != big_parts_by_code.end())
      {
        part.bbox = union_box(part.bbox, parts[big_parts_by_code[code]].bbox);
        merge_parts.push_back(*parts[big_parts_by_code[code]].mesh);
      }
      mesh_parts.push_back(cmesh4::MergeMeshes(merge_parts));
      part.mesh = &mesh_parts.back();
      part.level_metric = get_level_metric(settings, *part.mesh);
      if (big_parts_by_code.find(code) != big_parts_by_code.end())
      {
        parts[big_parts_by_code[code]] = part;
      }
      else
      {
        not_inside.push_back(part);
      }
    }
    uint32_t prev = parts.size();
    for (auto j : deleting)
    {
      parts.erase(parts.begin() + j);
    }
    printf("parts left %zu / %u\n", parts.size(), prev);
    printf("not_merged: %zu\n", not_inside.size());
    parts.insert(parts.end(), not_inside.begin(), not_inside.end());
  }

  void convert_CAD_parts_array(const std::vector<cmesh4::SimpleMesh> &mesh_parts, Settings settings, CompoundScene &out_scene)
  {
    assert(mesh_parts.size() > 0);
    assert(settings.compound_levels > 0 && settings.compound_levels < 4);
    assert(settings.target_compound_size > 1);
    assert(settings.levels_size_ratio >= 1);

    std::vector<PartInfo> parts(mesh_parts.size());
    int total_triangle_count = 0;

    for (int i = 0; i < mesh_parts.size(); i++)
    {
      parts[i].mesh = &mesh_parts[i];
      parts[i].level = -1;
      parts[i].level_metric = get_level_metric(settings, mesh_parts[i]);

      float3 bbox_min, bbox_max;
      cmesh4::get_bbox(*parts[i].mesh, &bbox_min, &bbox_max);
      parts[i].bbox.boxMax = to_float4(bbox_max, 1.0f);
      parts[i].bbox.boxMin = to_float4(bbox_min, 1.0f);

      total_triangle_count += parts[i].mesh->TrianglesNum();
    }

    std::vector<cmesh4::SimpleMesh> additional_mesh_parts;

    union_small_parts(parts, additional_mesh_parts, settings);

    int N_groups = std::max<int>(1, ceil(total_triangle_count / settings.target_compound_size));
    if (N_groups > mesh_parts.size()/3)
    {
      printf("[WARNING] Too many groups, setting N_groups = %d\n", (int)(mesh_parts.size()/3));
      N_groups = mesh_parts.size()/3;
    }

    std::vector<int> level_mults(settings.compound_levels, 0);
    level_mults[0] = 1;
    for (int i = 1; i < settings.compound_levels; i++)
      level_mults[i] = level_mults[i-1]*settings.levels_size_ratio;
    float level_denom = 0;
    for (int i = 0; i < settings.compound_levels; i++)
      level_denom += level_mults[i];

    std::vector<int> N_groups_per_level(settings.compound_levels, 0);
    for (int i = 0; i < settings.compound_levels; i++)
      N_groups_per_level[i] = std::max<int>(1, (level_mults[i]/level_denom) * N_groups);
    N_groups = 0;
    for (int i = 0; i < settings.compound_levels; i++)
      N_groups += N_groups_per_level[i];

    float expected_size = (float)total_triangle_count / N_groups;

    printf("Total triangle count = %d, N_groups = %d, expected_size = %d\n", total_triangle_count, N_groups, (int)expected_size);
    printf("Groups per level: ");
    for (int i = 0; i < settings.compound_levels; i++)
      printf(" %d", N_groups_per_level[i]);
    printf("\n");

    std::sort(parts.begin(), parts.end(), [](PartInfo &a, PartInfo &b) { return a.level_metric > b.level_metric; });

    std::vector<std::vector<int>> part_ids_by_level(settings.compound_levels);

    int cur_level = 0;
    float acc_size = 0;
    float acc_target = expected_size*N_groups_per_level[0];
    for (int i = 0; i < parts.size(); i++)
    {
      parts[i].level = cur_level;
      part_ids_by_level[cur_level].push_back(i);
      acc_size += parts[i].mesh->TrianglesNum();
      if (acc_size > acc_target && part_ids_by_level[cur_level].size() >= N_groups_per_level[cur_level])
      {
        cur_level++;
        acc_target += expected_size*N_groups_per_level[cur_level];
      }
    }

    float cur_tri_count = 0;
    for (int i = 0; i < parts.size(); i++)
    {
      cur_tri_count += parts[i].mesh->TrianglesNum();
      //printf("[%d] %.4f %d triangles, %d vertices\n", parts[i].level, parts[i].level_metric, 
      //       (int)parts[i].mesh->VerticesNum(), (int)parts[i].mesh->IndicesNum());
    }

    std::vector<std::vector<Group>> groups_by_level(settings.compound_levels);
    for (int level_idx = 0; level_idx < settings.compound_levels; level_idx++)
    {
      auto &part_ids = part_ids_by_level[level_idx];
      auto &groups = groups_by_level[level_idx];

      if (N_groups_per_level[level_idx] == 1) //one group for all
      {
        Group g;
        g.part_ids = part_ids;
        
        g.bbox = parts[part_ids[0]].bbox;
        for (int i = 1; i < part_ids.size(); i++)
          g.bbox = union_box(g.bbox, parts[part_ids[i]].bbox);
        
        g.prim_count = 0;
        for (int i = 0; i < part_ids.size(); i++)
          g.prim_count += parts[part_ids[i]].mesh->TrianglesNum();
        
        groups = {g};
      }
      else if (N_groups_per_level[level_idx] == part_ids.size()) //every part in its own group
      {
        for (int i = 0; i < part_ids.size(); i++)
        {
          Group g;
          g.part_ids = {part_ids[i]};
          g.bbox = parts[part_ids[i]].bbox;
          g.prim_count = parts[part_ids[i]].mesh->TrianglesNum();
          groups.push_back(g);
        }
      }
      else //clustering here
      {
        std::vector<PartInfo> level_parts;
        for (int i = 0; i < part_ids.size(); i++)
          level_parts.push_back(parts[part_ids[i]]);
        groups = clusterize_parts_greedy(level_parts, expected_size, N_groups_per_level[level_idx]);
        for (auto &g : groups)
        {
          for (int i = 0; i < g.part_ids.size(); i++)
            g.part_ids[i] = part_ids[g.part_ids[i]];
        }
      }
    }

    std::vector<Group> all_groups;
    for (int i = 0; i < settings.compound_levels; i++)
    {
      auto &groups = groups_by_level[i];
      for (int j = 0; j < groups.size(); j++)
        all_groups.push_back(groups[j]);
      
      printf("Level %d: %d groups\n", i, (int)groups.size());
      for (int j = 0; j < groups.size(); j++)
      {
        printf("  Group %d: %d parts, %d triangles\n", j, (int)groups[j].part_ids.size(), groups[j].prim_count);
        //for (int k = 0; k < groups[j].part_ids.size(); k++)
        //  printf("    part %d -- %d triangles\n", groups[j].part_ids[k], parts[groups[j].part_ids[k]].mesh->TrianglesNum());
      }
    }

    for (const Group &g : all_groups)
    {
      std::vector<CADPart> CAD_parts(g.part_ids.size());
      for (int i = 0; i < g.part_ids.size(); i++)
      {
        CAD_parts[i].type = CADPart::Type::MESH;
        CAD_parts[i].mesh = parts[g.part_ids[i]].mesh;
        CAD_parts[i].bbox = parts[g.part_ids[i]].bbox;
      }

      std::vector<cmesh4::SimpleMesh> part_meshes;
      for (int i = 0; i < g.part_ids.size(); i++)
        part_meshes.push_back(*parts[g.part_ids[i]].mesh);
      out_scene.compounds.push_back(cmesh4::MergeMeshes(part_meshes));

      std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
      convert_compound_to_sdf(CAD_parts, settings, out_scene);
      std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
      printf("Create octree time: %.1f ms\n", (float)std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count());
    }
  }
}