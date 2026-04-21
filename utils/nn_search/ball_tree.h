#pragma once

#include <cstdint>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include <immintrin.h>

#include "LiteMath.h"
#include "near_neighbor_common.h"
#include "utils/common/constexpr_for.h"

namespace nn_search
{
  // Ball tree acceleration structure for arbitrary metric
  class BallTree : public INNSearchAS
  {
  public:
    struct Node
    {
      unsigned l_idx, r_idx; // left and right children, = 0 <=> leaf
      unsigned centroid_index;
      unsigned start_index;
      unsigned count;
      float radius;
    };

    BallTree() = default;
    BallTree(IDistanceFunction *dist_func);

    void build(const Dataset &dataset, int max_leaf_size) override;
    const float *get_closest_point(const float *query, float max_dist, float *dist_to_nearest = nullptr) const override;
    int scan_near(const float *query, float max_dist, ScanFunction callback) const override;
  
  protected:  
    int build_rec(int max_leaf_size, int n, int offset);
    int find_furthest_id(const Dataset &dataset, int id_from, int n, int *index);
    virtual float distance(const float *a, const float *b) const { return m_dist_func->calc(m_dim, a, b); }

    size_t m_dim; // dimension count of data
    std::vector<Node> m_nodes;
    aligned_vector_f m_centroids_data; //m_dim * m_nodes.size();
    
    std::vector<DataPoint> m_points;
    std::vector<unsigned>  m_original_ids; //m_points.size();
    aligned_vector_f m_points_data; //m_dim * m_points.size();
    const Dataset *m_dataset;
    std::vector<float> m_tmp_vec;
    
    std::vector<int> m_index;
    IDistanceFunction *m_dist_func = nullptr;
  };

  // Ball tree acceleration structure with L1 (Manhattan) distance,
  // slightly faster than generic one
  class BallTreeL1 : public BallTree
  {
  public:
    virtual float distance(const float *a, const float *b) const override
    {
      float d = 0;
      for (int i = 0; i < m_dim; ++i)
        d += std::abs(a[i] - b[i]);
      return d;
    }
  };

  // Ball tree acceleration structure with LInf (Chebyshev) distance,
  // slightly faster than generic one
  class BallTreeLInf : public BallTree
  {
  public:
    virtual float distance(const float *a, const float *b) const override
    {
      float d = 0;
      for (int i = 0; i < m_dim; ++i)
        d = std::max(d, std::abs(a[i] - b[i]));
      return d;
    }
  };

  // Ball tree acceleration structure with L2 (Euclidean) distance,
  // faster than generic one
  class BallTreeL2 : public INNSearchAS
  {
  public:
    struct Node
    {
      unsigned l_idx, r_idx; // left and right children, = 0 <=> leaf
      unsigned centroid_index;
      unsigned start_index;
      unsigned count;
      float radius;
    };

    void build(const Dataset &dataset, int max_leaf_size) override;
    const float *get_closest_point(const float *query, float max_dist, float *dist_to_nearest = nullptr) const override;
    int scan_near(const float *query, float max_dist, ScanFunction callback) const override;
  
  protected:  
    int build_rec(int max_leaf_size, int n, int offset);
    int find_furthest_id(const Dataset &dataset, int id_from, int n, int *index);
    virtual float distance_sqr(const float *a, const float *b) const;
    float distance_sqr_trunc(const float *a, const float *b, float max_dist_sq) const;

    uint32_t m_dim; // dimension count of data
    std::vector<Node> m_nodes;
    aligned_vector_f m_centroids_data; //m_dim * m_nodes.size();
    
    std::vector<DataPoint> m_points;
    std::vector<unsigned>  m_original_ids; //m_points.size();
    aligned_vector_f m_points_data; //m_dim * m_points.size();
    const Dataset *m_dataset;
    std::vector<float> m_tmp_vec;
    
    std::vector<int> m_index;
  };

  // Ball tree acceleration structure with L2 (Euclidean) distance
  // Uses AVX2 vectorization
  class BallTreeFDL2 : public BallTreeL2
  {
  private:
    static float sum8(__m256 x)
    {
      // x = ( x7, x6, x5, x4, x3, x2, x1, x0 )
    #ifdef __AVX2__
      // hiQuad = ( x7, x6, x5, x4 )
      const __m128 hiQuad = _mm256_extractf128_ps(x, 1);
      // loQuad = ( x3, x2, x1, x0 )
      const __m128 loQuad = _mm256_castps256_ps128(x);
      // sumQuad = ( x3 + x7, x2 + x6, x1 + x5, x0 + x4 )
      const __m128 sumQuad = _mm_add_ps(loQuad, hiQuad);
      // loDual = ( -, -, x1 + x5, x0 + x4 )
      const __m128 loDual = sumQuad;
      // hiDual = ( -, -, x3 + x7, x2 + x6 )
      const __m128 hiDual = _mm_movehl_ps(sumQuad, sumQuad);
      // sumDual = ( -, -, x1 + x3 + x5 + x7, x0 + x2 + x4 + x6 )
      const __m128 sumDual = _mm_add_ps(loDual, hiDual);
      // lo = ( -, -, -, x0 + x2 + x4 + x6 )
      const __m128 lo = sumDual;
      // hi = ( -, -, -, x1 + x3 + x5 + x7 )
      const __m128 hi = _mm_shuffle_ps(sumDual, sumDual, 0x1);
      // sum = ( -, -, -, x0 + x1 + x2 + x3 + x4 + x5 + x6 + x7 )
      const __m128 sum = _mm_add_ss(lo, hi);
      return _mm_cvtss_f32(sum);
    #else
      return 0;
    #endif
    }
    virtual float distance_sqr(const float *a, const float *b) const override
    {
      //printf("%d %lx %lx\n", m_dim, (uint64_t)a, (uint64_t)b);
      float d = 0;
      #ifdef __AVX2__
      assert(m_dim % 8 == 0);
      for (int i = 0; i < m_dim / 8; ++i)
      {
        __m256 v1 = _mm256_load_ps(a + i * 8);
        __m256 v2 = _mm256_load_ps(b + i * 8);
        __m256 diff = _mm256_sub_ps(v1, v2);
        __m256 diff_sq = _mm256_mul_ps(diff, diff);
        d += sum8(diff_sq);
      }
      #else
      for (int i = 0; i < m_dim; ++i)
        d += (a[i] - b[i]) * (a[i] - b[i]);
      #endif

      return d;
    }
  };
}