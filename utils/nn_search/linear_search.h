#pragma once
#include "near_neighbor_common.h"

namespace nn_search
{
  // Linear search for arbitrary distance
  class LinearSearchAS : public INNSearchAS
  {
  public:
    LinearSearchAS(IDistanceFunction *dist_func) : m_dist_func(dist_func) {}
    void build(const Dataset &dataset, int max_leaf_size) override
    {
      m_dim = dataset.dim;
      m_dataset = dataset;
    }
    const float *get_closest_point(const float *query, float max_dist, float *dist_to_nearest) const override
    {
      const float *closest_point = nullptr;
      float closest_dist = max_dist;
      for (size_t i = 0; i < m_dataset.data_points.size(); ++i)
      {
        const float *p = m_dataset.all_points.data() + m_dataset.data_points[i].data_offset;
        float dist = m_dist_func->calcTruncated(m_dim, query, p, closest_dist);

        if (dist < closest_dist)
        {
          closest_dist = dist;
          closest_point = p;
        }
      }

      if (dist_to_nearest && closest_point)
        *dist_to_nearest = closest_dist;

      return closest_point;
    }

    int scan_near(const float *query, float max_dist, ScanFunction callback) const override
    {
      int scan_count = 0;
      for (size_t i = 0; i < m_dataset.data_points.size(); ++i)
      {
        const float *p = m_dataset.all_points.data() + m_dataset.data_points[i].data_offset;
        float dist = m_dist_func->calcTruncated(m_dim, query, p, max_dist);
        
        if (dist < max_dist)
        {
          bool finish = callback(dist, i, m_dataset.data_points[i], p);
          ++scan_count;
          if (finish)
            return scan_count;
        }
      }

      return scan_count;
    }

  private:
    IDistanceFunction *m_dist_func;
    size_t m_dim;
    Dataset m_dataset;
  };

  // Linear search with L2 (Euclidean) distance
  class LinearSearchL2AS : public INNSearchAS
  {
  public:
    void build(const Dataset &dataset, int max_leaf_size) override
    {
      m_dim = dataset.dim;
      m_dataset = dataset;
    }
    const float *get_closest_point(const float *query, float max_dist, float *dist_to_nearest) const override
    {
      const float *closest_point = nullptr;
      float closest_dist_sq = max_dist*max_dist;
      for (size_t i = 0; i < m_dataset.data_points.size(); ++i)
      {
        const float *p = m_dataset.all_points.data() + m_dataset.data_points[i].data_offset;
        float dist_sq = 0;
        for (size_t j = 0; j < m_dim; ++j)
          dist_sq += (query[j] - p[j]) * (query[j] - p[j]);

        //if (m_dataset.data_points[])

        if (dist_sq < closest_dist_sq)
        {
          closest_dist_sq = dist_sq;
          closest_point = p;
        }
      }

      if (dist_to_nearest && closest_point)
        *dist_to_nearest = sqrtf(closest_dist_sq);

      return closest_point;
    }

    int scan_near(const float *query, float max_dist, ScanFunction callback) const override
    {
      int scan_count = 0;
      float max_dist_sq = max_dist*max_dist;
      for (size_t i = 0; i < m_dataset.data_points.size(); ++i)
      {
        const float *p = m_dataset.all_points.data() + m_dataset.data_points[i].data_offset;
        
        float dist_sq = 0;
        for (size_t j = 0; j < m_dim && dist_sq < max_dist_sq; ++j)
          dist_sq += (query[j] - p[j]) * (query[j] - p[j]);
        
        if (dist_sq < max_dist_sq)
        {
          bool finish = callback(sqrtf(dist_sq), i, m_dataset.data_points[i], p);
          ++scan_count;
          if (finish)
            return scan_count;
        }
      }

      return scan_count;
    }

  private:
    size_t m_dim;
    Dataset m_dataset;
  };
}