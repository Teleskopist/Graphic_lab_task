#include "common_descriptor_makers.h"
#include "similarity_compression.h"
#include "sdf_bricks_dataset.h"
#include "branches_dataset.h"
#include "channels_dataset.h"
#include "scom_internal.h"
#include "clustering/clustering.h"

#include "utils/common/position_hash.h"
#include "utils/common/stat_box.h"

#include "utils/nn_search/common_distances.h"
#include "utils/nn_search/linear_search.h"
#include "utils/nn_search/exact_hash.h"
#include "utils/nn_search/ball_tree.h"
#include "utils/nn_search/kd_tree.h"
#include "utils/nn_search/lsh.h"

#include <chrono>

namespace scom2
{
  void print_branch_info(uint4 code, const BranchesCompressionLog::BranchInfo &info)
  {
    static const char *branch_type_names[3] = {"EMPTY", "LEAF", "CONSOLIDATED"};
    static const char * brick_type_names[4] = {"EMPTY", "INTERNAL", "VOLUME", "SURFACE"};

    printf("#############################\n");
    printf("%s branch (%d levels), %d depth\n", branch_type_names[(int)info.type], (int)info.levels.size(), info.start_depth);
    
    int max_depth = info.start_depth;
    for (auto &l : info.levels)
    {
      int node_count = l.grid_size*l.grid_size*l.grid_size;
      printf("*****************************\n");
      printf("depth = %d, %dx%dx%d nodes\n", max_depth, l.grid_size, l.grid_size, l.grid_size);
      for (int j=0;j<node_count;j++)
      {
        int n_x = j/l.grid_size/l.grid_size;
        int n_y = j/l.grid_size%l.grid_size;
        int n_z = j%l.grid_size;

        auto &node = l.multibricks[j];
        uint4 b_code = code*l.grid_size + uint4(n_x, n_y, n_z, 0);
        printf("=============================\n");
        printf("multibrick (%d %d %d) [%d %d %d %d]\n", n_x, n_y, n_z, b_code.x, b_code.y, b_code.z, b_code.w);
        printf("type: %s, %d surfaces\n", brick_type_names[(int)node.type], node.brick_count);
        for (int k=0; k<node.brick_count; k++)
        {
          printf("-----------------------------\n");
          for (int y=0;y<node.v_size;y++)
          {
            for (int x=0;x<node.v_size;x++)
            {
              if (x != 0)
                printf("| ");
              for (int z=0;z<node.v_size;z++)
                printf("%8.5f ", node.values[k*node.v_size*node.v_size*node.v_size + x*node.v_size*node.v_size + y*node.v_size + z]);
            }
            printf("\n");
          }
          printf("-----------------------------\n");
        }
        printf("=============================\n");
      }
      printf("*****************************\n");
      max_depth++;
    }
    printf("#############################\n");
  }

  void mark_accessible_elements(const SdfDAG &dag, uint32_t node_id,
                                std::vector<bool> &accessed_nodes, 
                                std::vector<bool> &accessed_child_edges, std::vector<bool> &accessed_data_edges,
                                std::vector<bool> &accessed_bricks, std::vector<bool> &accessed_channels)
  {
    uint32_t chidren_cnt = get_children_count(dag.header);
    uint32_t v_size = get_v_size(dag.header);
    uint32_t brick_values_count = get_v_count(dag.header);
    const SdfDAGNode &node = dag.nodes[node_id];

    accessed_nodes[node_id] = true;
    accessed_channels[node.channels_edge.child_offset] = true;

    if (node.children_edges_offset != 0)
    {
      for (int i = 0; i < chidren_cnt; i++)
        accessed_child_edges[node.children_edges_offset + i] = true;
    }
    if (node.data_edges_offset != 0)
    {
      uint32_t surf_count = DAG_extract_count(node.voxel_count_flags);
      for (int i = 0; i < surf_count; i++)
      {
        accessed_data_edges[node.data_edges_offset + i] = true;
        accessed_bricks[dag.data_edges[node.data_edges_offset + i].data_offset / brick_values_count] = true;
      }
    }

    if (node.children_edges_offset != 0)
    {
      for (int i = 0; i < chidren_cnt; i++)
      {
        uint32_t child_node_id = dag.child_edges[node.children_edges_offset + i].child_offset;
        if (child_node_id != 0)
          mark_accessible_elements(dag, child_node_id, accessed_nodes, accessed_child_edges, accessed_data_edges, accessed_bricks, accessed_channels);
      }
    }
  }

  // print some statistics regarding number of elements in DAG
  // node, brick, surface count etc.
  void print_DAG_info(const SdfDAG &dag)
  {
    uint32_t ch_count = get_children_count(dag.header);
    uint32_t v_size = get_v_size(dag.header);
    uint32_t brick_values_count = get_v_count(dag.header);
    uint32_t brick_count = dag.distances.size() / brick_values_count;
    uint32_t leaf_node_count = 0;
    uint32_t node_count = 0;
    uint32_t surf_counts[4] = {0};
    for (const auto &node : dag.nodes)
    {
      if (node.children_edges_offset == 0)
      {
        leaf_node_count++;
        surf_counts[std::min<int>(DAG_extract_count(node.voxel_count_flags), 3)]++;
      }
      else
        node_count++;
    }

    if (node_count > 10000)
      printf("%dk non-leaf nodes\n", node_count / 1000);
    else
      printf("%d non-leaf nodes\n", node_count);
    if (leaf_node_count > 10000)
      printf("%dk leaf nodes\n", leaf_node_count / 1000);
    else
      printf("%d leaf nodes\n", leaf_node_count);
    if (brick_count > 10000)
      printf("%dk bricks\n", brick_count / 1000);
    else
      printf("%d bricks\n", brick_count);

    //surface count statistics
    stat_utils::RangeBins<int> surf_ranges({1,2,3,4,5,6,7,8,15,63,255});
    uint32_t child_count = ch_count;
    for (const auto &node : dag.nodes)
    {
      if (node.children_edges_offset != 0)
      {
        uint32_t surf_count = DAG_extract_count(node.voxel_count_flags);
        for (int i = 0; i < child_count; i++)
        {
          uint32_t child_node_id = dag.child_edges[node.children_edges_offset + i].child_offset;
          if (child_node_id != 0)
          {
            const SdfDAGNode &child_node = dag.nodes[child_node_id];
            if (child_node.children_edges_offset == 0)
              surf_count += DAG_extract_count(child_node.voxel_count_flags);
            else
              surf_count++;
          }
        }
        surf_ranges.add(surf_count);
      }
    }

    //surf_ranges.print_bins();

    uint64_t total_channels_bytes = 0;
    for (const auto &ch : dag.point_channels)
      total_channels_bytes += ch.data_f.size()*sizeof(float) + ch.data_fp8.size()*sizeof(uint8_t) + ch.data_i.size()*sizeof(int32_t);
    for (const auto &ch : dag.voxel_channels)
      total_channels_bytes += ch.data_f.size()*sizeof(float) + ch.data_fp8.size()*sizeof(uint8_t) + ch.data_i.size()*sizeof(int32_t);
    printf("total channels bytes: %lld\n", (long long)total_channels_bytes);
  }

  // explicitly set 0 surfaces for empty nodes to skip them in compression
  // and remove later. Internal bricks can be preserved or not, depending on the settings
  void DAG_mark_empty_nodes(SdfDAG &dag, InternalBrickUse internal_brick_use)
  {
    uint32_t v_size = get_v_size(dag.header);
    uint32_t brick_values_count = get_v_count(dag.header);
    for (int i = 0; i < dag.nodes.size(); i++)
    {
      if (dag.nodes[i].data_edges_offset == 0)
        continue;

      uint32_t surf_count = DAG_extract_count(dag.nodes[i].voxel_count_flags);
      //printf("surf count %d\n", surf_count);
      BrickInfo::Type sum_type = BrickInfo::Type::EMPTY;
      for (int j = 0; j < surf_count; j++)
        sum_type = merge_types(sum_type, get_surface_type(&dag, i, dag.nodes[i].data_edges_offset+j, internal_brick_use));

      if (sum_type == BrickInfo::Type::EMPTY)
      {
        dag.nodes[i].voxel_count_flags = DAG_pack_voxel_count_flags(0, false, DAG_extract_is_inside(dag.nodes[i].voxel_count_flags)); // empty node
      }
    }
  }

  std::shared_ptr<nn_search::INNSearchAS> create_nn_search_as(scom2::Settings settings, nn_search::IDistanceFunction *dist_func,
                                                              uint32_t point_count, uint32_t point_size)
  {
    std::shared_ptr<nn_search::INNSearchAS> as;
    switch(settings.search_algorithm)
    {
      case scom2::SearchAlgorithm::LINEAR_SEARCH:
      {
        if (dist_func == nullptr || dist_func->getType() == nn_search::IDistanceFunction::Type::L2)
          as = std::make_shared<nn_search::LinearSearchL2AS>();
        else 
          as = std::make_shared<nn_search::LinearSearchAS>(dist_func);
      }
      break;
      case scom2::SearchAlgorithm::BALL_TREE:
      {
        if (dist_func == nullptr || dist_func->getType() == nn_search::IDistanceFunction::Type::L2)
        {
          //if (point_size < 8)
            as = std::make_shared<nn_search::BallTreeL2>();
          //else
          //  as = std::make_shared<nn_search::BallTreeFDL2>();
        }
        else if (dist_func->getType() == nn_search::IDistanceFunction::Type::L1)
          as = std::make_shared<nn_search::BallTreeL1>();
        else if (dist_func->getType() == nn_search::IDistanceFunction::Type::LInf)
          as = std::make_shared<nn_search::BallTreeLInf>();
        else if (dist_func->getType() == nn_search::IDistanceFunction::Type::CustomMetric)
          as = std::make_shared<nn_search::BallTree>(dist_func);
        else 
          printf("[scom2::create_nn_search_as] ERROR: BallTree requires a metric as a distance function. Type %d given\n", (int)dist_func->getType());
      }
      break;
      case scom2::SearchAlgorithm::LSH:
      {
        // TODO: find a good value based on dimension and point count
        int bits = point_size > 128 ? 20 : 16;
        int tables = bits / 2;
        as = std::make_shared<nn_search::LSHSearchAS>(tables, bits);
      }
      break;
      case scom2::SearchAlgorithm::EXACT_HASH:
      {
        as = std::make_shared<nn_search::ExactHashAS>();
      }
      break;
      case scom2::SearchAlgorithm::KD_TREE:
      {
        as = std::make_shared<nn_search::KDTree>();
      }
      break;
      default:
      {
        printf("[scom2::create_nn_search_as] unknown SearchAlgorithm %d\n", (int)settings.search_algorithm);
      }
      break;
    }

    return as;
  }

  std::shared_ptr<IDescriptorMaker> create_brick_descriptor_maker(const SdfDAG *dag, const Settings &settings)
  {
    switch (settings.embedding_type)
    {
      case scom2::EmbeddingType::DIRECT:
        return create_default_sdf_descriptor_maker(dag, settings.bricks_transform_subgroup, settings.surface_sensitivity);
      case scom2::EmbeddingType::TRIVIAL:
        return create_trivial_descriptor_maker(dag, settings.bricks_transform_subgroup);
      case scom2::EmbeddingType::NO_EMBEDDING:
        return create_empty_descriptor_maker();
      case scom2::EmbeddingType::CUSTOM:
      {
        if (settings.custom_descriptor_maker)
          return settings.custom_descriptor_maker;
        else
        {
          printf("[ERROR] settings.embedding_type == CUSTOM, but no custom descriptor maker provided\n");
          return nullptr;
        }
      }
      default:
        printf("[ERROR] Unknown embedding type %d\n", (int)settings.embedding_type);
        return nullptr;
    }
  }

  std::shared_ptr<IDescriptorMaker> create_point_channel_descriptor_maker(const SdfDAG *dag, const Settings &settings, const DataChannel &ch)
  {
    if (ch.type == DataChannel::Type::INT)
    {
      printf("ERROR: integer point channels are not supported in similarity compression\n");
      return nullptr;
    }
    else if (ch.type == DataChannel::Type::FP8)
    {
      printf("ERROR: compressed (FP8) channels are not supported in similarity compression\n");
      return nullptr;
    }
    else if (ch.type != DataChannel::Type::FLOAT)
    {
      printf("ERROR: unknown channel type %d\n", (int)ch.type);
      return nullptr;
    }

    auto custom_ch_settings = settings.custom_channel_settings.find(ch.name);
    if (custom_ch_settings != settings.custom_channel_settings.end() && custom_ch_settings->second.descriptor_maker)
    {
      if (custom_ch_settings->second.importance != 1.0f)
        printf("WARNING: channel \"%s\" has importance != 1 and custom descriptor_maker, importance will be ignored\n",ch.name.c_str());
      return custom_ch_settings->second.descriptor_maker;
    }
    else
    {
      float importance = 1.0f;
      if (custom_ch_settings != settings.custom_channel_settings.end())
      {
        // With default descriptor maker and default distance function we can propagate importance into descriptors
        // themselves, by increasing their values. It is fast, easy and most common case.
        // Otherwise, we force user to adjust their custom functions to reflect different importance of different channels
        if (custom_ch_settings->second.dist_func == nullptr ||
            custom_ch_settings->second.dist_func->getType() == nn_search::IDistanceFunction::Type::L1 ||
            custom_ch_settings->second.dist_func->getType() == nn_search::IDistanceFunction::Type::L2)
        {
          importance = custom_ch_settings->second.importance;
        }
        else if (custom_ch_settings->second.importance != 1.0f)
        {
          printf("WARNING: channel \"%s\" has importance != 1 and custom dist_func, importance will be ignored\n",ch.name.c_str());
        }
      }

      //if min_val or max_val are undefined or broken, calculate them
      float min_val = ch.min_val;
      float max_val = ch.max_val;
      if (min_val == DataChannel::LIMIT_UNDEFINED || max_val == DataChannel::LIMIT_UNDEFINED ||
          min_val >= max_val || std::isnan(min_val) || std::isnan(max_val))
      {
        min_val = ch.data_f[0];
        max_val = ch.data_f[0];
        for (const auto &val : ch.data_f)
        {
          min_val = std::min(min_val, val);
          max_val = std::max(max_val, val);
        }
        max_val += 1e-9f;
      }
      return create_normalized_descriptor_maker(dag, TransformSubgroup::DEFAULT, float2(min_val, max_val), importance);
    }
  }

  std::shared_ptr<IDescriptorMaker> create_voxel_channel_descriptor_maker(const SdfDAG *dag, const Settings &settings, const DataChannel &ch,
                                                                          bool hash_int_channels)
  {
    if (ch.type == DataChannel::Type::INT && hash_int_channels)
    {
      //it is not an error, we do not make descriptors from int channels, but hash them to divide the data into subdatasets
      return nullptr;
    }
    else if (ch.type == DataChannel::Type::FP8)
    {
      printf("ERROR: compressed (FP8) channels are not supported in similarity compression\n");
      return nullptr;
    }
    else if (ch.type != DataChannel::Type::FLOAT && ch.type != DataChannel::Type::INT)
    {
      printf("ERROR: unknown channel type %d\n", (int)ch.type);
      return nullptr;
    }
      
    auto custom_ch_settings = settings.custom_channel_settings.find(ch.name);
    if (custom_ch_settings != settings.custom_channel_settings.end() && custom_ch_settings->second.descriptor_maker)
    {
      if (custom_ch_settings->second.importance != 1.0f)
        printf("WARNING: channel \"%s\" has importance != 1 and custom descriptor_maker, importance will be ignored\n",ch.name.c_str());
      return custom_ch_settings->second.descriptor_maker;
    }
    else if (ch.type == DataChannel::Type::INT)
    {
      if (custom_ch_settings != settings.custom_channel_settings.end() && custom_ch_settings->second.importance != 1.0f)
        printf("WARNING: channel \"%s\" has importance != 1 for integer channel, importance will be ignored\n",ch.name.c_str());
      return create_integer_descriptor_maker(dag, TransformSubgroup::DEFAULT);
    }
    else
    {
      float importance = 1.0f;
      if (custom_ch_settings != settings.custom_channel_settings.end())
      {
        // With default descriptor maker and default distance function we can propagate importance into descriptors
        // themselves, by increasing their values. It is fast, easy and most common case.
        // Otherwise, we force user to adjust their custom functions to reflect different importance of different channels
        if (custom_ch_settings->second.dist_func == nullptr ||
            custom_ch_settings->second.dist_func->getType() == nn_search::IDistanceFunction::Type::L1 ||
            custom_ch_settings->second.dist_func->getType() == nn_search::IDistanceFunction::Type::L2)
        {
          importance = custom_ch_settings->second.importance;
        }
        else if (custom_ch_settings->second.importance != 1.0f)
        {
          printf("WARNING: channel \"%s\" has importance != 1 and custom dist_func, importance will be ignored\n",ch.name.c_str());
        }
      }

      //if min_val or max_val are undefined or broken, calculate them
      float min_val = ch.min_val;
      float max_val = ch.max_val;
      if (min_val == DataChannel::LIMIT_UNDEFINED || max_val == DataChannel::LIMIT_UNDEFINED ||
          min_val >= max_val || std::isnan(min_val) || std::isnan(max_val))
      {
        min_val = ch.data_f[0];
        max_val = ch.data_f[0];
        for (const auto &val : ch.data_f)
        {
          min_val = std::min(min_val, val);
          max_val = std::max(max_val, val);
        }
        max_val += 1e-9f;
      }

      return create_normalized_descriptor_maker(dag, TransformSubgroup::DEFAULT, float2(min_val, max_val), importance);
    }
  }

  static void print_sub_dataset_desc(const SubDatasetDescriptor &desc)
  {
    std::string type_name = "empty";
    switch (desc.brick_type)
    {
      case (uint32_t)BrickInfo::Type::INTERNAL: type_name = "internal"; break;
      case (uint32_t)BrickInfo::Type::SURFACE : type_name = "surface";  break;
      case (uint32_t)BrickInfo::Type::VOLUME  : type_name = "volume";   break;
    }
    if (desc.channels_count == 0)
      printf("sub-dataset (%8s, %4d, %2d, %2d, %2d, %8x) ", type_name.c_str(), 
             desc.dist_count_per_surface, desc.level, desc.surface_count, desc.int_channels_idx, desc.bin_hash);
    else
      printf("sub-dataset (%8s, %4d +%4d, %2d, %2d, %2d, %8x) ", type_name.c_str(), 
            desc.dist_count_per_surface, desc.channels_count, desc.level, desc.surface_count, desc.int_channels_idx, desc.bin_hash);
  }

  void DAG_bricks_similarity_compression(SdfDAG &dag, scom2::Settings settings)
  {
    std::unique_ptr<scom2::SComDatasetSDFBricks> dataset;
    dataset = std::make_unique<scom2::SComDatasetSDFBricks>(&dag, 
                                                            create_brick_descriptor_maker(&dag, settings),
                                                            settings.custom_descriptor_binner,
                                                            settings.internal_brick_use, 
                                                            settings.bricks_transform_subgroup, 
                                                            settings.bricks_use_average_val_transform,
                                                            settings.surface_sensitivity);

    std::shared_ptr<nn_search::IDistanceFunction> dist_func;
    if (settings.sim_metric == SimMetric::EUCLIDEAN)
      dist_func = std::make_shared<nn_search::L2Distance>();
    else if (settings.sim_metric == SimMetric::L1)
      dist_func = std::make_shared<nn_search::L1Distance>();
    else if (settings.sim_metric == SimMetric::CUSTOM && settings.custom_dist_func != nullptr)
      dist_func = settings.custom_dist_func;
    else
    {
      printf("[DAG_bricks_similarity_compression] ERROR: Unknown similarity metric %d or custom distance function not provided\n", 
            (int)settings.sim_metric);
      assert(false);
    }
    scom2::ClusteringSettings scom_settings;
    scom_settings.clustering_algorithm = settings.clustering_algorithm;
    scom_settings.search_algorithm = settings.search_algorithm;
    scom_settings.similarity_threshold = settings.bricks_similarity_threshold;
    scom_settings.target_leaf_count = settings.bricks_target_count;
    scom_settings.clustering_dataset_size_limit_GB = settings.clustering_dataset_size_limit_GB;
    scom_settings.use_close_match_heuristic = settings.use_close_match_heuristic;

    uint32_t initial_elems_total = 0;
    uint32_t clusters_total      = 0;
    for (const auto &sub_dataset_pair : dataset->sub_datasets)
    {
      initial_elems_total += sub_dataset_pair.second.size();

      // skip trivial sub-datasets
      if (sub_dataset_pair.second.size() < 2 || sub_dataset_pair.first.surface_count == 0)
      {
        clusters_total += sub_dataset_pair.second.size(); 
        continue;
      }

      print_sub_dataset_desc(sub_dataset_pair.first);
      dataset->cur_sub_desc = sub_dataset_pair.first;

      scom_settings.nn_search_as = create_nn_search_as(settings, dist_func.get(), dataset->get_point_count(), dataset->get_descriptor_size());

      scom2::CompressionOutput output;
      scom2::perform_clustering(dataset.get(), scom_settings, output);
      clusters_total += output.clusters.size();
      printf("%d -> %d\n", (int)sub_dataset_pair.second.size(), (int)output.clusters.size());

      uint32_t v_size = get_v_size(dag.header);
      uint32_t v_count = get_v_count(dag.header);
      std::vector<float> new_distances((1 + output.clusters.size()) * v_count, 1000.0f);
      auto &inverse_indices = dataset->get_transform_group()->inverse_indices;

      for (int i = 0; i < output.clusters.size(); i++)
      {
        // TODO: calculate centroid for the cluster, replace lead brick with centroid

        // link this brick to the lead brick with appropriate rotation and addition
        const scom2::Cluster &cl = output.clusters[i];
        uint32_t lead_point_id = sub_dataset_pair.second[cl.lead_id];
        uint32_t lead_node_id = dataset->data_points[lead_point_id].original_id;
        uint32_t lead_surf_id = dataset->data_points[lead_point_id].surface_id;

        assert(cl.point_ids.size() == cl.rotations.size());
        for (int j = 0; j < cl.point_ids.size(); j++)
        {
          uint32_t point_id = sub_dataset_pair.second[cl.point_ids[j]];
          uint32_t node_id = dataset->data_points[point_id].original_id;
          uint32_t surf_id = dataset->data_points[point_id].surface_id;

          uint32_t de_off = dag.nodes[node_id].data_edges_offset;
          dag.data_edges[de_off + surf_id].data_offset = dag.data_edges[dag.nodes[lead_node_id].data_edges_offset + lead_surf_id].data_offset;
          dag.data_edges[de_off + surf_id].rotation_id = inverse_indices[cl.rotations[j]];
          dag.data_edges[de_off + surf_id].add = dataset->data_points[point_id].average_val -
                                                 dataset->data_points[lead_point_id].average_val;
        }
      }
    }

    printf("compressed %d nodes to %d clusters in %d sub-datasets\n\n", initial_elems_total, clusters_total, (int)dataset->sub_datasets.size());
  }

  class CombinedL2Distance : public nn_search::IDistanceFunction
  {
  public:
    CombinedL2Distance(std::vector<std::shared_ptr<nn_search::IDistanceFunction>> _dist_funcs,
                       std::vector<uint32_t> _dist_funcs_dims)
    {
      assert(_dist_funcs.size() > 0);
      assert(_dist_funcs.size() == _dist_funcs_dims.size());
      if (_dist_funcs.size() == 1)
        printf("[WARNING] CombinedL2Distance is created with only one distance function. It is useless, but has some performance overhead\n");
      dist_funcs = _dist_funcs;
      dist_funcs_dims = _dist_funcs_dims;

      dist_funcs_offsets.resize(dist_funcs.size());
      dist_funcs_offsets[0] = 0;
      for (int i = 1; i < dist_funcs.size(); ++i)
        dist_funcs_offsets[i] = dist_funcs_offsets[i-1] + dist_funcs_dims[i-1];
      total_input_dim = dist_funcs_offsets.back() + dist_funcs_dims.back();

      bool all_L2 = true;
      bool all_metric = true;
      for (const auto &dist_func : dist_funcs)
      {
        all_L2 = all_L2 && (dist_func->getType() == Type::L2);
        all_metric = all_metric && dist_func->isMetric();
      }
      if (all_L2)
        type = Type::L2;
      else if (all_metric)
        type = Type::CustomMetric;
      else
        type = Type::CustomDistance;
    }

    virtual Type getType() override { return type; }
    virtual float calc(uint32_t dim, const float *a, const float *b) override
    {
      assert(dim >= total_input_dim);
      float d = 0;
      for (int i = 0; i < dist_funcs.size(); ++i)
      {
        float v = dist_funcs[i]->calc(dim, a + dist_funcs_offsets[i], b + dist_funcs_offsets[i]);
        d += v*v;
      }
      return sqrtf(d);
    }
    virtual float calcTruncated(uint32_t dim, const float *a, const float *b, float max_dist) override
    {
      assert(dim >= total_input_dim);
      float max_dist_sqr = max_dist*max_dist;
      float d = 0;
      for (int i = 0; i < dist_funcs.size() && d < max_dist_sqr; ++i)
      {
        float v = dist_funcs[i]->calcTruncated(dim, a + dist_funcs_offsets[i], b + dist_funcs_offsets[i], max_dist);
        d += v*v;
      }
      return sqrtf(d);
    }
  private:
    std::vector<std::shared_ptr<nn_search::IDistanceFunction>> dist_funcs;
    std::vector<uint32_t> dist_funcs_dims;
    std::vector<uint32_t> dist_funcs_offsets;
    uint32_t total_input_dim;
    Type type;
  };

  class CombinedL1Distance : public nn_search::IDistanceFunction
  {
  public:
    CombinedL1Distance(std::vector<std::shared_ptr<nn_search::IDistanceFunction>> _dist_funcs,
                       std::vector<uint32_t> _dist_funcs_dims)
    {
      assert(_dist_funcs.size() > 0);
      assert(_dist_funcs.size() == _dist_funcs_dims.size());
      if (_dist_funcs.size() == 1)
        printf("[WARNING] CombinedL1Distance is created with only one distance function. It is useless, but has some performance overhead\n");
      dist_funcs = _dist_funcs;
      dist_funcs_dims = _dist_funcs_dims;

      dist_funcs_offsets.resize(dist_funcs.size());
      dist_funcs_offsets[0] = 0;
      for (int i = 1; i < dist_funcs.size(); ++i)
        dist_funcs_offsets[i] = dist_funcs_offsets[i-1] + dist_funcs_dims[i-1];
      total_input_dim = dist_funcs_offsets.back() + dist_funcs_dims.back();

      bool all_L1 = true;
      bool all_metric = true;
      for (const auto &dist_func : dist_funcs)
      {
        all_L1 = all_L1 && (dist_func->getType() == Type::L1);
        all_metric = all_metric && dist_func->isMetric();
      }
      if (all_L1)
        type = Type::L1;
      else if (all_metric)
        type = Type::CustomMetric;
      else
        type = Type::CustomDistance;
    }

    virtual Type getType() override { return type; }
    virtual float calc(uint32_t dim, const float *a, const float *b) override
    {
      assert(dim >= total_input_dim);
      float d = 0;
      for (int i = 0; i < dist_funcs.size(); ++i)
      {
        float v = dist_funcs[i]->calc(dim, a + dist_funcs_offsets[i], b + dist_funcs_offsets[i]);
        d += v;
      }
      return d;
    }
    virtual float calcTruncated(uint32_t dim, const float *a, const float *b, float max_dist) override
    {
      assert(dim >= total_input_dim);
      float d = 0;
      for (int i = 0; i < dist_funcs.size() && d < max_dist; ++i)
      {
        float v = dist_funcs[i]->calcTruncated(dim, a + dist_funcs_offsets[i], b + dist_funcs_offsets[i], max_dist);
        d += v;
      }
      return d;
    }
  private:
    std::vector<std::shared_ptr<nn_search::IDistanceFunction>> dist_funcs;
    std::vector<uint32_t> dist_funcs_dims;
    std::vector<uint32_t> dist_funcs_offsets;
    uint32_t total_input_dim;
    Type type;
  };

  class CombinedCustomDistance : public nn_search::IDistanceFunction
  {
  public:
    static constexpr uint32_t MAX_DIST_FUNCS = 256;
    CombinedCustomDistance(std::shared_ptr<nn_search::ILossFunction> _loss_func,
                           std::vector<std::shared_ptr<nn_search::IDistanceFunction>> _dist_funcs,
                           std::vector<uint32_t> _dist_funcs_dims)
    {
      assert(_dist_funcs.size() > 0);
      assert(_dist_funcs.size() == _dist_funcs_dims.size());
      assert(_dist_funcs.size() <= MAX_DIST_FUNCS);
      if (_dist_funcs.size() == 1)
        printf("[WARNING] CombinedCustomDistance is created with only one distance function. It is useless, but has some performance overhead\n");
      dist_funcs = _dist_funcs;
      dist_funcs_dims = _dist_funcs_dims;
      loss_func = _loss_func;

      dist_funcs_offsets.resize(dist_funcs.size());
      dist_funcs_offsets[0] = 0;
      for (int i = 1; i < dist_funcs.size(); ++i)
        dist_funcs_offsets[i] = dist_funcs_offsets[i-1] + dist_funcs_dims[i-1];
      total_input_dim = dist_funcs_offsets.back() + dist_funcs_dims.back();

      if(loss_func->preserveMetric())
       type = Type::CustomMetric;
      else
        type = Type::CustomDistance;
    }

    virtual Type getType() override { return type; }
    virtual float calc(uint32_t dim, const float *a, const float *b) override
    {
      assert(dim >= total_input_dim);
      float tmp_distances[MAX_DIST_FUNCS];
      for (int i = 0; i < dist_funcs.size(); ++i)
        tmp_distances[i] = dist_funcs[i]->calc(dist_funcs_dims[i], a + dist_funcs_offsets[i], b + dist_funcs_offsets[i]);
      return loss_func->calc(dist_funcs.size(), tmp_distances);
    }
  private:
    std::shared_ptr<nn_search::ILossFunction> loss_func;
    std::vector<std::shared_ptr<nn_search::IDistanceFunction>> dist_funcs;
    std::vector<uint32_t> dist_funcs_dims;
    std::vector<uint32_t> dist_funcs_offsets;
    uint32_t total_input_dim;
    Type type;
  };

  BranchDescriptor get_point_channel_branch_descriptor(scom2::Settings &settings, const DataChannel &channel)
  {
    BranchDescriptor bd;
    if (settings.custom_channel_settings.find(channel.name) != settings.custom_channel_settings.end())
      bd = settings.custom_channel_settings[channel.name].branch_descriptor;
    else
      bd = settings.branch_descriptor_channel;
    if (bd == BranchDescriptor::DEFAULT)
      bd = BranchDescriptor::SIMPLE_MERGE; //SIMPLE_MERGE is a default for point channels

    if (bd != BranchDescriptor::SIMPLE_MERGE && bd != BranchDescriptor::CONCATENATE)
    {
      printf("[DAG_branches_similarity_compression] ERROR: branch_descriptor for point channel must be SIMPLE_MERGE or CONCATENATE\n");
      assert(false);
    }

    return bd;
  }

  BranchDescriptor get_voxel_channel_branch_descriptor(scom2::Settings &settings, const DataChannel &channel)
  {
    BranchDescriptor bd;
    if (settings.custom_channel_settings.find(channel.name) != settings.custom_channel_settings.end())
      bd = settings.custom_channel_settings[channel.name].branch_descriptor;
    else
      bd = settings.branch_descriptor_channel;
    if (bd == BranchDescriptor::DEFAULT)
      bd = BranchDescriptor::VOXEL_CONCATENATE; // VOXEL_CONCATENATE is a default for voxel channels

    if (bd != BranchDescriptor::CONCATENATE && bd != BranchDescriptor::VOXEL_CONCATENATE)
    {
      printf("[DAG_branches_similarity_compression] ERROR: branch_descriptor for voxel channel must be CONCATENATE or VOXEL_CONCATENATE\n");
      assert(false);
    }
    return bd;
  }

  std::shared_ptr<nn_search::IDistanceFunction> 
  create_combined_dist_func(SdfDAG &dag, scom2::Settings settings, bool use_bricks, bool use_channels, uint32_t branch_level)
  {
    std::vector<std::shared_ptr<nn_search::IDistanceFunction>> dist_funcs;
    std::vector<uint32_t> dist_funcs_dims;

    if (use_bricks)
    {
      std::shared_ptr<IDescriptorMaker> dm = create_brick_descriptor_maker(&dag, settings);
      if (dm == nullptr)
        assert(false);

      std::shared_ptr<nn_search::IDistanceFunction> b_dist_func;
      if (settings.sim_metric == SimMetric::EUCLIDEAN)
        b_dist_func = std::make_shared<nn_search::L2Distance>();
      else if (settings.sim_metric == SimMetric::L1)
        b_dist_func = std::make_shared<nn_search::L1Distance>();
      else if (settings.sim_metric == SimMetric::CUSTOM && settings.custom_dist_func != nullptr)
        b_dist_func = settings.custom_dist_func;
      else
      {
        printf("[DAG_bricks_similarity_compression] ERROR: Unknown similarity metric %d or custom distance function not provided\n", 
              (int)settings.sim_metric);
        assert(false);
      }
        IDescriptorMaker::DatasetInfo info;
        info.dim = dag.header.dim;
        info.node_count = 1;
        info.num_components = 1;
        info.v_size = int_pow(dag.header.node_grid_size, branch_level) + 1;
        dist_funcs_dims.push_back(dm->get_descriptor_size(info));
        dist_funcs.push_back(b_dist_func);
    }

    if (use_channels)
    {
      for (const auto &channel : dag.point_channels)
      {
        std::shared_ptr<IDescriptorMaker> dm = create_point_channel_descriptor_maker(&dag, settings, channel);
        if (dm == nullptr)
          continue;
          
        std::shared_ptr<nn_search::IDistanceFunction> dist_func;
        auto custom_ch_setting = settings.custom_channel_settings.find(channel.name);
        if (custom_ch_setting != settings.custom_channel_settings.end() && custom_ch_setting->second.dist_func)
          dist_func = custom_ch_setting->second.dist_func;
        else if (settings.sim_metric_channel == SimMetric::EUCLIDEAN)
          dist_func = std::make_shared<nn_search::L2Distance>();
        else if (settings.sim_metric_channel == SimMetric::L1)
          dist_func = std::make_shared<nn_search::L1Distance>();
        else if (settings.sim_metric_channel == SimMetric::CUSTOM)
        {
          printf("[DAG_channels_similarity_compression] ERROR: Channel %s does not have custom distance function\n", channel.name.c_str());
          assert(false);
          return nullptr;
        }
        else
          assert(false);

        uint32_t v_size = 2;
        if (branch_level > 0)
        {
          BranchDescriptor bd = get_point_channel_branch_descriptor(settings, channel);
          if (bd == BranchDescriptor::SIMPLE_MERGE)
            v_size = int_pow(dag.header.node_grid_size, branch_level) + 1;
          else if (bd == BranchDescriptor::CONCATENATE)
            v_size = 2*int_pow(dag.header.node_grid_size, branch_level);
        }

        IDescriptorMaker::DatasetInfo info;
        info.dim = dag.header.dim;
        info.node_count = 1;
        info.num_components = channel.num_components;
        info.v_size = v_size;
        dist_funcs_dims.push_back(dm->get_descriptor_size(info));
        dist_funcs.push_back(dist_func);
      }
      for (const auto &channel : dag.voxel_channels)
      {
        std::shared_ptr<IDescriptorMaker> dm = create_voxel_channel_descriptor_maker(&dag, settings, channel, branch_level == 0);
        if (dm == nullptr)
          continue;

        std::shared_ptr<nn_search::IDistanceFunction> dist_func;
        auto custom_ch_setting = settings.custom_channel_settings.find(channel.name);
        if (custom_ch_setting != settings.custom_channel_settings.end() && custom_ch_setting->second.dist_func)
          dist_func = custom_ch_setting->second.dist_func;
        else if (settings.sim_metric_channel == SimMetric::EUCLIDEAN)
          dist_func = std::make_shared<nn_search::L2Distance>();
        else if (settings.sim_metric_channel == SimMetric::L1)
          dist_func = std::make_shared<nn_search::L1Distance>();
        else if (settings.sim_metric_channel == SimMetric::CUSTOM)
        {
          printf("[DAG_channels_similarity_compression] ERROR: Channel %s does not have custom distance function\n", channel.name.c_str());
          assert(false);
          return nullptr;
        }
        else
          assert(false);

        uint32_t v_size = 1;
        uint32_t d_count = 1;
        if (branch_level > 0)
        {
          BranchDescriptor bd = get_voxel_channel_branch_descriptor(settings, channel);
          if (bd == BranchDescriptor::CONCATENATE)
          {
            v_size = 1;
            d_count = int_pow(dag.header.node_grid_size, branch_level); 
          }
          else if (bd == BranchDescriptor::VOXEL_CONCATENATE)
          {
            v_size = int_pow(dag.header.node_grid_size, branch_level);
            d_count = 1;
          }
        }

        IDescriptorMaker::DatasetInfo info;
        info.dim = dag.header.dim;
        info.node_count = 1;
        info.num_components = channel.num_components;
        info.v_size = v_size;

        for (int d = 0; d < d_count; d++)
        {
          dist_funcs_dims.push_back(dm->get_descriptor_size(info));
          dist_funcs.push_back(dist_func);
        }
      }
    }

    std::shared_ptr<nn_search::IDistanceFunction> dist_func;
    if (dist_funcs.size() == 0)
      dist_func = nullptr;
    else if (dist_funcs.size() == 1)
      dist_func = dist_funcs[0];
    else if (branch_level == 0 && settings.custom_combined_loss_func)
      dist_func = std::make_shared<CombinedCustomDistance>(settings.custom_combined_loss_func, dist_funcs, dist_funcs_dims);
    else if (branch_level > 0 && settings.custom_branch_loss_func)
      dist_func = std::make_shared<CombinedCustomDistance>(settings.custom_branch_loss_func, dist_funcs, dist_funcs_dims);
    else if (settings.combined_loss == CombinedLoss::L1)
      dist_func = std::make_shared<CombinedL1Distance>(dist_funcs, dist_funcs_dims);
    else if (settings.combined_loss == CombinedLoss::L2)
      dist_func = std::make_shared<CombinedL2Distance>(dist_funcs, dist_funcs_dims);
    else
    {
      printf("[DAG_channels_similarity_compression] ERROR: combined_loss must be L1, L2, or CUSTOM with non-null custom_combined_loss_func\n");
      assert(false);
    }

    return dist_func;
  }

  void DAG_channels_similarity_compression(SdfDAG &dag, scom2::Settings settings)
  {
    std::vector<std::shared_ptr<IDescriptorMaker>> point_channel_dms;
    std::vector<std::shared_ptr<IDescriptorMaker>> voxel_channel_dms;
    std::vector<std::shared_ptr<IDescriptorBinner>> point_channel_binners;
    std::vector<std::shared_ptr<IDescriptorBinner>> voxel_channel_binners;

    for (const auto &channel : dag.point_channels)
    {
      std::shared_ptr<IDescriptorMaker> dm = create_point_channel_descriptor_maker(&dag, settings, channel);
      point_channel_dms.push_back(dm);
    }
    for (const auto &channel : dag.voxel_channels)
    {
      std::shared_ptr<IDescriptorMaker> dm = create_voxel_channel_descriptor_maker(&dag, settings, channel, true);
      voxel_channel_dms.push_back(dm);
    }

    for (const auto &channel : dag.point_channels)
    {
      auto it = settings.custom_channel_settings.find(channel.name);
      if (it != settings.custom_channel_settings.end() && it->second.descriptor_binner != nullptr)
        point_channel_binners.push_back(it->second.descriptor_binner);
      else
        point_channel_binners.push_back(nullptr);
    }

    for (const auto &channel : dag.voxel_channels)
    {
      auto it = settings.custom_channel_settings.find(channel.name);
      if (it != settings.custom_channel_settings.end() && it->second.descriptor_binner != nullptr)
        voxel_channel_binners.push_back(it->second.descriptor_binner);
      else
        voxel_channel_binners.push_back(nullptr);
    }

    scom2::SComDatasetChannels dataset(&dag, point_channel_dms, voxel_channel_dms, 
                                       point_channel_binners, voxel_channel_binners);

    std::shared_ptr<nn_search::IDistanceFunction> dist_func = create_combined_dist_func(dag, settings, false, true, 0);

    scom2::ClusteringSettings scom_settings;
    scom_settings.clustering_algorithm = settings.clustering_algorithm;
    scom_settings.search_algorithm = settings.search_algorithm;
    scom_settings.similarity_threshold = settings.channels_similarity_threshold;
    scom_settings.target_leaf_count = settings.channels_target_count;
    scom_settings.clustering_dataset_size_limit_GB = settings.clustering_dataset_size_limit_GB;
    scom_settings.use_close_match_heuristic = settings.use_close_match_heuristic;

    uint32_t initial_elems_total = 0;
    uint32_t clusters_total      = 0;
    for (const auto &sub_dataset_pair : dataset.sub_datasets)
    {
      initial_elems_total += sub_dataset_pair.second.size();

      // skip trivial sub-datasets
      if (sub_dataset_pair.second.size() < 2)
      {
        clusters_total += sub_dataset_pair.second.size();
        continue;
      }

      print_sub_dataset_desc(sub_dataset_pair.first);
      dataset.cur_sub_desc = sub_dataset_pair.first;

      scom_settings.nn_search_as = create_nn_search_as(settings, dist_func.get(), dataset.get_point_count(), dataset.get_descriptor_size());

      scom2::CompressionOutput output;
      scom2::perform_clustering(&dataset, scom_settings, output);
      clusters_total += output.clusters.size();
      printf("%d -> %d\n", (int)sub_dataset_pair.second.size(), (int)output.clusters.size());

      auto &inverse_indices = dataset.get_transform_group()->inverse_indices;
      for (int i = 0; i < output.clusters.size(); i++)
      {
        uint32_t lead_node_id = sub_dataset_pair.second[output.clusters[i].lead_id];
        for (int j = 0; j < output.clusters[i].point_ids.size(); j++)
        {
          uint32_t id = sub_dataset_pair.second[output.clusters[i].point_ids[j]];
          dag.nodes[id].channels_edge.child_offset = dag.nodes[lead_node_id].channels_edge.child_offset;
          dag.nodes[id].channels_edge.rotation_id = inverse_indices[output.clusters[i].rotations[j]];
        }
      }
    }

    printf("compressed %d data points to %d clusters in %d sub-datasets\n\n", initial_elems_total, clusters_total, (int)dataset.sub_datasets.size());
  }

  void DAG_branches_similarity_compression(SdfDAG &dag, scom2::Settings settings)
  {
    // we use int4 dot for indices, therefore only 4D is supported, but more is impractical anyway 
    assert(dag.header.dim <= 4);

    std::vector<ChannelCustomSettings> pc_custom_settings;
    std::vector<ChannelCustomSettings> vc_custom_settings;
    for (const auto &channel : dag.point_channels)
    {
      pc_custom_settings.emplace_back();
      if (settings.custom_channel_settings.find(channel.name) != settings.custom_channel_settings.end())
        pc_custom_settings.back() = settings.custom_channel_settings[channel.name];
      pc_custom_settings.back().descriptor_maker = create_point_channel_descriptor_maker(&dag, settings, channel);
      pc_custom_settings.back().branch_descriptor = get_point_channel_branch_descriptor(settings, channel);
      if (pc_custom_settings.back().default_values.empty())
        pc_custom_settings.back().default_values = std::vector<float>(channel.num_components*int_pow(2u, dag.header.dim), 0.0f);
    }

    for (const auto &channel : dag.voxel_channels)
    {
      vc_custom_settings.emplace_back();
      if (settings.custom_channel_settings.find(channel.name) != settings.custom_channel_settings.end())
        vc_custom_settings.back() = settings.custom_channel_settings[channel.name];
      vc_custom_settings.back().descriptor_maker = create_voxel_channel_descriptor_maker(&dag, settings, channel, false);
      vc_custom_settings.back().branch_descriptor = get_voxel_channel_branch_descriptor(settings, channel);
      if (vc_custom_settings.back().default_values.empty())
        vc_custom_settings.back().default_values = std::vector<float>(channel.num_components, 0.0f);
    }

    scom2::SComDatasetBranches dataset(&dag, 
                                       create_brick_descriptor_maker(&dag, settings),
                                       settings.custom_descriptor_binner,
                                       pc_custom_settings, 
                                       vc_custom_settings,
                                       settings.embedding_type,
                                       settings.internal_brick_use, 
                                       settings.bricks_transform_subgroup,
                                       settings.branches_transform_subgroup,
                                       settings.branch_descriptor,
                                       settings.branches_merge_surface_threshold);

    // scom2::BranchesCompressionLog log;
    // dataset.bc_log = &log;

    scom2::ClusteringSettings scom_settings;
    scom_settings.clustering_algorithm = settings.clustering_algorithm;
    scom_settings.search_algorithm = settings.search_algorithm;
    scom_settings.similarity_threshold = settings.branches_similarity_threshold;
    scom_settings.target_leaf_count = settings.branches_target_count;
    scom_settings.clustering_dataset_size_limit_GB = settings.clustering_dataset_size_limit_GB;
    scom_settings.use_close_match_heuristic = settings.use_close_match_heuristic;

    for (int i = 1; i <= settings.branches_similarity_compression_levels; i++)
    {
      float mult = int_pow(settings.branches_threshold_reduction_factor*int_pow(2, dag.header.dim), i);
      scom_settings.similarity_threshold = mult * settings.branches_similarity_threshold;

      std::shared_ptr<nn_search::IDistanceFunction> dist_func = create_combined_dist_func(dag, settings, true, true, i);
      scom2::CompressionOutput output;

      auto bt1 = std::chrono::high_resolution_clock::now();
      dataset.create_next_consolidated_level();
      auto bt2 = std::chrono::high_resolution_clock::now();
      printf("create_next_consolidated_level: %.1f ms\n", std::chrono::duration<float, std::milli>(bt2 - bt1).count());

      uint32_t max_v_size = IDescriptorMaker::get_max_v_size(dag.header);
      
      if (dataset.cons_levels[i].brick_size+1 > max_v_size)
      {
        printf("[DAG_branches_similarity_compression] WARNING v_size for branch level %d is too large (%d, limit is %d). Skipping compression step\n",
               i, dataset.cons_levels[i].brick_size+1, max_v_size);
        break;
      }

      uint32_t initial_elems_total = 0;
      uint32_t clusters_total      = 0;
      for (const auto &sub_dataset_pair : dataset.cons_levels[i].sub_datasets)
      {
        auto bt3 = std::chrono::high_resolution_clock::now();

        initial_elems_total += sub_dataset_pair.second.size();

        // skip trivial sub-datasets
        if (sub_dataset_pair.second.size() < 2 || get_desc_size(sub_dataset_pair.first) == 0)
        {
          clusters_total += sub_dataset_pair.second.size();
          continue;
        }

        print_sub_dataset_desc(sub_dataset_pair.first);
        dataset.cur_sub_desc = sub_dataset_pair.first;

        //choose optimal search algorithm depending on dataset
        if (scom_settings.search_algorithm == scom2::SearchAlgorithm::BALL_TREE)
        {
          if (dataset.get_point_count() < 1000)
            scom_settings.search_algorithm = scom2::SearchAlgorithm::LINEAR_SEARCH;
          else if (dataset.get_descriptor_size() > 256)
            scom_settings.search_algorithm = scom2::SearchAlgorithm::LSH;
          else
            scom_settings.search_algorithm = scom2::SearchAlgorithm::BALL_TREE;
        }
        scom_settings.nn_search_as = create_nn_search_as(settings, dist_func.get(), dataset.get_point_count(), dataset.get_descriptor_size());

        scom2::perform_clustering(&dataset, scom_settings, output);
        clusters_total += output.clusters.size();
        printf("%d -> %d\n", (int)sub_dataset_pair.second.size(), (int)output.clusters.size());

        auto bt4 = std::chrono::high_resolution_clock::now();

        // fill remap data
        auto &inverse_indices = dataset.get_transform_group()->inverse_indices;
        std::vector<uint2> node_id_to_cluster_remap(dag.nodes.size(), uint2(INVALID_IDX, INVALID_IDX));
        for (int j = 0; j < output.clusters.size(); j++)
        {
          for (int k = 0; k < output.clusters[j].count; k++)
          {
            uint32_t pointId = output.clusters[j].point_ids[k]; // index of point in dataset (i.e. node among active nodes of this size)
            uint32_t activeNodeId = sub_dataset_pair.second[pointId];
            uint32_t nodeId = dataset.cons_levels[i].active_nodes[activeNodeId].original_id;
            node_id_to_cluster_remap[nodeId] = uint2(j, k);
          }
        }

        // change child edges after remap
        for (auto &child_edge : dag.child_edges)
        {
          uint2 cluster_remap = node_id_to_cluster_remap[child_edge.child_offset];
          if (cluster_remap.x != INVALID_IDX && cluster_remap.y != INVALID_IDX)
          {
            uint32_t leadPointId = output.clusters[cluster_remap.x].lead_id;
            uint32_t leadActiveNodeId = sub_dataset_pair.second[leadPointId];
            uint32_t leadNodeId = dataset.cons_levels[i].active_nodes[leadActiveNodeId].original_id;

            child_edge.child_offset = leadNodeId;
            child_edge.rotation_id = inverse_indices[output.clusters[cluster_remap.x].rotations[cluster_remap.y]];
          }
        }
        auto bt5 = std::chrono::high_resolution_clock::now();

        printf("similarity_compression: %.1f ms\n", std::chrono::duration<float, std::milli>(bt4 - bt3).count());
        printf("remap: %.1f ms\n\n", std::chrono::duration<float, std::milli>(bt5 - bt4).count());
      }
      printf("compressed %d level %d branches to %d clusters in %d sub-datasets\n\n", 
             initial_elems_total, i, clusters_total, (int)dataset.cons_levels[i].sub_datasets.size());
    }

    uint32_t node_by_cons_level[16] = {0};
    uint32_t nodes_unknown = 0;
    uint32_t nodes_empty = 0;
    uint32_t nodes_higher_level = 0;
    for (auto &l : dataset.nodes_cons_level)
    {
      if (l < 16)
        node_by_cons_level[l]++;
      else if (l == scom2::SComDatasetBranches::LEVEL_UNKNOWN)
        nodes_unknown++;
      else if (l == scom2::SComDatasetBranches::NO_LEVEL)
        nodes_empty++;
      else if (l == scom2::SComDatasetBranches::LEVEL_UNCONSOLIDATED)
        nodes_higher_level++;
    }
    printf("nodes_empty      : %9d\n", nodes_empty);
    printf("leaf nodes       : %9d\n", node_by_cons_level[0]);
    printf("node_by_cons_level: %8d %7d %7d %7d %7d\n", node_by_cons_level[1], node_by_cons_level[2],
           node_by_cons_level[3], node_by_cons_level[4], node_by_cons_level[5]);
    printf("nodes_higher_level: %8d\n", nodes_higher_level);
    printf("nodes_unknown:      %8d\n", nodes_unknown);
  }

  void DAG_remove_links_to_empty_nodes(SdfDAG &dag)
  {
    std::vector<bool> empty_nodes(dag.nodes.size(), false);
    for (int i = 0; i < dag.nodes.size(); i++)
    {
      if (DAG_extract_count(dag.nodes[i].voxel_count_flags) == 0 && dag.nodes[i].children_edges_offset == 0)
        empty_nodes[i] = true;
    }
    for (int i = 0; i < dag.child_edges.size(); i++)
    {
      if (empty_nodes[dag.child_edges[i].child_offset])
        dag.child_edges[i].child_offset = 0;
    }
  }

  // after all branches similarity compression and remaps, a lot of hanging edges and bricks may appear
  // remove them here, effectively rebuilding DAG
  void DAG_remove_unconnected_nodes(SdfDAG &dag)
  {
    uint32_t v_size = get_v_size(dag.header);
    uint32_t brick_values_count = get_v_count(dag.header);

    std::vector<bool> accessed_nodes(dag.nodes.size(), false);
    std::vector<bool> accessed_child_edges(dag.child_edges.size(), false);
    std::vector<bool> accessed_data_edges(dag.data_edges.size(), false);
    std::vector<bool> accessed_bricks(dag.distances.size() / brick_values_count, false);
    std::vector<bool> accessed_channels(dag.nodes.size(), false);

    accessed_nodes[0] = true;
    accessed_child_edges[0] = true;
    accessed_data_edges[0] = true;

    mark_accessible_elements(dag, 0, accessed_nodes, accessed_child_edges, accessed_data_edges,
                             accessed_bricks, accessed_channels);

    // printf("accessed %d/%ld nodes\n", (int)std::count(accessed_nodes.begin(), accessed_nodes.end(), true), dag.nodes.size());
    // printf("accessed %d/%ld child edges\n", (int)std::count(accessed_child_edges.begin(), accessed_child_edges.end(), true), dag.child_edges.size());
    // printf("accessed %d/%ld data edges\n", (int)std::count(accessed_data_edges.begin(), accessed_data_edges.end(), true), dag.data_edges.size());
    // printf("accessed %d/%ld bricks\n", (int)std::count(accessed_bricks.begin(), accessed_bricks.end(), true), dag.distances.size() / brick_values_count);
    // printf("accessed %d/%ld channels\n", (int)std::count(accessed_channels.begin(), accessed_channels.end(), true), dag.nodes.size());

    // remove unused distances
    {
      std::vector<uint32_t> brick_remap(accessed_bricks.size(), INVALID_IDX);
      uint32_t brick_count = 0;

      for (int i = 0; i < accessed_bricks.size(); i++)
      {
        if (accessed_bricks[i])
        {
          brick_remap[i] = brick_count;
          brick_count++;
        }
      }

      std::vector<float> new_bricks(brick_count * brick_values_count, 0.0f);

      for (int i = 0; i < accessed_bricks.size(); i++)
      {
        if (accessed_bricks[i])
        {
          for (int j = 0; j < brick_values_count; j++)
          {
            assert(brick_remap[i] != INVALID_IDX);
            new_bricks[brick_remap[i] * brick_values_count + j] = dag.distances[i * brick_values_count + j];
          }
        }
      }

      for (int i = 0; i < dag.data_edges.size(); i++)
      {
        uint32_t idx = dag.data_edges[i].data_offset / brick_values_count;
        if (brick_remap[idx] == INVALID_IDX)
          dag.data_edges[i].data_offset = 0;
        else
          dag.data_edges[i].data_offset = brick_remap[idx] * brick_values_count;
      }

      dag.distances = new_bricks;
    }

    // remove unused data edges
    {
      std::vector<uint32_t> data_edge_remap(accessed_data_edges.size(), INVALID_IDX);
      uint32_t data_edge_count = 0;

      for (int i = 0; i < accessed_data_edges.size(); i++)
      {
        if (accessed_data_edges[i])
        {
          data_edge_remap[i] = data_edge_count;
          data_edge_count++;
        }
      }

      std::vector<SdfDAGDataEdge> new_data_edges(data_edge_count);

      for (int i = 0; i < accessed_data_edges.size(); i++)
      {
        if (accessed_data_edges[i])
        {
          assert(data_edge_remap[i] != INVALID_IDX);
          new_data_edges[data_edge_remap[i]] = dag.data_edges[i];
        }
      }

      for (int i = 0; i < dag.nodes.size(); i++)
      {
        if (dag.nodes[i].data_edges_offset != 0)
        {
          if (data_edge_remap[dag.nodes[i].data_edges_offset] == INVALID_IDX)
            dag.nodes[i].data_edges_offset = 0;
          else
            dag.nodes[i].data_edges_offset = data_edge_remap[dag.nodes[i].data_edges_offset];
        }
      }

      dag.data_edges = new_data_edges;
    }

    // remove unused child edges
    {
      std::vector<uint32_t> child_edge_remap(accessed_child_edges.size(), INVALID_IDX);
      uint32_t child_edge_count = 0;

      for (int i = 0; i < accessed_child_edges.size(); i++)
      {
        if (accessed_child_edges[i])
        {
          child_edge_remap[i] = child_edge_count;
          child_edge_count++;
        }
      }

      std::vector<SdfDAGChildEdge> new_child_edges(child_edge_count);

      for (int i = 0; i < accessed_child_edges.size(); i++)
      {
        if (accessed_child_edges[i])
        {
          assert(child_edge_remap[i] != INVALID_IDX);
          new_child_edges[child_edge_remap[i]] = dag.child_edges[i];
        }
      }

      for (int i = 0; i < dag.nodes.size(); i++)
      {
        if (dag.nodes[i].children_edges_offset != 0)
        {
          if (dag.nodes[i].children_edges_offset == INVALID_IDX)
            dag.nodes[i].children_edges_offset = 0;
          else
            dag.nodes[i].children_edges_offset = child_edge_remap[dag.nodes[i].children_edges_offset];
        }
      }

      dag.child_edges = new_child_edges;
    }

    // remove unused nodes
    {
      std::vector<uint32_t> node_remap(accessed_nodes.size(), INVALID_IDX);
      uint32_t node_count = 0;

      for (int i = 0; i < accessed_nodes.size(); i++)
      {
        if (accessed_nodes[i])
        {
          node_remap[i] = node_count;
          node_count++;
        }
      }

      std::vector<SdfDAGNode> new_nodes(node_count);

      for (int i = 0; i < accessed_nodes.size(); i++)
      {
        if (accessed_nodes[i])
        {
          assert(node_remap[i] != INVALID_IDX);
          new_nodes[node_remap[i]] = dag.nodes[i];
        }
      }

      for (int i = 0; i < dag.child_edges.size(); i++)
      {
        assert(node_remap[dag.child_edges[i].child_offset] != INVALID_IDX);
        dag.child_edges[i].child_offset = node_remap[dag.child_edges[i].child_offset];
      }

      dag.nodes = new_nodes;
    }

    {
      std::vector<uint32_t> channels_remap(accessed_channels.size(), INVALID_IDX);
      std::vector<uint32_t> channels_remap_inv(accessed_channels.size(), INVALID_IDX);
      uint32_t channels_count = 0;

      for (int i = 0; i < accessed_channels.size(); i++)
      {
        if (accessed_channels[i])
        {
          channels_remap[i] = channels_count;
          channels_remap_inv[channels_count] = i;
          channels_count++;
        }
      }

      std::vector<DataChannel> new_point_channels(dag.point_channels.size());
      std::vector<DataChannel> new_voxel_channels(dag.voxel_channels.size());

      uint32_t data_points_per_node = int_pow(2u, dag.header.dim);
      for (int i = 0; i < dag.point_channels.size(); i++)
      {
        new_point_channels[i].max_val = dag.point_channels[i].max_val;
        new_point_channels[i].min_val = dag.point_channels[i].min_val;
        new_point_channels[i].name = dag.point_channels[i].name;
        new_point_channels[i].num_components = dag.point_channels[i].num_components;
        new_point_channels[i].type = dag.point_channels[i].type;

        for (int j = 0; j < channels_count; j++)
          assert(channels_remap_inv[j] != INVALID_IDX);

        uint32_t nc = data_points_per_node * new_point_channels[i].num_components;
        if (new_point_channels[i].type == DataChannel::Type::FLOAT)
        {
          new_point_channels[i].data_f.resize(nc * channels_count);
          for (int j = 0; j < channels_count; j++)
            for (int k = 0; k < nc; k++)
              new_point_channels[i].data_f[nc * j + k] = dag.point_channels[i].data_f[nc * channels_remap_inv[j] + k];
        }
        else if (new_point_channels[i].type == DataChannel::Type::INT)
        {
          new_point_channels[i].data_i.resize(nc * channels_count);
          for (int j = 0; j < channels_count; j++)
            for (int k = 0; k < nc; k++)
              new_point_channels[i].data_i[nc * j + k] = dag.point_channels[i].data_i[nc * channels_remap_inv[j] + k];
        }
        else if (new_point_channels[i].type == DataChannel::Type::FP8)
        {
          new_point_channels[i].data_fp8.resize(nc * channels_count);
          for (int j = 0; j < channels_count; j++)
            for (int k = 0; k < nc; k++)
              new_point_channels[i].data_fp8[nc * j + k] = dag.point_channels[i].data_fp8[nc * channels_remap_inv[j] + k];
        }
      }

      for (int i = 0; i < dag.voxel_channels.size(); i++)
      {
        new_voxel_channels[i].max_val = dag.voxel_channels[i].max_val;
        new_voxel_channels[i].min_val = dag.voxel_channels[i].min_val;
        new_voxel_channels[i].name = dag.voxel_channels[i].name;
        new_voxel_channels[i].num_components = dag.voxel_channels[i].num_components;
        new_voxel_channels[i].type = dag.voxel_channels[i].type;

        for (int j = 0; j < channels_count; j++)
          assert(channels_remap_inv[j] != INVALID_IDX);

        uint32_t nc = new_voxel_channels[i].num_components;
        if (new_voxel_channels[i].type == DataChannel::Type::FLOAT)
        {
          new_voxel_channels[i].data_f.resize(nc * channels_count);
          for (int j = 0; j < channels_count; j++)
            for (int k = 0; k < nc; k++)
              new_voxel_channels[i].data_f[nc * j + k] = dag.voxel_channels[i].data_f[nc * channels_remap_inv[j] + k];
        }
        else if (new_voxel_channels[i].type == DataChannel::Type::INT)
        {
          new_voxel_channels[i].data_i.resize(nc * channels_count);
          for (int j = 0; j < channels_count; j++)
            for (int k = 0; k < nc; k++)
              new_voxel_channels[i].data_i[nc * j + k] = dag.voxel_channels[i].data_i[nc * channels_remap_inv[j] + k];
        }
        else if (new_voxel_channels[i].type == DataChannel::Type::FP8)
        {
          new_voxel_channels[i].data_fp8.resize(nc * channels_count);
          for (int j = 0; j < channels_count; j++)
            for (int k = 0; k < nc; k++)
              new_voxel_channels[i].data_fp8[nc * j + k] = dag.voxel_channels[i].data_fp8[nc * channels_remap_inv[j] + k];
        }
      }

      for (int i = 0; i < dag.nodes.size(); i++)
      {
        if (dag.nodes[i].channels_edge.child_offset != 0)
        {
          assert(channels_remap[dag.nodes[i].channels_edge.child_offset] != INVALID_IDX);
          dag.nodes[i].channels_edge.child_offset = channels_remap[dag.nodes[i].channels_edge.child_offset];
        }
      }

      dag.voxel_channels = new_voxel_channels;
      dag.point_channels = new_point_channels;
    }
  }

  void DAG_similarity_compression(SdfDAG &dag, scom2::Settings settings)
  {
    dag.header.transform_subgroup = (uint32_t)settings.bricks_transform_subgroup;

    auto t1 = std::chrono::high_resolution_clock::now();

    DAG_mark_empty_nodes(dag, settings.internal_brick_use);

    auto t2 = std::chrono::high_resolution_clock::now();

    if (settings.bricks_use_similarity_compression)
    {
      DAG_bricks_similarity_compression(dag, settings);
    }

    auto t3 = std::chrono::high_resolution_clock::now();

    if (settings.channels_use_similarity_compression && (dag.point_channels.size() + dag.voxel_channels.size() > 0))
    {
      DAG_channels_similarity_compression(dag, settings);
    }

    auto t4 = std::chrono::high_resolution_clock::now();

    if (settings.branches_use_similarity_compression)
    {
      DAG_branches_similarity_compression(dag, settings);
    }

    auto t5 = std::chrono::high_resolution_clock::now();

    DAG_remove_links_to_empty_nodes(dag);

    auto t6 = std::chrono::high_resolution_clock::now();

    DAG_remove_unconnected_nodes(dag);

    auto t7 = std::chrono::high_resolution_clock::now();

    float time1 = std::chrono::duration<float, std::milli>(t2 - t1).count();
    float time2 = std::chrono::duration<float, std::milli>(t3 - t2).count();
    float time3 = std::chrono::duration<float, std::milli>(t4 - t3).count();
    float time4 = std::chrono::duration<float, std::milli>(t5 - t4).count();
    float time5 = std::chrono::duration<float, std::milli>(t6 - t5).count();
    float time6 = std::chrono::duration<float, std::milli>(t7 - t6).count();

    print_DAG_info(dag);

    printf("    prepare nodes: %.1f ms\n", time1);
    printf("    nodes SCom: %.1f ms\n", time2);
    printf("    channels SCom: %.1f ms\n", time3);
    printf("    branches SCom: %.1f ms\n", time4);
    printf("    remove links: %.1f ms\n", time5);
    printf("    rebuild DAG: %.1f ms\n", time6);
  }
}