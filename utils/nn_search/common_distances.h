#pragma once
#include "near_neighbor_common.h"

namespace nn_search
{
  class L2Distance : public IDistanceFunction
  {
  public:
    virtual Type getType() override { return Type::L2; }
    virtual float calc(uint32_t dim, const float *a, const float *b) override 
    { 
      float d = 0;
      for (int i = 0; i < dim; ++i)
        d += (a[i] - b[i]) * (a[i] - b[i]);
      return sqrtf(d);
    }
    virtual float calcTruncated(uint32_t dim, const float *a, const float *b, float max_dist) override\
    {
      float dm_sqr = max_dist*max_dist;
      float d = 0;
      for (int i = 0; i < dim && d < dm_sqr; ++i)
        d += (a[i] - b[i]) * (a[i] - b[i]);
      return sqrtf(d);
    }
  };

  class L1Distance : public IDistanceFunction
  {
  public:
    virtual Type getType() override { return Type::L1; }
    virtual float calc(uint32_t dim, const float *a, const float *b) override 
    { 
      float d = 0;
      for (int i = 0; i < dim; ++i)
        d += fabsf(a[i] - b[i]);
      return d;
    }
    virtual float calcTruncated(uint32_t dim, const float *a, const float *b, float max_dist) override\
    {
      float dm = max_dist;
      float d = 0;
      for (int i = 0; i < dim && d < dm; ++i)
        d += fabsf(a[i] - b[i]);
      return d;
    }
  };

  class LInfDistance : public IDistanceFunction
  {
  public:
    virtual Type getType() override { return Type::LInf; }
    virtual float calc(uint32_t dim, const float *a, const float *b) override 
    { 
      float d = 0;
      for (int i = 0; i < dim; ++i)
        d = std::max(d, fabsf(a[i] - b[i]));
      return d;
    }
    virtual float calcTruncated(uint32_t dim, const float *a, const float *b, float max_dist) override\
    {
      float dm = max_dist;
      float d = 0;
      for (int i = 0; i < dim && d < dm; ++i)
        d = std::max(d, fabsf(a[i] - b[i]));
      return d;
    }
  };
}