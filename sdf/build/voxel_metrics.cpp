#include "LiteMath.h"
#include <iostream>
#include <algorithm>
#include <cmath>

#include "voxel_metrics.h"

namespace sim_metric
{
  static float trilinear_interp(const float values[8], LiteMath::float3 dp)
  {
    return (1-dp.x)*(1-dp.y)*(1-dp.z)*values[0] + 
           (1-dp.x)*(1-dp.y)*(  dp.z)*values[1] + 
           (1-dp.x)*(  dp.y)*(1-dp.z)*values[2] + 
           (1-dp.x)*(  dp.y)*(  dp.z)*values[3] + 
           (  dp.x)*(1-dp.y)*(1-dp.z)*values[4] + 
           (  dp.x)*(1-dp.y)*(  dp.z)*values[5] + 
           (  dp.x)*(  dp.y)*(1-dp.z)*values[6] + 
           (  dp.x)*(  dp.y)*(  dp.z)*values[7];
  }

  void sample_voxel_surface(const float voxel[8], std::vector<LiteMath::float3> &points, int &size, float step)
  {
    // if (points.size() < (1 / step + 1) * (1 / step + 1) * 3)
    //   points.resize((1 / step + 1) * (1 / step + 1) * 3);
    size = 0;
    // int cnt = 0;
    for (float i = 0.0f; i <= 1.0f; i += step)
    {
      for (float j = 0.0f; j <= 1.0f; j += step)
      {
        for (int k = 0; k < 3; ++k)
        {
          float k0 = 0.0f, k1 = 1.0f;
          // printf("%d - %f %f %d\n", cnt++, i, j, k);
          LiteMath::float3 pos1, pos2, resp;
          pos1[(0 + k) % 3] = i;  pos2[(0 + k) % 3] = i; resp[(0 + k) % 3] = i;
          pos1[(1 + k) % 3] = j;  pos2[(1 + k) % 3] = j; resp[(1 + k) % 3] = j;
          pos1[(2 + k) % 3] = k0; pos2[(2 + k) % 3] = k1;
          float res1 = trilinear_interp(voxel, pos1), res2 = trilinear_interp(voxel, pos2);
          if (res1 * res2 <= 0 && std::abs(res1 - res2) > 1e-6)
          {
            resp[(2 + k) % 3] = res1 / (res1 - res2);
            // if (points.size() <= size) 
            // {
            //   printf("%d %d\n", (int)points.size(), size);
            //   int *data = NULL;
            //   size = data[0];
            // }
            points[size++] = resp;
          }
        }
      }
    }
    return;
  }

  std::vector<LiteMath::float3> sample_voxel_surface(const float voxel[8], float step)
  {
    std::vector<LiteMath::float3> points;
    points.reserve((1 / step + 1) * 3);
    for (float i = 0.0f; i <= 1.0f; i += step)
    {
      for (float j = 0.0f; j <= 1.0f; j += step)
      {
        for (int k = 0; k < 3; ++k)
        {
          float k0 = 0.0f, k1 = 1.0f;
          LiteMath::float3 pos1, pos2, resp;
          pos1[(0 + k) % 3] = i;  pos2[(0 + k) % 3] = i; resp[(0 + k) % 3] = i;
          pos1[(1 + k) % 3] = j;  pos2[(1 + k) % 3] = j; resp[(1 + k) % 3] = j;
          pos1[(2 + k) % 3] = k0; pos2[(2 + k) % 3] = k1;
          float res1 = trilinear_interp(voxel, pos1), res2 = trilinear_interp(voxel, pos2);
          if (res1 * res2 <= 0 && std::abs(res1 - res2) > 1e-6)
          {
            resp[(2 + k) % 3] = res1 / (res1 - res2);
            points.push_back(resp);
          }
        }
      }
    }
    return points;
  }

  float chamfer_distance(const LiteMath::float3* set1, int count1, const LiteMath::float3* set2, int count2)
  {
    float sum = 0.0f;
    if (count1 <= 0 || count2 <= 0) return 0;

    for (int i = 0; i < count1; ++i)
    {
      float minDist = LiteMath::length(set1[i] - set2[0]);
      for (int j = 0; j < count2; ++j)
      {
        float d = LiteMath::length(set1[i] - set2[j]);
        if (d < minDist) minDist = d;
      }
      sum += minDist;
    }

    for (int j = 0; j < count2; ++j)
    {
      float minDist = LiteMath::length(set2[j] - set1[0]);
      for (int i = 0; i < count1; ++i)
      {
        float d = LiteMath::length(set2[j] - set1[i]);
        if (d < minDist) minDist = d;
      }
      sum += minDist;
    }
    return sum / (count1 + count2);
  }

  float hausdorff_distance(const LiteMath::float3* set1, int count1, const LiteMath::float3* set2, int count2)
  {
    if (count1 <= 0 || count2 <= 0) return 0;

    float maxMinAtoB = 0.0f;

    for (int i = 0; i < count1; ++i)
    {
      float minDist = LiteMath::length(set1[i] - set2[0]);
      for (int j = 0; j < count2; ++j)
      {
        float d = LiteMath::length(set1[i] - set2[j]);
        if (d < minDist) minDist = d;
      }
      if (minDist > maxMinAtoB) maxMinAtoB = minDist;
    }

    float maxMinBtoA = 0.0f;

    for (int j = 0; j < count2; ++j)
    {
      float minDist = LiteMath::length(set2[j] - set1[0]);
      for (int i = 0; i < count1; ++i)
      {
        float d = LiteMath::length(set2[j] - set1[i]);
        if (d < minDist) minDist = d;
      }
      if (minDist > maxMinBtoA) maxMinBtoA = minDist;
    }
    return std::max(maxMinAtoB, maxMinBtoA);
  }

  float chamfer_metric(const float voxel1[8], const float voxel2[8], float step)
  {
    std::vector<LiteMath::float3> points1 = sample_voxel_surface(voxel1, step);
    std::vector<LiteMath::float3> points2 = sample_voxel_surface(voxel2, step);
    return chamfer_distance(points1.data(), (int)points1.size(), points2.data(), (int)points2.size());
  }

  float hausdorff_metric(const float voxel1[8], const float voxel2[8], float step)
  {
    std::vector<LiteMath::float3> points1 = sample_voxel_surface(voxel1, step);
    std::vector<LiteMath::float3> points2 = sample_voxel_surface(voxel2, step);
    return hausdorff_distance(points1.data(), (int)points1.size(), points2.data(), (int)points2.size());
  }
};