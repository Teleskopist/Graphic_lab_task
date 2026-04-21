#include "clustering.h"
#include "LiteMath/LiteMath.h"
#include "utils/common/stat_box.h"
#include "utils/common/stat_utils.h"

#include <omp.h>
#include <cassert>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <set>

namespace scom2
{
  using LiteMath::int2;
  using LiteMath::int3;
  using LiteMath::int4;
  using LiteMath::float2;
  using LiteMath::float3;
  using LiteMath::float4;
  using nn_search::DataPoint;
  using nn_search::Dataset;

  uint32_t ISComDataset::get_dim_with_alignment(uint32_t dim)
  {
    // Align to 16 floats (64 bytes) for AVX512, 8 floats (32 bytes) for AVX2
    constexpr uint32_t alignment_avx512 = 16;
    constexpr uint32_t alignment_avx2 = 8;

    // Check for AVX512 support
    bool has_avx512 = false;
#if defined(__AVX512F__)
    has_avx512 = true;
#endif
    bool has_avx2 = false;
#if defined(__AVX2__)
    has_avx2 = true;
#endif

    uint32_t alg = 1;
    if (has_avx512 && dim > 8)
      alg = alignment_avx512;
    else if (has_avx2 && dim > 4)
      alg = alignment_avx2;
    return ((dim + alg - 1) / alg) * alg;
  }

  static bool is_leaf(unsigned offset)
  {
    return (offset == 0) || (offset & INVALID_IDX);
  }

  struct Link
  {
    int target = -1; // original node id (i.e. from global octree)
    int rotation = -1;
    int data_offset = -1;
    float dist = 1e6f;
  };

  static constexpr int HC_VALID = 0x7fffffff;
  static constexpr int MAX_STEPS = HC_VALID - 1;
  static constexpr float MAX_DISTANCE = 1.0f;
  static constexpr int LABEL_UNVISITED = -1;
  static constexpr int LABEL_ISOLATED = -2;

  struct HCluster
  {
    HCluster() = default;
    HCluster(int _U, int _V, int _invalidation_step, int _size) : U(_U), V(_V), invalidation_step(_invalidation_step), size(_size) {}
    int U = -1;
    int V = -1;
    int invalidation_step = HC_VALID;
    int size = 0;
  };

  struct Dist
  {
    Dist() = default;
    Dist(int _U, int _V, float _dist) : U(_U), V(_V), dist(_dist) {}
    int U = -1;
    int V = -1;
    float dist = 1000.0f;
  };

  struct CCluster
  {
    int offset = -1;
    int size = 0;
    float creation_min_dist = MAX_DISTANCE;
    int creation_step = -1;
    int invalidation_step = HC_VALID;
    int cluster_id = -1;
  };

  void create_dataset(const ISComDataset *in_dataset, uint32_t rot_start, uint32_t rot_end, Dataset &dataset)
  {
    assert(rot_start >= 0);
    assert(rot_start < rot_end);

    uint32_t rot_count = rot_end - rot_start;
    uint32_t real_dim = in_dataset->get_descriptor_size();
    dataset.dim = ISComDataset::get_dim_with_alignment(real_dim);

    dataset.data_points.resize(rot_count*in_dataset->get_point_count());
    dataset.all_points.resize(rot_count*in_dataset->get_point_count()*dataset.dim, 0.0f);

    uint32_t desc_count = in_dataset->get_point_count() * rot_count;
    std::vector<uint32_t> p_ids(desc_count);
    std::vector<uint16_t> rot_ids(desc_count);

    for (int p=0; p<in_dataset->get_point_count(); p++)
    {
      for (int r=0; r<rot_count; r++)
      {
        int i = p*rot_count + r;
        dataset.data_points[i].data_offset = i*dataset.dim;
        dataset.data_points[i].original_id = p;
        dataset.data_points[i].transform_id = rot_start + r;
        dataset.data_points[i].average_val = 0; //unused

        p_ids[i] = p;
        rot_ids[i] = rot_start + r;
      }
    }

    in_dataset->calculate_descriptors(desc_count, p_ids.data(), rot_ids.data(), dataset.all_points.data(), dataset.dim);
  }

  void clustering_recursive_fill(std::vector<Cluster> &final_clusters, const std::vector<Cluster> &components,
                                 const std::vector<std::vector<Link>> &sim_graph, int total_node_count)
  {
    final_clusters.reserve(std::min<int>(2 * components.size(), total_node_count));

    std::vector<int> visited(total_node_count);
    std::vector<int> cur_cluster(total_node_count);
    int cur_cluster_size = 0;

    for (int component_id = 0; component_id < components.size(); component_id++)
    {
      const Cluster &component = components[component_id];
      if (component.count <= 2)
      {
        // components with 2 or less points are already guaranteed to have distance < threshold
        // between any of their points
        final_clusters.push_back(component);
      }
      else
      {
        int prev_fc_size = final_clusters.size();
        std::fill_n(visited.data(), visited.size(), LABEL_ISOLATED);
        for (int i = 0; i < component.count; i++)
        {
          visited[component.point_ids[i]] = LABEL_UNVISITED;
        }
        int visited_count = 0;
        while (visited_count < component.count)
        {
          cur_cluster_size = 0;
          int start_id = 0; // rand() % component.count;
          int lead_id = -1;
          for (int i = 0; i < component.count; i++)
          {
            int id = component.point_ids[(start_id + i) % component.count];
            if (visited[id] == LABEL_UNVISITED)
            {
              lead_id = id;
              break;
            }
          }
          assert(lead_id != -1);
          visited[lead_id] = final_clusters.size();
          cur_cluster[cur_cluster_size++] = lead_id;

          // add all points from cluster that are reachable from lead_id but not yet visited
          for (int i = 0; i < sim_graph[lead_id].size(); i++)
          {
            int target_id = sim_graph[lead_id][i].target;
            if (visited[target_id] == LABEL_UNVISITED)
            {
              visited[target_id] = final_clusters.size();
              cur_cluster[cur_cluster_size++] = target_id;
            }
          }
          visited_count += cur_cluster_size;
          // printf("added cluster with %d points, %d/%d visited\n", cur_cluster_size, visited_count, component.count);

          final_clusters.emplace_back();
          final_clusters.back().lead_id = lead_id;
          final_clusters.back().count = cur_cluster_size;
          final_clusters.back().point_ids = std::vector<int>(cur_cluster.begin(), cur_cluster.begin() + cur_cluster_size);
        }
      }
    }

    final_clusters.shrink_to_fit();
  }

  struct SparseMatrix
  {
    struct Elem
    {
      Elem() = default;
      Elem(uint32_t _id, float _val) : id(_id), val(_val) {}
      uint32_t id = INVALID_IDX;
      float   val = 0.0f;
    };

    std::vector<std::vector<Elem>> rows; // one vector for each row
                                         // each sorted by id (column index)
    float default_value = 0.0f;

    inline float get(uint32_t i, uint32_t j) const
    {
      // no such element
      if (i >= rows.size() || j >= rows.size() || rows[i].empty())
        return default_value;
      if (rows[i][0].id > j || rows[i].back().id < j)
        return default_value;

      //binary search in i-th row
      int l = 0, r = rows[i].size() - 1;
      while (l <= r)
      {
        int m = (l + r) / 2;
        if (rows[i][m].id == j)
          return rows[i][m].val;
        else if (rows[i][m].id < j)
          l = m + 1;
        else // if (rows[i][m].id > j)
          r = m - 1;
      }
      return default_value;
    }
  };


  void clustering_hierarchical(const ClusteringSettings &settings, std::vector<Cluster> &final_clusters,
                               const std::vector<Cluster> &components, const std::vector<std::vector<Link>> &sim_graph,
                               int node_count)
  {
    auto t00 = std::chrono::high_resolution_clock::now();

    std::atomic<uint64_t> time_init_ns(0);
    std::atomic<uint64_t> time_table_init_ns(0);
    std::atomic<uint64_t> time_new_distances_ns(0);
    std::atomic<uint64_t> time_find_min_ns(0);
    std::atomic<uint64_t> time_ddg_traverse_ns(0);
    std::atomic<uint64_t> time_ddg_sort_ns(0);
    std::atomic<uint64_t> time_all_clusters_iterate_ns(0);
    std::atomic<uint64_t> time_prepare_output_ns(0);

    final_clusters.reserve(std::min<int>(2 * components.size(), node_count));

    int max_component_size = 0;
    for (int i = 0; i < components.size(); i++)
      max_component_size = std::max(max_component_size, components[i].count);

    std::vector<std::vector<int>> all_cluster_contents(components.size());
    std::vector<std::vector<CCluster>> all_clusters(components.size());
    std::atomic<int> min_clusters_count(0);

    std::vector<int> stack(node_count);
    std::vector<int> global_id_to_component_id(node_count, -1);
    std::vector<Dist> absolute_min_history(node_count);

    int max_clusters_count = 2 * max_component_size;
    std::vector<HCluster> line_clusters(max_clusters_count); //each line in the distance matrix is a cluster
    std::vector<Dist> line_min(max_clusters_count, Dist(-1, -1, MAX_DISTANCE));
    std::vector<float> dist_to_minU(max_clusters_count);
    std::vector<float> dist_to_minV(max_clusters_count);
    
    auto t0 = std::chrono::high_resolution_clock::now();
    time_init_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t0-t00).count();

    // Main cycle. Iterate over connected compinents and perform hierarchical clustering
    // for each component independently
    
    //You can turn it on, but I haven't got any performance boost from multithreading here
    //Probably because of the fast that we have one largest component that that will be the bottleneck
    //#pragma omp parallel for schedule(dynamic)
    for (int component_id = 0; component_id < components.size(); component_id++)
    {
      const Cluster &component = components[component_id];
      if (component.count == 1) //there is nothing to do here
      {
        #pragma omp critical
        {
          final_clusters.push_back(component);
        }
        min_clusters_count++;
      }
      else
      {        
        auto t1 = std::chrono::high_resolution_clock::now();

        int prev_fc_size = final_clusters.size();
        Dist absolute_min(-1, -1, MAX_DISTANCE);
        std::fill_n(line_min.data(), max_clusters_count, Dist(-1, -1, MAX_DISTANCE));

        //build global_id_to_component_id remap, will be needed only in the next cycle
        for (int i = 0; i < component.count; i++)
          global_id_to_component_id[component.point_ids[i]] = i;

        //create initial clusters, each contain only one point from the component
        SparseMatrix dist_mat;
        dist_mat.default_value = MAX_DISTANCE;
        dist_mat.rows.resize(max_clusters_count);
        for (int i = 0; i < component.count; i++)
        {
          line_clusters[i] = HCluster(i, -1, HC_VALID, 1);

          uint32_t links_count = sim_graph[component.point_ids[i]].size();
          dist_mat.rows[i].resize(links_count);

          for (int link_id = 0; link_id < links_count; link_id++)
          {
            const auto &link = sim_graph[component.point_ids[i]][link_id];
            int target_id = global_id_to_component_id[link.target];
            assert(target_id != -1);
            dist_mat.rows[i][link_id] = SparseMatrix::Elem(target_id, link.dist); 
            if (link.dist < line_min[i].dist)
              line_min[i] = Dist(i, target_id, link.dist);
          }
          // While rows in sim_graph are somewhat sorted, here we use relative ids (inside current component)
          // and there is no guarantee that they are sorted. Sort each row to allow binary search
          std::sort(dist_mat.rows[i].data(), dist_mat.rows[i].data() + links_count,
                    [](const SparseMatrix::Elem &a, const SparseMatrix::Elem &b) { return a.id < b.id; });

          //reserve some memory in the distance matrix
          dist_mat.rows[i].reserve(2 * links_count);

          if (line_min[i].dist < absolute_min.dist)
            absolute_min = line_min[i];
        }

        auto t2 = std::chrono::high_resolution_clock::now();
        time_table_init_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();

        int clusters_left = component.count;
        int next_cluster_pos = component.count;
        int step = 0;

        //ok, there is a main cycle of hierarchical clustering
        //on each step we find the closest pair of clusters and merge them
        //and find the next closest pair
        //we maintain the closest pair in absolute_min
        //and closest pair for each cluster in line_min
        while (absolute_min.dist < settings.similarity_threshold &&
               clusters_left >= 2)
        {
          // printf("merge clusters U=%d V=%d with d = %f\n", absolute_min.U, absolute_min.V, absolute_min.dist);
          auto t3 = std::chrono::high_resolution_clock::now();

          assert(step == 0 || absolute_min_history[step-1].dist <= absolute_min.dist);
          absolute_min_history[step] = absolute_min;

          // merge clusters to create new one
          line_clusters[next_cluster_pos] = HCluster(absolute_min.U, absolute_min.V, HC_VALID,
                                                     line_clusters[absolute_min.U].size + line_clusters[absolute_min.V].size); // merge U and V
          line_clusters[absolute_min.U].invalidation_step = step+1;                                                              // invalidate U
          line_clusters[absolute_min.V].invalidation_step = step+1;                                                              // invalidate V

          // fill dist_to_minU and dist_to_minV - it allows us to skip binary serch in dist_mat
          std::fill_n(dist_to_minU.data(), next_cluster_pos, MAX_DISTANCE);
          for (auto &elem : dist_mat.rows[absolute_min.U])
            dist_to_minU[elem.id] = elem.val;
          std::fill_n(dist_to_minV.data(), next_cluster_pos, MAX_DISTANCE);
          for (auto &elem : dist_mat.rows[absolute_min.V])
            dist_to_minV[elem.id] = elem.val;

          // find distances from this cluster to all other valid clusters
          int far_cluster = dist_mat.rows[absolute_min.U].size() > dist_mat.rows[absolute_min.V].size() ? absolute_min.V : absolute_min.U;
          dist_mat.rows[next_cluster_pos].reserve(dist_mat.rows[far_cluster].size());
          for (auto &elem : dist_mat.rows[far_cluster])
          {
            if (line_clusters[elem.id].invalidation_step == HC_VALID)
            {
              float d = std::max(dist_to_minU[elem.id], dist_to_minV[elem.id]);

              // add new distance if it is below threshold (otherwise it is not useful)
              // new distances will not break sorting (next_cluster_pos is the last cluster), so we can just append
              if (d < settings.similarity_threshold)
              {
                dist_mat.rows[elem.id].push_back(SparseMatrix::Elem(next_cluster_pos, d));
                dist_mat.rows[next_cluster_pos].push_back(SparseMatrix::Elem(elem.id, d));
              }

              if (d < line_min[elem.id].dist)
                line_min[elem.id] = Dist(elem.id, next_cluster_pos, d);
              if (d < line_min[next_cluster_pos].dist)
                line_min[next_cluster_pos] = Dist(next_cluster_pos, elem.id, d);
            }
          }

          auto t4 = std::chrono::high_resolution_clock::now();
          time_new_distances_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t4 - t3).count();

          // recalculate minimums in lines where it was invalidated and find new absolute minimum
          absolute_min = Dist(-1, -1, MAX_DISTANCE);
          for (int i = 0; i <= next_cluster_pos; i++)
          {
            if (line_clusters[i].invalidation_step == HC_VALID)
            {
              if (line_min[i].dist < absolute_min.dist)
              {
                // this minimum is a candidate to be the new absolute minimum and at the same time
                // it was invalidated. We have to recalculate it
                if (line_clusters[line_min[i].V].invalidation_step != HC_VALID)
                {
                  line_min[i] = Dist(-1, -1, MAX_DISTANCE);
                  for (auto elem : dist_mat.rows[i])
                  {
                    if (line_clusters[elem.id].invalidation_step == HC_VALID && elem.val < line_min[i].dist)
                      line_min[i] = Dist(i, elem.id, elem.val);
                  }
                }
                assert(line_min[i].U == -1 || line_clusters[line_min[i].V].invalidation_step == HC_VALID);

                // line min can be changed above
                if (line_min[i].dist < absolute_min.dist)
                  absolute_min = line_min[i];
              }
            }
          }

          auto t5 = std::chrono::high_resolution_clock::now();
          time_find_min_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t5 - t4).count();

          next_cluster_pos++;
          step++;
          clusters_left--;
        }

        //Now, after we finish clustering process, we build a clustering dendrogram
        //inside line_clusters, like every non-trivial cluster in this array has links to it's childeren
        //However, we are limited by similarity_threshold, so we probably haven't finished our dendrogram 
        //We also may want to get the desired number of clusters for the whole model, not the minimum
        //in each component.
        //To do all of this, we traverse this dendromgram (which is a forest, set of trees, actually)
        //And fill all_cluster_contents and all_clusters arrays
        //all_cluster_contents contains all the node ids and all_clusters have the cluster infos
        int top = 0;
        int cur_content_size = 0;
        int clusters_count = next_cluster_pos;

        auto t6 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < clusters_count; i++)
        {
          if (line_clusters[i].invalidation_step == HC_VALID)
          {
            stack[top] = i;
            top++;
          }
        }

        min_clusters_count += top; // these are clusters with HC_VALID invalidation_step
                                   // it means they cannot be merged together without breaking similarity threshold

        all_cluster_contents[component_id].resize(component.count, -1);
        all_clusters[component_id].resize(clusters_count);

        //Here we traverse the dendrogram and fill the arrays 
        //Dealing with explicit stack as it's faster than recursion
        while (top > 0)
        {
          int cur = stack[top - 1];
          top--;
          if (line_clusters[cur].size == 1) // it is a one-point cluster
          {
            all_clusters[component_id][cur].offset = cur_content_size;
            all_clusters[component_id][cur].size = 1;
            all_clusters[component_id][cur].creation_min_dist = 0.0f;
            all_clusters[component_id][cur].creation_step = 0;
            all_clusters[component_id][cur].invalidation_step = line_clusters[cur].invalidation_step;
            all_clusters[component_id][cur].cluster_id = cur;

            all_cluster_contents[component_id][cur_content_size] = component.point_ids[line_clusters[cur].U];
            cur_content_size++;
          }
          else
          {
            assert(cur >= component.count);
            int creation_step = cur - component.count;
            all_clusters[component_id][cur].offset = cur_content_size;
            all_clusters[component_id][cur].size = line_clusters[cur].size;
            all_clusters[component_id][cur].creation_min_dist = absolute_min_history[creation_step].dist;
            all_clusters[component_id][cur].creation_step = creation_step + 1;
            all_clusters[component_id][cur].invalidation_step = line_clusters[cur].invalidation_step;
            all_clusters[component_id][cur].cluster_id = cur;

            stack[top] = line_clusters[cur].U;
            stack[top + 1] = line_clusters[cur].V;
            top += 2;
          }
        }

        auto t7 = std::chrono::high_resolution_clock::now();
        time_ddg_traverse_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t7 - t6).count();

        //Sort the clusters by creation_min_dist
        //Remind that all_clusters have all the clusters from the dendrogram (incl. overlapping ones)
        //We then can iterate over this array to determine at which level of dendrogram we should stop
        std::sort(all_clusters[component_id].begin(), all_clusters[component_id].end(),
                  [](const CCluster &a, const CCluster &b)
                  { return a.creation_min_dist < b.creation_min_dist; });

        auto t8 = std::chrono::high_resolution_clock::now();
        time_ddg_sort_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t8 - t7).count();
      }
    }

    //Here we iterate all the cluster data from different components to determine at which level of dendrogram 
    //we should stop in each of them to met the target leaf count with the lowest error
    auto t9 = std::chrono::high_resolution_clock::now();
    std::vector<int> end_step_per_cluster(all_clusters.size(), MAX_STEPS);
    if (settings.target_leaf_count <= 0 || min_clusters_count >= settings.target_leaf_count)
    {
      // We have to take the last level of the clustering dendrogram in each component
      if (settings.target_leaf_count > 0)
        printf("Warning: settings.target_leaf_count is too low to influence the compression.\n");
    }
    else
    {
      // We should determine which level of dendrogram to take in each component

      // a.x is the component id, a.y is the current position
      auto list_cmp = [&all_clusters](int2 a, int2 b)
      {
        return all_clusters[a.x][a.y].creation_min_dist < all_clusters[b.x][b.y].creation_min_dist;
      };
      std::set<int2, decltype(list_cmp)> list_ends(list_cmp);

      int cur_nodes = node_count;
      // first, we need to add to the list the first actual clusters
      for (int i = 0; i < all_clusters.size(); i++)
      {
        end_step_per_cluster[i] = 0;
        for (int j = 0; j < all_clusters[i].size(); j++)
        {
          if (all_clusters[i][j].size > 1) // it is a cluster, not a single point
            list_ends.emplace(int2(i, j));
        }
      }

      while (cur_nodes > settings.target_leaf_count+1)
      {
        assert(list_ends.size() > 0);
        int2 end = *list_ends.begin();
        list_ends.erase(list_ends.begin());
        end_step_per_cluster[end.x] = all_clusters[end.x][end.y].creation_step;
        if (end.y + 1 < all_clusters[end.x].size())
          list_ends.emplace(int2(end.x, end.y + 1));
        cur_nodes--;
      }
    }

    auto t10 = std::chrono::high_resolution_clock::now();
    time_all_clusters_iterate_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t10 - t9).count();

    //Finally we transform the data into the format expected by the output
    for (int i = 0; i < all_clusters.size(); i++)
    {
      for (int j = 0; j < all_clusters[i].size(); j++)
      {
        CCluster &clust = all_clusters[i][j];
        if (clust.creation_step <= end_step_per_cluster[i] && clust.invalidation_step > end_step_per_cluster[i])
        {
          final_clusters.emplace_back();
          final_clusters.back().lead_id = all_cluster_contents[i][clust.offset + 0];
          final_clusters.back().count = clust.size;
          final_clusters.back().point_ids = std::vector<int>(all_cluster_contents[i].begin() + clust.offset, all_cluster_contents[i].begin() + clust.offset + clust.size);
        }
      }
    }

    auto t11 = std::chrono::high_resolution_clock::now();
    time_prepare_output_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t11 - t10).count();

    unsigned time_A = std::chrono::duration_cast<std::chrono::nanoseconds>(t9 - t0).count();

    printf("time_init                 (ms) = %.3f\n", 1e-6f*time_init_ns);
    printf("time_table_init           (ms) = %.3f\n", 1e-6f*time_table_init_ns);
    printf("time_new_distances        (ms) = %.3f\n", 1e-6f*time_new_distances_ns);
    printf("time_find_min             (ms) = %.3f\n", 1e-6f*time_find_min_ns);
    printf("time_ddg_traverse         (ms) = %.3f\n", 1e-6f*time_ddg_traverse_ns);
    printf("time_ddg_sort             (ms) = %.3f\n", 1e-6f*time_ddg_sort_ns);
    printf("time_all_clusters_iterate (ms) = %.3f\n", 1e-6f*time_all_clusters_iterate_ns);
    printf("time_prepare_output       (ms) = %.3f\n", 1e-6f*time_prepare_output_ns);
    printf("TOTAL                     (ms) = %.3f\n", 1e-6f*(time_table_init_ns + time_new_distances_ns + time_find_min_ns + 
                                                             time_ddg_traverse_ns + time_ddg_sort_ns + time_all_clusters_iterate_ns + 
                                                             time_prepare_output_ns + time_init_ns));

    final_clusters.shrink_to_fit();
  }

  void perform_clustering_advanced(const ISComDataset *in_dataset, const ClusteringSettings &settings, CompressionOutput &output)
  {
    auto tt1 = std::chrono::high_resolution_clock::now();
 
    uint32_t transform_count = in_dataset->get_transform_group()->elements.size();
    uint32_t point_count = in_dataset->get_point_count();
    std::vector<float> closest_dist(point_count, settings.similarity_threshold);

    Dataset dataset;
    create_dataset(in_dataset, 0, transform_count, dataset);

    Dataset no_rot_dataset;
    create_dataset(in_dataset, 0, 1, no_rot_dataset);

    auto tt2 = std::chrono::high_resolution_clock::now();
    printf("prepare data %.2f ms\n", 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(tt2-tt1).count());  

    nn_search::INNSearchAS *NN_search_AS = settings.nn_search_as.get();
    NN_search_AS->build(dataset, 32);

    dataset.all_points.clear();
    dataset.data_points.clear();

    auto tt3 = std::chrono::high_resolution_clock::now();
    printf("build NN search AS %.2f ms\n", 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(tt3-tt2).count());  

    int max_threads = omp_get_max_threads();
    std::vector<std::vector<Link>> sim_graph(point_count); //sparse adjacency matrix

    std::vector<std::vector<Link>> tmp_links(max_threads);
    for (int i=0; i<max_threads; i++)
      tmp_links[i].resize(transform_count*point_count, Link());
  

  auto t1 = std::chrono::high_resolution_clock::now();
    std::atomic<uint64_t> total_links(0);
    std::atomic<uint64_t> active_links(0);

    #pragma omp parallel for schedule(dynamic)
    for (int point_a = 0; point_a < no_rot_dataset.data_points.size(); ++point_a)
    {
      int thread_id = omp_get_thread_num();
      std::vector<float3> B_voxs_points, A_vox_points;
      float step = 0.25;
      A_vox_points.resize((1 / step + 1) * (1 / step + 1) * 3);
      B_voxs_points.resize(A_vox_points.size());
      int A_size = 0, B_size = 0;
      int link_count = 0;
      int i = no_rot_dataset.data_points[point_a].original_id;
      const DataPoint &A = no_rot_dataset.data_points[point_a];
      NN_search_AS->scan_near(no_rot_dataset.all_points.data() + A.data_offset, settings.similarity_threshold,
                              [&](float dist, unsigned point_b, const DataPoint &B, const float *) -> bool
                              {
                                if (A.original_id == B.original_id)
                                  return false;

                                tmp_links[thread_id][link_count].target = B.original_id;
                                tmp_links[thread_id][link_count].rotation = B.transform_id;
                                tmp_links[thread_id][link_count].data_offset = B.data_offset;
                                tmp_links[thread_id][link_count].dist = dist;

                                link_count++;
                                return false;
                              });

      std::sort(tmp_links[thread_id].begin(), tmp_links[thread_id].begin() + link_count, [](const Link &a, const Link &b)
      {
        return a.target != b.target ? (a.target < b.target) : (a.dist < b.dist);
      });

      int real_link_count = 0;
      int prev_target_id = -1;
      for (int j=0; j<link_count; j++)
      {
        if (prev_target_id != tmp_links[thread_id][j].target)
        {
          tmp_links[thread_id][real_link_count] = tmp_links[thread_id][j];
          prev_target_id = tmp_links[thread_id][j].target;
          real_link_count++;
        }
      }

      total_links += point_count;
      active_links += real_link_count;

      sim_graph[i] = std::vector<Link>(tmp_links[thread_id].begin(), tmp_links[thread_id].begin() + real_link_count);
    }

    int64_t tmp_links_size = omp_get_num_threads()*transform_count*point_count*sizeof(Link);
    int64_t sim_graph_size = active_links.load()*sizeof(Link);
    printf("total links = %d Mb, active links = %d Mb )\n", int(tmp_links_size/1000000), int(sim_graph_size/1000000));

    auto t2 = std::chrono::high_resolution_clock::now();
    printf("build links %.2f ms\n", 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count());

    //find connected components
    std::vector<int> labels(point_count, LABEL_UNVISITED);
    std::vector<Cluster> components;
    std::vector<unsigned> stack(2*point_count);
    int head = 0;
    for (int lead_id=0;lead_id<point_count;lead_id++)
    {
      if (labels[lead_id] != LABEL_UNVISITED)
        continue;
      else 
      {
        int cur_component_id = components.size();
        components.emplace_back();

        int total_count = 0;
        if (sim_graph[lead_id].empty())
        {
          total_count = 1;
          labels[lead_id] = LABEL_ISOLATED;
          components[cur_component_id].point_ids.push_back(lead_id);
        }
        else
        {
          head = 0;
          stack[head] = lead_id;
          labels[lead_id] = cur_component_id;

          while (head >= 0)
          {
            int node_id = stack[head];
            head--;
            total_count++;
            components[cur_component_id].point_ids.push_back(node_id);
            for (int i=0; i<sim_graph[node_id].size(); i++)
            {
              int target_id = sim_graph[node_id][i].target;
              if (labels[target_id] == LABEL_UNVISITED)
              {
                stack[++head] = target_id;
                assert(head < stack.size());
                labels[target_id] = labels[node_id];
              }
            }
          }
        }
        components[cur_component_id].lead_id = lead_id;
        components[cur_component_id].count = total_count;
      }
    }

    auto t3 = std::chrono::high_resolution_clock::now();
    printf("find connected components %.2f ms\n", 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(t3-t2).count());

    // hierarchical clustering requires a matrix N*N with N is the size of the largest component
    // so if it is too big, we have to use recursive fill. Check it here.
    constexpr int HC_COMPONENT_SIZE_LIMIT = 150000;
    int max_component_size = components[0].count;
    for (int i = 1; i < components.size(); i++)
      max_component_size = std::max(max_component_size, components[i].count);
    if (max_component_size > HC_COMPONENT_SIZE_LIMIT && settings.clustering_algorithm == ClusteringAlgorithm::HIERARCHICAL)
      printf("WARNING: max component size (%d) is too big, using recursive fill instead of hierarchical\n", max_component_size);

    //clustering
    std::vector<Cluster> final_clusters;
    if (settings.clustering_algorithm == ClusteringAlgorithm::COMPONENTS_RECURSIVE_FILL ||
        max_component_size > HC_COMPONENT_SIZE_LIMIT)
    {
      clustering_recursive_fill(final_clusters, components, sim_graph, point_count);
    }
    else if (settings.clustering_algorithm == ClusteringAlgorithm::HIERARCHICAL)
    {
      clustering_hierarchical(settings, final_clusters, components, sim_graph, point_count);
    }

    auto t4 = std::chrono::high_resolution_clock::now();
    printf("clustering %.2f ms\n", 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(t4-t3).count());  

    //prepare rotations for clusters
    //clustering here guarantees that all nodes in each cluster are closer than similarity_threshold
    //to the leading node, it means that we have Link between them in our graph and it contains optimal rotation
    for (auto &cluster : final_clusters)
    {
      assert(cluster.count >= 1);
      assert(cluster.count == cluster.point_ids.size());

      cluster.rotations.resize(cluster.count);
      
      for (int i = 0; i < cluster.count; i++)
      {
        int n_id = cluster.point_ids[i];

        if (n_id == cluster.lead_id)
        {
          cluster.rotations[i] = 0;
          continue;
        }

        cluster.rotations[i] = -1;

        //binary search in sim_graph[cluster.lead_id]
        int left = 0;
        int right = sim_graph[cluster.lead_id].size() - 1;
        while (left <= right)
        {
          int mid = (left + right) / 2;
          if (sim_graph[cluster.lead_id][mid].target < n_id)
            left = mid + 1;
          else if (sim_graph[cluster.lead_id][mid].target > n_id)
            right = mid - 1;
          else
          {
            cluster.rotations[i] = sim_graph[cluster.lead_id][mid].rotation;
            break;
          }
        }
        
        assert(cluster.rotations[i] != (unsigned short)-1);
      }
    }

    output.clusters = final_clusters;
    
    auto t5 = std::chrono::high_resolution_clock::now();
    printf("prepare final clusters %.2f ms\n", 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(t5-t4).count()); 
    printf("total time = %.2f ms\n", 1e-3f*std::chrono::duration_cast<std::chrono::microseconds>(t4-tt1).count());
    
    int remapped = 0;
    for (int i = 0; i < final_clusters.size(); i++)
      remapped += final_clusters[i].count - 1;

    printf("remapped %d/%d nodes\n", remapped, point_count);
  }

  struct TransformCompact
  {
    unsigned rotation_id = 0;
    float add = 0.0f;
  };

  //replacement
  void perform_clustering_replacement(const ISComDataset *in_dataset, const ClusteringSettings &settings, CompressionOutput &output)
  {
    float time_prepare = 0;
    float time_create_dataset = 0;
    float time_build_AS = 0;
    float time_replacement = 0;
    float time_finalize = 0;

    auto t1 = std::chrono::high_resolution_clock::now();

    uint32_t point_count = in_dataset->get_point_count();

    std::vector<float> closest_dist(point_count, settings.similarity_threshold);
    std::vector<unsigned> remap(point_count);
    std::vector<bool> is_parent(point_count, 0);
    std::vector<TransformCompact> remap_transforms(point_count);

    for (int i=0; i<point_count; i++)
    {
      remap[i] = i;
      remap_transforms[i] = TransformCompact();
    }

    Dataset dataset_no_rot;
    create_dataset(in_dataset, 0, 1, dataset_no_rot);

    int transform_count = in_dataset->get_transform_group()->elements.size();
    size_t expected_full_dataset_size = transform_count*(dataset_no_rot.all_points.size()*sizeof(float) + dataset_no_rot.data_points.size()*sizeof(DataPoint));
    const size_t dataset_size_limit = (size_t)settings.clustering_dataset_size_limit_GB * 1024ul * 1024ul * 1024ul;

    int batches = ceil((double)expected_full_dataset_size/dataset_size_limit);
    if (batches > transform_count)
      printf("WARNING: expected batch dataset size (%zu) will exceed limit even with maximum batches\n", expected_full_dataset_size/transform_count);
    else if (batches > 1)
      printf("WARNING: datset will be split into %d batches, compression quality will be reduced\n", batches);
    
    int splits = std::min(batches, transform_count);
    int step = (transform_count + splits - 1) / splits;
    int remapped = 0;

    auto t2 = std::chrono::high_resolution_clock::now();
    time_prepare += std::chrono::duration<float, std::milli>(t2 - t1).count();

    for (int split = 0; split < splits; split++)
    {
      auto tt1 = std::chrono::high_resolution_clock::now();

      Dataset dataset;
      int rot_start = split*step;
      int rot_end = std::min(transform_count, (split+1)*step);
      int rot_count = rot_end - rot_start;
      create_dataset(in_dataset, rot_start, rot_end, dataset);

      auto tt2 = std::chrono::high_resolution_clock::now();

      nn_search::INNSearchAS *NN_search_AS = settings.nn_search_as.get();
      NN_search_AS->build(dataset, 32);

      dataset.all_points.clear();
      dataset.data_points.clear();

      auto tt3 = std::chrono::high_resolution_clock::now();

      for (int point_a = 0; point_a < point_count; point_a++)
      {
        int i  = dataset_no_rot.data_points[point_a].original_id;
        if (remap[i] != i)
          continue;
        
        remap_transforms[i].rotation_id = 0;
        remap_transforms[i].add = dataset_no_rot.data_points[point_a].average_val;
        
        size_t off_a = dataset_no_rot.data_points[point_a].data_offset;

        NN_search_AS->scan_near(dataset_no_rot.all_points.data() + off_a, settings.similarity_threshold,
          [&](float dist, unsigned point_b, const DataPoint &B, const float *) -> bool {
            if (point_b < (point_a + 1)*rot_count)
              return false;
            
            int j = B.original_id;
            if (dist < closest_dist[j] && !is_parent[j])
            {
              remapped += remap[j] == j;
              remap[j] = i;
              is_parent[i] = true;
              remap_transforms[j].rotation_id = B.transform_id;
              remap_transforms[j].add = B.average_val;
              closest_dist[j] = dist;
            }
            return false;
        });
      }
      auto tt4 = std::chrono::high_resolution_clock::now();
      time_create_dataset += std::chrono::duration<float, std::milli>(tt2 - tt1).count();
      time_build_AS += std::chrono::duration<float, std::milli>(tt3 - tt2).count();
      time_replacement += std::chrono::duration<float, std::milli>(tt4 - tt3).count();
    }

    auto t3 = std::chrono::high_resolution_clock::now();

    std::vector<Cluster> clusters;
    std::vector<int> lead_node_id_to_cluster_id(point_count, -1);

    for (int i = 0; i < point_count; i++)
    {
      if (remap[i] == i)
      {
        clusters.emplace_back();
        clusters.back().lead_id = i;
        lead_node_id_to_cluster_id[i] = clusters.size() - 1;
      }
    }
    for (int i = 0; i < point_count; i++)
    {
      Cluster &cur_cluster = clusters[lead_node_id_to_cluster_id[remap[i]]];
      cur_cluster.point_ids.push_back(i);
      cur_cluster.count++;
      cur_cluster.rotations.push_back(remap_transforms[i].rotation_id);
    }

    output.clusters = std::move(clusters);
    auto t4 = std::chrono::high_resolution_clock::now();
    time_finalize += std::chrono::duration<float, std::milli>(t4 - t3).count();

    bool verbose = false;
    if (verbose)
    {
      float total_time = time_prepare + time_create_dataset + time_build_AS + time_replacement + time_finalize;
      printf("  %d/%d nodes left\n", point_count - remapped, point_count);
      printf("  total time: %.1f s (%.1f ms/leaf)\n", 1e-3f*total_time, total_time/point_count);
      printf("    time_prepare:        %.1f ms\n", time_prepare);
      printf("    time_create_dataset: %.1f ms\n", time_create_dataset);
      printf("    time_build_AS:       %.1f ms\n", time_build_AS);
      printf("    time_replacement:    %.1f ms\n", time_replacement);
      printf("    time_finalize:       %.1f ms\n", time_finalize);

      if (point_count > 1000)
      {
        stat_utils::RangeBins<int> cluster_sizes({1, 2, 3, 4, 5, 10, 25, 50, 100, 500});
        for (auto &cluster : output.clusters)
        {
          cluster_sizes.add(cluster.count);
        }
        cluster_sizes.print_bins();
      }
    }
  }

  void create_single_dataset(const ISComDataset *in_dataset, uint32_t p_id,
                             uint32_t rot_start, uint32_t rot_end, Dataset &dataset)
  {
    assert(rot_start >= 0);
    assert(rot_end <= in_dataset->get_transform_group()->elements.size());
    assert(rot_start < rot_end);

    uint32_t rot_count = rot_end - rot_start;
    uint32_t real_dim = in_dataset->get_descriptor_size();
    dataset.dim = ISComDataset::get_dim_with_alignment(real_dim);

    dataset.data_points.resize(rot_count);
    dataset.all_points.resize(rot_count * dataset.dim, 0.0f);

    uint32_t desc_count = rot_count;
    std::vector<uint32_t> p_ids(desc_count);
    std::vector<uint16_t> rot_ids(desc_count);

    for (int r = 0; r < rot_count; r++)
    {
      dataset.data_points[r].data_offset = r * dataset.dim;
      dataset.data_points[r].original_id = p_id;
      dataset.data_points[r].transform_id = rot_start + r;
      dataset.data_points[r].average_val = 0; // unused

      p_ids[r] = p_id;
      rot_ids[r] = rot_start + r;
    }

    in_dataset->calculate_descriptors(desc_count, p_ids.data(), rot_ids.data(), dataset.all_points.data(), dataset.dim);
  }

  struct RemapElem
  {
    float closest_dist = 1000.0f;
    uint32_t remap = INVALID_IDX;
    bool is_parent = false;
    uint16_t rot_id = ROT_ID_IDENTITY;
    float add = 0.0f;
  };

  //inverse_replacement
  void perform_clustering_inverse_replacement(const ISComDataset *in_dataset, 
                                              const ClusteringSettings &settings, 
                                              CompressionOutput &output,
                                              bool verbose)
  {
    float search_early_stop_thr = 0.75f;
    float rebuild_AS_thr        = 0.25f;

    //rebuild_AS_thr = 10;

    float time_create_initial_dataset = 0;
    float time_create_AS = 0;
    float time_rebuild_AS = 0;
    float time_create_descriptors = 0;
    float time_AS_queries = 0;
    float time_finalize = 0;

    auto t1 = std::chrono::high_resolution_clock::now();

    const uint32_t point_count = in_dataset->get_point_count();
    const uint32_t transform_count = in_dataset->get_transform_group()->elements.size();
    std::vector<RemapElem> remap_elems(point_count);

    for (int i=0; i<point_count; i++)
    {
      remap_elems[i].closest_dist = settings.similarity_threshold;
      remap_elems[i].remap = i;
    }

    Dataset dataset_no_rot;
    create_dataset(in_dataset, 0, 1, dataset_no_rot);

    auto t2 = std::chrono::high_resolution_clock::now();

    nn_search::INNSearchAS *NN_search_AS = settings.nn_search_as.get();
    NN_search_AS->build(dataset_no_rot, 32);

    auto t3 = std::chrono::high_resolution_clock::now();

    const uint32_t aligned_dim = ISComDataset::get_dim_with_alignment(in_dataset->get_descriptor_size());
    nn_search::aligned_vector_f all_points(transform_count * aligned_dim, 0.0f);
    std::vector<uint32_t> p_ids(transform_count);
    std::vector<uint16_t> rot_ids(transform_count);

    for (int r = 0; r < transform_count; r++)
      rot_ids[r] = r;

    int remapped = 0;
    int unused_nodes_in_AS = 0;
    int nodes_in_AS = point_count;
    for (uint32_t i = 0; i < point_count; i++)
    {
      unused_nodes_in_AS += remap_elems[i].closest_dist >= search_early_stop_thr*settings.similarity_threshold;

      if (remap_elems[i].remap != i)
        continue;

      auto tt1 = std::chrono::high_resolution_clock::now();

      for (int r = 0; r < transform_count; r++)
        p_ids[r] = i;
      in_dataset->calculate_descriptors(transform_count, p_ids.data(), rot_ids.data(), all_points.data(), aligned_dim);

      auto tt2 = std::chrono::high_resolution_clock::now();

      #pragma omp parallel for schedule(static)
      for (int rot_id = 0; rot_id < transform_count; rot_id++)
      {
        auto scan_callback = [&](float dist, unsigned point_b, const DataPoint &B, const float *) -> bool
        {
          int j = B.original_id;

          if (j <= i)
            return false;

          RemapElem &re = remap_elems[j];
          if (dist < re.closest_dist && !re.is_parent)
          {
            #pragma omp critical
            {
              if (dist < search_early_stop_thr*settings.similarity_threshold &&
                  re.closest_dist >= search_early_stop_thr*settings.similarity_threshold)
              {
                unused_nodes_in_AS++;
              }
              remapped += re.remap == j;
              re.remap = i;
              re.is_parent = true;
              re.rot_id = in_dataset->get_transform_group()->inverse_indices[rot_id];
              re.add = B.average_val;
              re.closest_dist = dist;
            }
          }
          return false;
        };
        NN_search_AS->scan_near(all_points.data() + rot_id * aligned_dim, settings.similarity_threshold, scan_callback);
      }

      auto tt3 = std::chrono::high_resolution_clock::now();

      // close match heuristic, rebuilding acceleration structure with less points
      // nodes that are already remapped and close to their cluster centers are 
      // removed from the AS (ball tree/kdtree)
      if (settings.use_close_match_heuristic && nodes_in_AS > 256 && unused_nodes_in_AS > rebuild_AS_thr*nodes_in_AS)
      {
        // put node that can be remapped to the beginning of the array
        uint32_t cur_point = 0;
        for (int j=0; j<nodes_in_AS; j++)
        {
          uint32_t id = dataset_no_rot.data_points[j].original_id;
          if (id > i && !remap_elems[id].is_parent && 
              remap_elems[id].closest_dist > search_early_stop_thr*settings.similarity_threshold)
          {
            dataset_no_rot.data_points[cur_point] = dataset_no_rot.data_points[j];
            cur_point++;
          }
        }

        nodes_in_AS = cur_point;
        unused_nodes_in_AS = 0;
        dataset_no_rot.data_points.resize(nodes_in_AS);
        NN_search_AS->build(dataset_no_rot, 32);
      }

      auto tt4 = std::chrono::high_resolution_clock::now();

      time_create_descriptors += std::chrono::duration<float, std::milli>(tt2 - tt1).count();
      time_AS_queries += std::chrono::duration<float, std::milli>(tt3 - tt2).count();
      time_rebuild_AS += std::chrono::duration<float, std::milli>(tt4 - tt3).count();
    }

    auto t4 = std::chrono::high_resolution_clock::now();

    std::vector<Cluster> clusters;
    std::vector<int> lead_node_id_to_cluster_id(point_count, -1);

    for (int i = 0; i < point_count; i++)
    {
      if (remap_elems[i].remap == i)
      {
        clusters.emplace_back();
        clusters.back().lead_id = i;
        lead_node_id_to_cluster_id[i] = clusters.size() - 1;
      }
    }
    for (int i = 0; i < point_count; i++)
    {
      Cluster &cur_cluster = clusters[lead_node_id_to_cluster_id[remap_elems[i].remap]];
      cur_cluster.point_ids.push_back(i);
      cur_cluster.count++;
      cur_cluster.rotations.push_back(remap_elems[i].rot_id);
    }

    output.clusters = std::move(clusters);
    auto t5 = std::chrono::high_resolution_clock::now();

    time_create_initial_dataset += std::chrono::duration<float, std::milli>(t2 - t1).count();
    time_create_AS += std::chrono::duration<float, std::milli>(t3 - t2).count();
    time_finalize += std::chrono::duration<float, std::milli>(t5 - t4).count();

    if (verbose)
    {
      float total_time = time_create_initial_dataset + time_create_AS + time_create_descriptors +
                         time_rebuild_AS + time_AS_queries + time_finalize;
      printf("  %d/%d nodes left\n", point_count - remapped, point_count);
      printf("  total time: %.1f s (%.1f ms/leaf)\n", 1e-3f*total_time, total_time/point_count);
      printf("    time_create_initial_dataset: %.1f ms\n", time_create_initial_dataset);
      printf("    time_create_AS:              %.1f ms\n", time_create_AS);
      printf("    time_rebuild_AS:             %.1f ms\n", time_rebuild_AS);
      printf("    time_create_descriptors:     %.1f ms\n", time_create_descriptors);
      printf("    time_AS_queries:             %.1f ms\n", time_AS_queries);
      printf("    time_finalize:               %.1f ms\n", time_finalize);

      if (false)
      {
        stat_utils::RangeBins<int> cluster_sizes({1, 2, 3, 4, 5, 10, 25, 50, 100, 500});
        for (auto &cluster : output.clusters)
        {
          cluster_sizes.add(cluster.count);
        }
        cluster_sizes.print_bins();
      }

      if (true)
      {
        std::vector<float> distances;
        distances.reserve(point_count);
        for (int i = 0; i < point_count; i++)
        {
          if (remap_elems[i].remap != i)
            distances.push_back(remap_elems[i].closest_dist);
          else
            distances.push_back(0);
        }

        printf("Average clustering error:   %.5f\n", stat_utils::mean(distances));
        printf("Median clustering error:    %.5f\n", stat_utils::median(distances));
        printf("1%% worst clustering error: %.5f\n", stat_utils::quantile(distances, 0.99));
        printf("Max clustering error:       %.5f\n", stat_utils::max(distances));
      }
    }
  }

  float sqr_dist(int dimension, const float *a, const float *b)
  {
    float res = 0;
    for (int i = 0; i < dimension; ++i)
    {
      res += (a[i] - b[i]) * (a[i] - b[i]);
    }
    return res;
  }

  void write_centroid(int dimension, float *centroid, const float *point)
  {
    for (int i = 0; i < dimension; ++i)
    {
      centroid[i] = point[i];
    }
  }

  void recount_centroid(int dimension, int &count, float *centroid, const float *point)
  {
    for (int i = 0; i < dimension; ++i)
    {
      centroid[i] = (centroid[i] * count + point[i]) / (count + 1);
    }
    ++count;
  }

  void recount_centroid(int dimension, int &count, float *centroid, int count_2, const float *centroid_2)
  {
    for (int i = 0; i < dimension; ++i)
    {
      centroid[i] = (centroid[i] * count + centroid_2[i] * count_2) / (count + count_2);
    }
    count += count_2;
  }

  void perform_clustering(const ISComDataset *in_dataset, const ClusteringSettings &settings, CompressionOutput &output, bool verbose)
  {
    // edge case, dataset has 0 descriptor size, it can happen if NO_EMBEDDING is used, or all the data
    // was "hashed-out" and embedded into sub-dataset split. It is a valid case, we just need to output 
    // one cluster with all the points in it
    if (in_dataset->get_descriptor_size() == 0)
    {
      output.clusters.resize(1);
      output.clusters[0].count = in_dataset->get_point_count();
      output.clusters[0].lead_id = 0;
      output.clusters[0].point_ids.resize(output.clusters[0].count);
      output.clusters[0].rotations.resize(output.clusters[0].count);
      for (int i = 0; i < output.clusters[0].count; ++i)
      {
        output.clusters[0].point_ids[i] = i;
        output.clusters[0].rotations[i] = ROT_ID_IDENTITY;
      }
      return;
    }

    if (settings.clustering_algorithm == ClusteringAlgorithm::HIERARCHICAL || 
        settings.clustering_algorithm == ClusteringAlgorithm::COMPONENTS_RECURSIVE_FILL)
    {
      perform_clustering_advanced(in_dataset, settings, output);
    }
    else if (settings.clustering_algorithm == ClusteringAlgorithm::REPLACEMENT)
    {
      perform_clustering_replacement(in_dataset, settings, output);
    }
    else if (settings.clustering_algorithm == ClusteringAlgorithm::INVERSE_REPLACEMENT)
    {
      perform_clustering_inverse_replacement(in_dataset, settings, output, verbose);
    }
    else if (settings.clustering_algorithm == ClusteringAlgorithm::LEADER)
    {
      printf("[scom2::perform_clustering] ERROR: old LEADER clustering implementation is deprecated. Use INVERSE_REPLACEMENT\n");
    }
    else
    {
      printf("[scom2::perform_clustering] ERROR: unknown clustering algorithm\n");
    }
  }
}
