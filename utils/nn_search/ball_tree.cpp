#include "ball_tree.h"
#include "omp.h"
#include <chrono>
#include <array>

namespace nn_search
{
  BallTree::BallTree(IDistanceFunction *dist_func)
  {
    if (!dist_func->isMetric())
    {
      printf("[BallTree] Error: BallTree requires a metric as a distance function. Futher behavior is undefined.\n");
      assert(false);
    }
    m_dist_func = dist_func;
  }

  void BallTree::build(const Dataset &dataset, int max_leaf_size)
  {
    m_dim = dataset.dim;

    m_points_data.clear();
    m_points.clear();
    m_original_ids.clear();
    m_nodes.clear();
    m_centroids_data.clear();

    m_points_data.reserve(dataset.all_points.size());
    m_points.reserve(dataset.data_points.size());
    m_original_ids.reserve(dataset.data_points.size());
    m_nodes.reserve(dataset.data_points.size());
    m_centroids_data.reserve(dataset.all_points.size());

    m_index.resize(dataset.data_points.size());
    for (int i = 0; i < dataset.data_points.size(); ++i)
      m_index[i] = i;

    int max_threads = omp_get_max_threads();
    m_tmp_vec.resize(m_dim*max_threads, 0);

    m_dataset = &dataset;

    #pragma omp parallel
    #pragma omp single
    {
      build_rec(max_leaf_size, dataset.data_points.size(), 0);
    }
  
    m_points_data.shrink_to_fit();
    m_points.shrink_to_fit();
    m_original_ids.shrink_to_fit();
    m_nodes.shrink_to_fit();
    m_centroids_data.shrink_to_fit();
  }

  int BallTree::build_rec(int max_leaf_size, int n, int offset)
  {
    const Dataset &dataset = *m_dataset;
    int *index = m_index.data() + offset;
    float *tmp_vec = m_tmp_vec.data();

    int cur_thread_id = omp_get_thread_num();
    unsigned cur_node_id = 0;

    #pragma omp critical
    {
      assert(m_nodes.capacity() >= m_nodes.size() + 1);
      if (m_nodes.capacity() <= m_nodes.size() + 1)
        printf("BallTreeL2::build_rec: increase m_nodes.capacity()\n");
      assert(m_centroids_data.capacity() >= m_centroids_data.size() + m_dim);
      if (m_centroids_data.capacity() <= m_centroids_data.size() + m_dim)
        printf("BallTreeL2::build_rec: increase m_centroids_data.capacity()\n");
      
      cur_node_id = m_nodes.size();
      m_nodes.emplace_back();
      m_centroids_data.resize(m_centroids_data.size() + m_dim);
    }

    if (n <= max_leaf_size)
    {
      // build leaf node
      unsigned start_id = 0;

      #pragma omp critical
      {
        start_id = m_points.size();
        m_points.resize(start_id + n);
        m_original_ids.resize(start_id + n);
        m_points_data.resize(m_points_data.size() + n * m_dim);
      }
      for (int i = 0; i < n; ++i)
      {
        const DataPoint &dp = dataset.data_points[index[i]];
        m_points[start_id + i] = dp;
        m_original_ids[start_id + i] = index[i];
        memcpy(m_points_data.data() + (start_id + i) * m_dim, 
               dataset.all_points.data() + dp.data_offset, 
               m_dim * sizeof(float));
      }

      //We can find more optimal bounding sphere with center that is not a centroid
      //e.g. check CGAL Min_sphere_of_spheres_d_impl.h
      float c_mult = 1.0f/n;
      for (int j = 0; j < m_dim; ++j)
        m_centroids_data[cur_node_id*m_dim + j] = 0;
      for (int i = 0; i < n; ++i)
      {
        const float *p = dataset.all_points.data() + dataset.data_points[index[i]].data_offset;
        for (int j = 0; j < m_dim; ++j)
          m_centroids_data[cur_node_id*m_dim + j] += c_mult*p[j];
      }

      float max_dist = 0;
      for (int i = 0; i < n; ++i)
      {
        const float *p = dataset.all_points.data() + dataset.data_points[index[i]].data_offset;
        float dist = distance(m_centroids_data.data() + cur_node_id * m_dim, p);
        max_dist = std::max(max_dist, dist);
      }

      m_nodes[cur_node_id].start_index = start_id;
      m_nodes[cur_node_id].count = n;
      m_nodes[cur_node_id].l_idx = 0;
      m_nodes[cur_node_id].r_idx = 0;
      m_nodes[cur_node_id].centroid_index = cur_node_id;
      m_nodes[cur_node_id].radius = max_dist;
    }
    else
    {
      int left = 0, right = n - 1, cnt = 0;
      do
      {
        // build internal node
        int x_p = rand() % n;
        int l_p = find_furthest_id(dataset, x_p, n, index);
        int r_p = find_furthest_id(dataset, l_p, n, index);
        assert(l_p != r_p);

        // note: we use l_p and r_p as two pivots
        const float *l_pivot = dataset.all_points.data() + dataset.data_points[index[l_p]].data_offset;
        const float *r_pivot = dataset.all_points.data() + dataset.data_points[index[r_p]].data_offset;
        float l_sqr = 0.0f, r_sqr = 0.0f;
        for (int i = 0; i < m_dim; ++i)
        {
          float l_v = l_pivot[i], r_v = r_pivot[i];
          tmp_vec[cur_thread_id * m_dim + i] = r_v - l_v;
          l_sqr += l_v*l_v;
          r_sqr += r_v*r_v;
        }
        float b = 0.5f * (l_sqr - r_sqr);

        left = 0, right = n - 1;
        while (left <= right)
        {
          const float *x = dataset.all_points.data() + dataset.data_points[index[left]].data_offset;
          float inner_product = 0;
          for (int i = 0; i < m_dim; ++i)
            inner_product += tmp_vec[cur_thread_id * m_dim + i] * x[i];
          float val = inner_product + b;
          if (val < 0.0f)
            ++left;
          else
          {
            std::swap(index[left], index[right]);
            --right;
          }
        }
        ++cnt;
      } while ((left <= 0 || left >= n) && cnt <= 3); // ensure split into two parts
      if (cnt > 3)
        left = n / 2;

      int l_idx = -1, r_idx = -1;

      const int parallel_threshold = 8*max_leaf_size; //some empiric value to avoid too much overhead
      if (left > parallel_threshold)
      {
#ifndef DISABLE_OMP_TASKS
        #pragma omp task shared(l_idx)
#endif
        l_idx = build_rec(max_leaf_size, left, offset);
      }
      else
      {
        l_idx = build_rec(max_leaf_size, left, offset);
      }

      if (n - left > parallel_threshold)
      {
#ifndef DISABLE_OMP_TASKS
        #pragma omp task shared(r_idx)
#endif
        r_idx = build_rec(max_leaf_size, n - left, offset + left);
      }
      else
      {
        r_idx = build_rec(max_leaf_size, n - left, offset + left);
      }
#ifndef DISABLE_OMP_TASKS
      #pragma omp taskwait
#endif
      assert(l_idx != -1 && r_idx != -1);

      const Node &l_node = m_nodes[l_idx];
      const Node &r_node = m_nodes[r_idx];

      float c_mult_1 = l_node.count/(float)(l_node.count + r_node.count);
      float c_mult_2 = r_node.count/(float)(l_node.count + r_node.count);
      for (int i = 0; i < n; ++i)
      {
        for (int j = 0; j < m_dim; ++j)
          m_centroids_data[cur_node_id*m_dim + j] = c_mult_1*m_centroids_data[l_node.centroid_index*m_dim + j] + 
                                                    c_mult_2*m_centroids_data[r_node.centroid_index*m_dim + j];
      }

      float max_dist = 0;
      for (int i = 0; i < n; ++i)
      {
        const float *p = dataset.all_points.data() + dataset.data_points[index[i]].data_offset;
        float dist = distance(m_centroids_data.data() + cur_node_id * m_dim, p);
        max_dist = std::max(max_dist, dist);
      }

      m_nodes[cur_node_id].start_index = 0xFFFFFFFFu;
      m_nodes[cur_node_id].count = l_node.count + r_node.count;
      m_nodes[cur_node_id].l_idx = l_idx;
      m_nodes[cur_node_id].r_idx = r_idx;
      m_nodes[cur_node_id].centroid_index = cur_node_id;
      m_nodes[cur_node_id].radius = max_dist;
    }

    return cur_node_id;
  }

  int BallTree::find_furthest_id(const Dataset &dataset, int id_from, int n, int *index)
  {
    int far_id = -1;
    float far_dist = -1.0f;

    const float *a = dataset.all_points.data() + dataset.data_points[index[id_from]].data_offset;
    for (int i = 0; i < n; ++i)
    {
      if (i == id_from)
        continue;

      const float *b = dataset.all_points.data() + dataset.data_points[index[i]].data_offset;
      float dist = distance(a, b);
      if (far_dist < dist)
      {
        far_dist = dist;
        far_id = i;
      }
    }
    return far_id;
  }

  const float *BallTree::get_closest_point(const float *query, float max_dist, float *dist_to_nearest) const
  {
    constexpr unsigned MAX_STACK_SIZE = 128;
    std::array<unsigned, MAX_STACK_SIZE> stack;
    int top = 0;
    stack[top] = 0;
    top = 1;
    const float *cur_nearest = nullptr;
    float nearest_dist = max_dist;
    while (top > 0)
    {
      const Node &cur_node = m_nodes[stack[top - 1]];
      top--;

      if (cur_node.l_idx == 0) // is leaf
      {
        for (int i = 0; i < cur_node.count; ++i)
        {
          // compute the actual distance
          const float *point = m_points_data.data() + (cur_node.start_index + i) * m_dim;
          float dist = distance(query, point);

          if (dist < nearest_dist)
          {
            nearest_dist = dist;
            cur_nearest = point;
          }
        }
      }
      else
      {
        const Node &left_node = m_nodes[cur_node.l_idx];
        float left_center_dist = distance(query, m_centroids_data.data() + left_node.centroid_index * m_dim);
        if (left_center_dist < left_node.radius + nearest_dist)
        {
          stack[top] = cur_node.l_idx;
          top++;
        }

        const Node &right_node = m_nodes[cur_node.r_idx];
        float right_center_dist = distance(query, m_centroids_data.data() + right_node.centroid_index * m_dim);
        if (right_center_dist < right_node.radius + nearest_dist)
        {
          stack[top] = cur_node.r_idx;
          top++;
        }
      }
    }

    if (cur_nearest && dist_to_nearest)
    {
      *dist_to_nearest = nearest_dist;
    }
    return cur_nearest;
  }

  int BallTree::scan_near(const float *query, float max_dist, ScanFunction callback) const
  {
    constexpr unsigned MAX_STACK_SIZE = 128;
    std::array<unsigned, MAX_STACK_SIZE> stack;
    int top = 0;
    stack[top] = 0;
    top = 1;
    int count = 0;

    while (top > 0)
    {
      const Node &cur_node = m_nodes[stack[top - 1]];
      top--;

      if (cur_node.l_idx == 0) // is leaf
      {
        for (int i = 0; i < cur_node.count; ++i)
        {
          // compute the actual distance
          const float *point = m_points_data.data() + (cur_node.start_index + i) * m_dim;
          float dist = distance(query, point);

          if (dist < max_dist)
          {
            bool finish = callback(dist, m_original_ids[cur_node.start_index + i],
                                   m_points[cur_node.start_index + i], point);
            count++;
            if (finish)
              return count;
          }
        }
      }
      else
      {
        const Node &left_node = m_nodes[cur_node.l_idx];
        float left_center_dist = distance(query, m_centroids_data.data() + left_node.centroid_index * m_dim);
        if (left_center_dist < left_node.radius + max_dist)
        {
          stack[top] = cur_node.l_idx;
          top++;
        }

        const Node &right_node = m_nodes[cur_node.r_idx];
        float right_center_dist = distance(query, m_centroids_data.data() + right_node.centroid_index * m_dim);
        if (right_center_dist < right_node.radius + max_dist)
        {
          stack[top] = cur_node.r_idx;
          top++;
        }
      }
    }

    return count;
  }

  float BallTreeL2::distance_sqr(const float *a, const float *b) const
  {
    float d = 0;
    for (int i = 0; i < m_dim; ++i)
      d += (a[i] - b[i]) * (a[i] - b[i]);
    
    return d;
  }

  float BallTreeL2::distance_sqr_trunc(const float *a, const float *b, float max_dist_sq) const
  {
    float d = 0;
    for (int i = 0; i < m_dim && d <= max_dist_sq; i+=16)
    {
      for (int j=0; j<std::min(16u, m_dim-i); j++)
        d += (a[i+j] - b[i+j]) * (a[i+j] - b[i+j]);
    }
    
    return d;
  }

  void BallTreeL2::build(const Dataset &dataset, int max_leaf_size)
  {
    m_dim = dataset.dim;

    m_points_data.clear();
    m_points.clear();
    m_original_ids.clear();
    m_nodes.clear();
    m_centroids_data.clear();

    m_points_data.reserve(dataset.all_points.size());
    m_points.reserve(dataset.data_points.size());
    m_original_ids.reserve(dataset.data_points.size());
    m_nodes.reserve(dataset.data_points.size());
    m_centroids_data.reserve(dataset.all_points.size());

    m_index.resize(dataset.data_points.size());
    for (int i = 0; i < dataset.data_points.size(); ++i)
      m_index[i] = i;

    int max_threads = omp_get_max_threads();
    m_tmp_vec.resize(m_dim*max_threads, 0);

    m_dataset = &dataset;

    #pragma omp parallel
    #pragma omp single
    {
      build_rec(max_leaf_size, dataset.data_points.size(), 0);
    }
  
    m_points_data.shrink_to_fit();
    m_points.shrink_to_fit();
    m_original_ids.shrink_to_fit();
    m_nodes.shrink_to_fit();
    m_centroids_data.shrink_to_fit();
  }

  int BallTreeL2::build_rec(int max_leaf_size, int n, int offset)
  {
    const Dataset &dataset = *m_dataset;
    int *index = m_index.data() + offset;
    float *tmp_vec = m_tmp_vec.data();

    int cur_thread_id = omp_get_thread_num();
    unsigned cur_node_id = 0;

    #pragma omp critical
    {
      assert(m_nodes.capacity() >= m_nodes.size() + 1);
      if (m_nodes.capacity() <= m_nodes.size() + 1)
        printf("BallTreeL2::build_rec: increase m_nodes.capacity()\n");
      assert(m_centroids_data.capacity() >= m_centroids_data.size() + m_dim);
      if (m_centroids_data.capacity() <= m_centroids_data.size() + m_dim)
        printf("BallTreeL2::build_rec: increase m_centroids_data.capacity()\n");
      
      cur_node_id = m_nodes.size();
      m_nodes.emplace_back();
      m_centroids_data.resize(m_centroids_data.size() + m_dim);
    }

    if (n <= max_leaf_size)
    {
      // build leaf node
      unsigned start_id = 0;

      #pragma omp critical
      {
        start_id = m_points.size();
        m_points.resize(start_id + n);
        m_original_ids.resize(start_id + n);
        m_points_data.resize(m_points_data.size() + n * m_dim);
      }
      for (int i = 0; i < n; ++i)
      {
        const DataPoint &dp = dataset.data_points[index[i]];
        m_points[start_id + i] = dp;
        m_original_ids[start_id + i] = index[i];
        memcpy(m_points_data.data() + (start_id + i) * m_dim, 
               dataset.all_points.data() + dp.data_offset, 
               m_dim * sizeof(float));
      }

      //We can find more optimal bounding sphere with center that is not a centroid
      //e.g. check CGAL Min_sphere_of_spheres_d_impl.h
      float c_mult = 1.0f/n;
      for (int j = 0; j < m_dim; ++j)
        m_centroids_data[cur_node_id*m_dim + j] = 0;
      for (int i = 0; i < n; ++i)
      {
        const float *p = dataset.all_points.data() + dataset.data_points[index[i]].data_offset;
        for (int j = 0; j < m_dim; ++j)
          m_centroids_data[cur_node_id*m_dim + j] += c_mult*p[j];
      }

      float max_dist_sq = 0;
      for (int i = 0; i < n; ++i)
      {
        const float *p = dataset.all_points.data() + dataset.data_points[index[i]].data_offset;
        float dist_sq = distance_sqr(m_centroids_data.data() + cur_node_id * m_dim, p);
        max_dist_sq = std::max(max_dist_sq, dist_sq);
      }

      m_nodes[cur_node_id].start_index = start_id;
      m_nodes[cur_node_id].count = n;
      m_nodes[cur_node_id].l_idx = 0;
      m_nodes[cur_node_id].r_idx = 0;
      m_nodes[cur_node_id].centroid_index = cur_node_id;
      m_nodes[cur_node_id].radius = sqrtf(max_dist_sq);
    }
    else
    {
      int left = 0, right = n - 1, cnt = 0;
      do
      {
        // build internal node
        int x_p = rand() % n;
        int l_p = find_furthest_id(dataset, x_p, n, index);
        int r_p = find_furthest_id(dataset, l_p, n, index);
        //assert(l_p != r_p); Not a problem?

        // note: we use l_p and r_p as two pivots
        const float *l_pivot = dataset.all_points.data() + dataset.data_points[index[l_p]].data_offset;
        const float *r_pivot = dataset.all_points.data() + dataset.data_points[index[r_p]].data_offset;
        float l_sqr = 0.0f, r_sqr = 0.0f;
        for (int i = 0; i < m_dim; ++i)
        {
          float l_v = l_pivot[i], r_v = r_pivot[i];
          tmp_vec[cur_thread_id * m_dim + i] = r_v - l_v;
          l_sqr += l_v*l_v;
          r_sqr += r_v*r_v;
        }
        float b = 0.5f * (l_sqr - r_sqr);

        left = 0, right = n - 1;
        while (left <= right)
        {
          const float *x = dataset.all_points.data() + dataset.data_points[index[left]].data_offset;
          float inner_product = 0;
          for (int i = 0; i < m_dim; ++i)
            inner_product += tmp_vec[cur_thread_id * m_dim + i] * x[i];
          float val = inner_product + b;
          if (val < 0.0f)
            ++left;
          else
          {
            std::swap(index[left], index[right]);
            --right;
          }
        }
        ++cnt;
      } while ((left <= 0 || left >= n) && cnt <= 3); // ensure split into two parts
      if (cnt > 3)
        left = n / 2;

      int l_idx = -1, r_idx = -1;

      const int parallel_threshold = 8*max_leaf_size; //some empiric value to avoid too much overhead
      if (left > parallel_threshold)
      {
#ifndef DISABLE_OMP_TASKS
        #pragma omp task shared(l_idx)
#endif
        l_idx = build_rec(max_leaf_size, left, offset);
      }
      else
      {
        l_idx = build_rec(max_leaf_size, left, offset);
      }

      if (n - left > parallel_threshold)
      {
#ifndef DISABLE_OMP_TASKS
        #pragma omp task shared(r_idx)
#endif
        r_idx = build_rec(max_leaf_size, n - left, offset + left);
      }
      else
      {
        r_idx = build_rec(max_leaf_size, n - left, offset + left);
      }
#ifndef DISABLE_OMP_TASKS
      #pragma omp taskwait
#endif
      assert(l_idx != -1 && r_idx != -1);

      const Node &l_node = m_nodes[l_idx];
      const Node &r_node = m_nodes[r_idx];

      float c_mult_1 = l_node.count/(float)(l_node.count + r_node.count);
      float c_mult_2 = r_node.count/(float)(l_node.count + r_node.count);
      for (int i = 0; i < n; ++i)
      {
        for (int j = 0; j < m_dim; ++j)
          m_centroids_data[cur_node_id*m_dim + j] = c_mult_1*m_centroids_data[l_node.centroid_index*m_dim + j] + 
                                                    c_mult_2*m_centroids_data[r_node.centroid_index*m_dim + j];
      }

      float max_dist_sq = 0;
      for (int i = 0; i < n; ++i)
      {
        const float *p = dataset.all_points.data() + dataset.data_points[index[i]].data_offset;
        float dist_sq = distance_sqr(m_centroids_data.data() + cur_node_id * m_dim, p);
        max_dist_sq = std::max(max_dist_sq, dist_sq);
      }

      m_nodes[cur_node_id].start_index = 0xFFFFFFFFu;
      m_nodes[cur_node_id].count = l_node.count + r_node.count;
      m_nodes[cur_node_id].l_idx = l_idx;
      m_nodes[cur_node_id].r_idx = r_idx;
      m_nodes[cur_node_id].centroid_index = cur_node_id;
      m_nodes[cur_node_id].radius = sqrtf(max_dist_sq);
    }

    return cur_node_id;
  }

  int BallTreeL2::find_furthest_id(const Dataset &dataset, int id_from, int n, int *index)
  {
    int far_id = -1;
    float far_dist = -1.0f;

    const float *a = dataset.all_points.data() + dataset.data_points[index[id_from]].data_offset;
    for (int i = 0; i < n; ++i)
    {
      if (i == id_from)
        continue;

      const float *b = dataset.all_points.data() + dataset.data_points[index[i]].data_offset;
      float dist_sq = distance_sqr(a,b);
      if (far_dist < dist_sq)
      {
        far_dist = dist_sq;
        far_id = i;
      }
    }
    return far_id;
  }

  const float *BallTreeL2::get_closest_point(const float *query, float max_dist, float *dist_to_nearest) const
  {
    constexpr unsigned MAX_STACK_SIZE = 128;
    std::array<unsigned, MAX_STACK_SIZE> stack;
    int top = 0;
    stack[top] = 0;
    top = 1;
    const float *cur_nearest = nullptr;
    float nearest_dist_sq = max_dist * max_dist;
    while (top > 0)
    {
      const Node &cur_node = m_nodes[stack[top - 1]];
      top--;

      if (cur_node.l_idx == 0) // is leaf
      {
        for (int i = 0; i < cur_node.count; ++i)
        {
          // compute the actual distance
          const float *point = m_points_data.data() + (cur_node.start_index + i) * m_dim;
          float dist_sq = distance_sqr(query, point);

          if (dist_sq < nearest_dist_sq)
          {
            nearest_dist_sq = dist_sq;
            cur_nearest = point;
          }
        }
      }
      else
      {
        const Node &left_node = m_nodes[cur_node.l_idx];
        float left_center_dist_sq = distance_sqr(query, m_centroids_data.data() + left_node.centroid_index * m_dim);
        if (sqrtf(left_center_dist_sq) < left_node.radius + sqrtf(nearest_dist_sq))
        {
          stack[top] = cur_node.l_idx;
          top++;
        }

        const Node &right_node = m_nodes[cur_node.r_idx];
        float right_center_dist_sq = distance_sqr(query, m_centroids_data.data() + right_node.centroid_index * m_dim);
        if (sqrtf(right_center_dist_sq) < right_node.radius + sqrtf(nearest_dist_sq))
        {
          stack[top] = cur_node.r_idx;
          top++;
        }
      }
    }

    if (cur_nearest && dist_to_nearest)
    {
      *dist_to_nearest = sqrtf(nearest_dist_sq);
    }
    return cur_nearest;
  }

  int BallTreeL2::scan_near(const float *query, float max_dist, ScanFunction callback) const
  {
    constexpr unsigned MAX_STACK_SIZE = 128;
    std::array<unsigned, MAX_STACK_SIZE> stack;
    int top = 0;
    stack[top] = 0;
    top = 1;
    float max_dist_sq = max_dist * max_dist;
    int count = 0;

    while (top > 0)
    {
      const Node &cur_node = m_nodes[stack[top - 1]];
      top--;

      if (cur_node.l_idx == 0) // is leaf
      {
        for (int i = 0; i < cur_node.count; ++i)
        {
          // compute the actual distance
          const float *point = m_points_data.data() + (cur_node.start_index + i) * m_dim;
          //printf("leaf %d N %d\n", cur_node.start_index, i);
          float dist_sq = distance_sqr_trunc(query, point, max_dist_sq);

          if (dist_sq < max_dist_sq)
          {
            bool finish = callback(sqrtf(dist_sq), m_original_ids[cur_node.start_index + i], m_points[cur_node.start_index + i], point);
            count++;
            if (finish)
              return count;
          }
        }
      }
      else
      {
        const Node &left_node = m_nodes[cur_node.l_idx];
        //printf("left (%d) centroid %d\n", cur_node.l_idx, left_node.centroid_index);
        float left_limit_sq = (left_node.radius + max_dist) * (left_node.radius + max_dist);
        float left_center_dist_sq = distance_sqr_trunc(query, m_centroids_data.data() + left_node.centroid_index * m_dim, left_limit_sq);
        if (left_center_dist_sq < left_limit_sq)
        {
          stack[top] = cur_node.l_idx;
          top++;
        }

        const Node &right_node = m_nodes[cur_node.r_idx];
        //printf("right (%d) centroid %d\n", cur_node.r_idx, right_node.centroid_index);
        float right_limit_sq = (right_node.radius + max_dist) * (right_node.radius + max_dist);
        float right_center_dist_sq = distance_sqr_trunc(query, m_centroids_data.data() + right_node.centroid_index * m_dim, right_limit_sq);
        if (right_center_dist_sq < right_limit_sq)
        {
          stack[top] = cur_node.r_idx;
          top++;
        }
      }
    }

    return count;
  }
}