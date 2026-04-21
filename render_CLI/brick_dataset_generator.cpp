#include "brick_dataset_generator.h"
#include "sdf/simple/sdf_converter.h"
#include "sdf/build/sparse_octree_builder.h"
#include "blk/blk.h"
#include "utils/nn_search/near_neighbor_common.h"
#include "utils/nn_search/ball_tree.h"

#include <fstream>
#include <atomic>

float pearson_correlation(const float *a, const float *b, uint32_t n)
{
  double mean_a = 0.0f, mean_b = 0.0f;
  for (uint32_t i = 0; i < n; i++)
  {
    mean_a += a[i];
    mean_b += b[i];
  }

  mean_a /= n;
  mean_b /= n;

  double cov = 0.0f;
  double var_a = 0.0f, var_b = 0.0f;
  for (uint32_t i = 0; i < n; i++)
  {
    cov   += (a[i] - mean_a) * (b[i] - mean_b);
    var_a += (a[i] - mean_a) * (a[i] - mean_a);
    var_b += (b[i] - mean_b) * (b[i] - mean_b);
  }

  return cov / (sqrt(var_a) * sqrt(var_b));
}

void preprocess_brick(const float *in, float *out, uint32_t v_size)
{
  float average_val = 0.0f;

  for (uint32_t i = 0; i < v_size*v_size*v_size; i++)
    average_val += in[i];
  average_val /= v_size*v_size*v_size;

  float sum_sqr = 0.0f;

  for (uint32_t i = 0; i < v_size*v_size*v_size; i++)
  {
    float v = in[i] - average_val;
    out[i] = v;
    sum_sqr += v*v;
  }

  float i_sum = 1.0f / sqrtf(sum_sqr);
  for (uint32_t i = 0; i < v_size*v_size*v_size; i++)
    out[i] *= i_sum;
}

float trilinear_interp(const float *values, float3 dp, uint32_t off, uint32_t v_size)
{
  return (1-dp.x)*(1-dp.y)*(1-dp.z)*values[off + 0] + 
         (1-dp.x)*(1-dp.y)*(  dp.z)*values[off + 1] + 
         (1-dp.x)*(  dp.y)*(1-dp.z)*values[off + v_size] + 
         (1-dp.x)*(  dp.y)*(  dp.z)*values[off + v_size + 1] + 
         (  dp.x)*(1-dp.y)*(1-dp.z)*values[off + v_size*v_size] + 
         (  dp.x)*(1-dp.y)*(  dp.z)*values[off + v_size*v_size + 1] + 
         (  dp.x)*(  dp.y)*(1-dp.z)*values[off + v_size*v_size + v_size] + 
         (  dp.x)*(  dp.y)*(  dp.z)*values[off + v_size*v_size + v_size + 1];
}

float brick_distance_default(const float *a, const float *b, uint32_t v_size)
{
  float err = 0.0f;
  for (uint32_t i = 0; i < v_size*v_size*v_size; i++)
    err += (a[i] - b[i]) * (a[i] - b[i]);
  return sqrtf(err);
}

float brick_distance_IoU(const float *a, const float *b, uint32_t v_size, uint32_t subdiv)
{
  float step = 1.0 / (subdiv + 1);
  uint32_t cnt_same = 0;
  uint32_t cnt_all = v_size*v_size*v_size * subdiv*subdiv*subdiv;

  for (uint32_t bi = 0; bi < v_size; bi++)
  {
    for (uint32_t bj = 0; bj < v_size; bj++)
    {
      for (uint32_t bk = 0; bk < v_size; bk++)
      {
        uint32_t off = bi * v_size * v_size + bj * v_size + bk;
        for (uint32_t si = 0; si < subdiv; si++)
        {
          for (uint32_t sj = 0; sj < subdiv; sj++)
          {
            for (uint32_t sk = 0; sk < subdiv; sk++)
            {
              float v1 = trilinear_interp(a, step*float3(1 + si, 1 + sj, 1 + sk), off, v_size);
              float v2 = trilinear_interp(b, step*float3(1 + si, 1 + sj, 1 + sk), off, v_size);
              cnt_same += ((v1 > 0.0f) == (v2 > 0.0f));
            }
          }
        }
      }
    }
  }

  return 1.0f - (float)cnt_same / cnt_all;
}

void generate_brick_dataset(const char *mesh_path, unsigned pairs_count)
{
  std::vector<std::string> mesh_paths = {mesh_path};
  Block blk;
  blk.add_int("pairs_per_mesh", pairs_count);
  blk.add_arr("mesh_paths", mesh_paths);
  generate_brick_dataset(blk);
}

void generate_brick_dataset(const Block &blk)
{
  // settings (can be put in blk if needed)
  const uint32_t depth = blk.get_int("octree_depth", 7);
  const uint32_t brick_size = blk.get_int("brick_size", 3);
  const uint32_t IoU_subdiv_precise = blk.get_int("IoU_subdiv_precise", 12);
  const uint32_t IoU_subdiv_rough = blk.get_int("IoU_subdiv_rough", 4);
  uint32_t pairs_count = blk.get_int("pairs_per_mesh", 1'000'000);
  const float distance_limit = blk.get_double("distance_limit", 1.0f);
  const std::string save_bricks_path = blk.get_string("save_bricks_path", "saves/bricks.csv");
  const std::string save_pairs_path = blk.get_string("save_pairs_path", "saves/pairs.csv");
  std::vector<std::string> mesh_paths; 
  blk.get_arr("mesh_paths", mesh_paths);

  if (mesh_paths.empty())
  {
    printf("Error: no mesh paths provided\n");
    return;
  }

  const uint32_t v_size = brick_size + 1;
  std::vector<uint2> pairs(pairs_count);
  std::vector<float> default_distances(pairs_count);
  std::vector<float> iou_distances(pairs_count);
  std::vector<float> iou_distances_rough(pairs_count);

  nn_search::Dataset dataset;
  dataset.dim = v_size*v_size*v_size;

  uint32_t m_id = 0;
  for (const std::string &mesh_path : mesh_paths)
  {
    uint32_t dataset_start = dataset.data_points.size();
    auto mesh = cmesh4::LoadMesh(mesh_path.c_str());
    m_id++;
    printf("Mesh %d/%d: \"%s\", %d triangles\n", m_id, (int)mesh_paths.size(), mesh_path.c_str(), (int)mesh.TrianglesNum());

    SparseOctreeSettings settings;
    settings.depth = depth;
    settings.brick_size = brick_size;

    sdf_converter::GlobalOctree g;
    auto plo = sdf_converter::create_triangle_list_octree(mesh, sdf_converter::PLOSettings(depth));
    sdf_converter::mesh_octree_to_global_octree(settings, mesh, plo, g);

    for (auto &node : g.nodes)
    {
      if (node.bricks_count == 0)
        continue;
      
      for (int i = 0; i < node.bricks_count; ++i)
      {
        uint32_t v_off = node.val_off + i*v_size*v_size*v_size;

        float min_val =  1000;
        float max_val = -1000;
        for (uint32_t j = 0; j < v_size*v_size*v_size; ++j)
        {
          float v = g.values_f[v_off + j];
          min_val = std::min(min_val, v);
          max_val = std::max(max_val, v);
        }

        if (min_val * max_val < 0)
        {
          uint32_t off = dataset.all_points.size();
          nn_search::DataPoint dp;
          dp.average_val = 0.0f;
          dp.data_offset = off;
          dp.original_id = i;
          dp.transform_id = 0;
          dataset.all_points.resize(dataset.all_points.size() + v_size*v_size*v_size);
          dataset.data_points.push_back(dp);
          preprocess_brick(&g.values_f[v_off], dataset.all_points.data() + off, v_size);
        }
      }
    }

    uint32_t dataset_end = dataset.data_points.size();
    printf("Octree is ready, %d active bricks\n", dataset_end - dataset_start);

    // std::atomic<uint32_t> done(0);
    // #pragma omp parallel for
    // for (uint32_t i = 0; i < pairs_count; i++)
    // {
    //   uint32_t idx1 = rand() % (dataset_end - dataset_start) + dataset_start;
    //   uint32_t idx2 = rand() % (dataset_end - dataset_start) + dataset_start;

    //   float *b1 = dataset.all_points.data() + dataset.data_points[idx1].data_offset;
    //   float *b2 = dataset.all_points.data() + dataset.data_points[idx2].data_offset;

    //   float d1 = brick_distance_default(b1, b2, v_size);
    //   //float d2 = brick_distance_IoU(b1, b2, v_size, IoU_subdiv_precise);
    //   float d3 = brick_distance_IoU(b1, b2, v_size, IoU_subdiv_rough);
    //   pairs[i] = uint2(idx1, idx2);
    //   default_distances[i] = LiteMath::clamp(d1, 0.0f, 1.0f);
    //   //iou_distances[i] = LiteMath::clamp(d2, 0.0f, 1.0f);
    //   iou_distances_rough[i] = LiteMath::clamp(d3, 0.0f, 1.0f);

    //   done++;
    //   if (done.load() % 10000 == 0)
    //     printf("Processing: %d/%d pairs\n", done.load(), pairs_count);
    // }
  }

      std::unique_ptr<nn_search::BallTreeL2> NN_search_AS(new nn_search::BallTreeL2());
      NN_search_AS->build(dataset, 32);

      const uint32_t dataset_size = dataset.data_points.size();
      const uint32_t max_threads = omp_get_max_threads();
      const uint32_t max_tries = 10 * dataset_size;
      std::atomic<uint32_t> tries(0);
      std::atomic<uint32_t> found(0);

      #pragma omp parallel for
      for (int thread_id = 0; thread_id < max_threads; thread_id++)
      {
        while (found < pairs_count)
        {
          uint32_t t_idx = tries.fetch_add(1);
          if (t_idx > max_tries)
            continue;
          uint32_t idx1 = rand() % dataset_size;
          size_t off_a = dataset.data_points[idx1].data_offset;

          NN_search_AS->scan_near(dataset.all_points.data() + dataset.data_points[idx1].data_offset, distance_limit,
                                  [&](float dist, uint32_t idx2, const nn_search::DataPoint &B, const float *) -> bool {

          if (idx2 == idx1)
            return false;

          float *b1 = dataset.all_points.data() + dataset.data_points[idx1].data_offset;
          float *b2 = dataset.all_points.data() + dataset.data_points[idx2].data_offset;

          float d1 = brick_distance_default(b1, b2, v_size);
          //float d2 = brick_distance_IoU(b1, b2, v_size, IoU_subdiv_precise);
          float d3 = brick_distance_IoU(b1, b2, v_size, IoU_subdiv_rough);

          if (d1 < 1e-6f && d3 < 1e-6f)
            return false;

          uint32_t p_idx = found.fetch_add(1);
          if (p_idx >= pairs_count)
            return false;

          pairs[p_idx] = uint2(idx1, idx2);
          default_distances[p_idx] = LiteMath::clamp(d1, 0.0f, 1.0f);
          //iou_distances[p_idx] = LiteMath::clamp(d2, 0.0f, 1.0f);
          iou_distances_rough[p_idx] = LiteMath::clamp(d3, 0.0f, 1.0f);

          if (p_idx % 10000 == 0)
            printf("Processing: %d/%d pairs\n", p_idx, pairs_count);

          return false;
          });
        }
      }

      if (pairs_count > found.load())
      {
        printf("only %d/%d pairs found\n", pairs_count, found.load());
        pairs_count = found.load();
      }

  std::vector<float> ranges = {0.001, 0.01, 0.02, 0.05, 0.1, 0.25, 1000};
  std::vector<uint32_t> def_counts(ranges.size(), 0);
  std::vector<uint32_t> iou_counts(ranges.size(), 0);

  for (uint32_t i = 0; i < pairs_count; i++)
  {
    for (uint32_t j = 0; j < ranges.size(); j++)
    {
      if (default_distances[i] < ranges[j])
      {
        def_counts[j]++;
        break;
      }
    }
  }

  for (uint32_t i = 0; i < pairs_count; i++)
  {
    for (uint32_t j = 0; j < ranges.size(); j++)
    {
      if (iou_distances_rough[i] < ranges[j])
      {
        iou_counts[j]++;
        break;
      }
    }
  }

  printf("%d active bricks\n", dataset_size);
  printf("total %d pairs\n", pairs_count);
  printf("default ranges: %d %d %d %d %d %d %d\n", def_counts[0], def_counts[1], def_counts[2], def_counts[3], def_counts[4], def_counts[5], def_counts[6]);
  printf("IoU ranges: %d %d %d %d %d %d %d\n", iou_counts[0], iou_counts[1], iou_counts[2], iou_counts[3], iou_counts[4], iou_counts[5], iou_counts[6]);
  printf("correlation IoU (rough) - default: %f\n", pearson_correlation(default_distances.data(), iou_distances_rough.data(), pairs_count));
  //printf("correlation IoU (rough) - IoU (precise): %f\n", pearson_correlation(iou_distances.data(), iou_distances_rough.data(), pairs_count));

  {
    std::string bricks_csv;
    uint32_t b_cnt = v_size*v_size*v_size;
    for (uint32_t i = 0; i < b_cnt; i++)
    {
      bricks_csv += "val_" + std::to_string(i);
      bricks_csv += (i < b_cnt - 1) ? "," : "\n";
    }
    for (uint32_t i = 0; i < dataset_size; i++)
    {
      for (uint32_t j = 0; j < b_cnt; j++)
      {
        bricks_csv += std::to_string(dataset.all_points[dataset.data_points[i].data_offset + j]);
        bricks_csv += (j < b_cnt - 1) ? "," : "\n";
      }
    }

    std::ofstream ofile;
    ofile.open(save_bricks_path.c_str());
    ofile << bricks_csv;
    ofile.close();
  }

  {
    std::string pairs_csv = "x,y,dist_default,dist_IoU_rough\n";
    for (uint32_t i = 0; i < pairs_count; i++)
    {
      pairs_csv += std::to_string(pairs[i].x);
      pairs_csv += ",";
      pairs_csv += std::to_string(pairs[i].y);
      pairs_csv += ",";
      pairs_csv += std::to_string(default_distances[i]);
      pairs_csv += ",";
      pairs_csv += std::to_string(iou_distances_rough[i]);
      pairs_csv += "\n";
    }

    std::ofstream ofile;
    ofile.open(save_pairs_path.c_str());
    ofile << pairs_csv;
    ofile.close();
  }
}