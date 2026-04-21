#pragma once
#include "LiteMath.h"

using LiteMath::uint3;
using LiteMath::int3;
using LiteMath::float3;

static inline float solve_eikonal(LiteMath::float3 axes_mins, float grid_spacing)
{
  LiteMath::float3 m = axes_mins;
  float  h = grid_spacing;

  // Sort m in ascending order
  if (m.x > m.y)
    std::swap(m.x, m.y);
  if (m.x > m.z)
    std::swap(m.x, m.z);
  if (m.y > m.z)
    std::swap(m.y, m.z);

  // From: https://github.com/scikit-fmm/scikit-fmm/blob/master/skfmm/distance_marcher.cpp
  float dist_new = m.x + h;
  if (dist_new > m.y)
  {
    float m_sum = m.x + m.y;
    dist_new = (m_sum + std::sqrt(2 * h * h - (m.x - m.y) * (m.x - m.y))) * 0.5f;

    if (dist_new > m.z)
    {
      m_sum += m.z;
      dist_new = (m_sum + std::sqrt(m_sum * m_sum - 3.f * (m.x * m.x + m.y * m.y + m.z * m.z - h * h))) / 3.f;
    }
  }
  return dist_new;
}

template <int N, bool freeze_surface> 
void redistance_fast_sweep_fixed_small_brick(float *dist_in, float grid_spacing, float *dist_out, bool *updated_values, bool *frozen)
{
  const uint3 size_in = uint3(N, N, N);
  const uint32_t N_in = size_in.x * size_in.y * size_in.z;
  const int3 axes_offsets{(int) (size_in.y * size_in.z), (int) size_in.z, 1};

  // Init
  const float big_active_const = 99999.f;
  const float Infin = 1e38f;

  for (int i = 0; i < size_in.x; ++i)
    for (int j = 0; j < size_in.y; ++j)
      for (int k = 0; k < size_in.z; ++k)
      {
        int3 ind_in = { i, j, k };
        uint32_t ind_in_lin = ind_in.x * axes_offsets.x + ind_in.y * axes_offsets.y + ind_in.z;

        frozen[ind_in_lin] = false;

        // Initialize with a big value
        dist_out[ind_in_lin] = big_active_const;

        float dist_val = dist_in[ind_in_lin];

        // Surface itself is not moved
        if (dist_val == 0.f)
        {
          frozen[ind_in_lin] = true;
          dist_out[ind_in_lin] = 0.f;
        }

        // Also freeze if a surface is between this node and any of its neighbours
        if (!frozen[ind_in_lin])
        {
          float3 ldistance{0.f};
          bool close_to_surface = false;

          for (int dim = 0; dim < 3; ++dim)
            for (int offset_sign = -1; offset_sign < 2; offset_sign += 2)
            {
              int3 neighbour = ind_in;
              neighbour[dim] += offset_sign;

              if (neighbour.x >= 0 && neighbour.x < size_in.x &&
                  neighbour.y >= 0 && neighbour.y < size_in.y &&
                  neighbour.z >= 0 && neighbour.z < size_in.z)
              {
                float n_val = dist_in[neighbour.x * axes_offsets.x +
                                      neighbour.y * axes_offsets.y +
                                      neighbour.z];
                if (n_val * dist_val < 0)
                {
                  // calculate the distance to the surface
                  close_to_surface = true;
                  float d = grid_spacing * dist_val / (dist_val - n_val);
                  if (ldistance[dim] == 0.f || d < ldistance[dim])
                    ldistance[dim] = d;

                  frozen[ind_in_lin] = true;
                }
              }
            }

          if (close_to_surface)
          {
            float dsum = 0;
            for (int dim=0; dim < 3; dim++)
              if (ldistance[dim] > 0.f)
                dsum += 1.f / ldistance[dim] / ldistance[dim];

            if (!freeze_surface)
              dist_val = dist_val < 0.f ? -sqrt(1.f / dsum) : sqrt(1.f / dsum);
            dist_out[ind_in_lin] = dist_val;
          }
        }
      }
  

  // Update loop
  const int max_level = size_in.x + size_in.y + size_in.z - 3;
  bool has_updatable_values = true;

  while (has_updatable_values)
  {
    has_updatable_values = false;
    for (int ordering = 0; ordering < 8; ++ordering)
    {
      int start, end, step;
      if (ordering == 1 || ordering == 4 || ordering == 6 || ordering == 7)
      {
        start = max_level;
        end   = -1;
        step  = -1;
      }
      else
      {
        start = 0;
        end   = max_level + 1;
        step  = 1;
      }

      for (int level = start; level != end; level += step)
      {
        int xs = std::max(0, level - int(size_in.y + size_in.z - 2));
        int ys = std::max(0, level - int(size_in.x + size_in.z - 2));
        int xe = std::min((int) size_in.x - 1, level);
        int ye = std::min((int) size_in.y - 1, level);
        int xr = xe - xs + 1;
        int yr = ye - ys + 1;

        for (int i = xs; i < xs + xr; ++i)
          for (int j = ys; j < ys + yr; ++j)
          {
            int k = level - i - j;
            if (k < 0 || k >= size_in.z) continue; // only k check is needed

            int3 ind_in{ i, j, k };
            uint32_t ind_in_lin = ind_in.x * axes_offsets.x + ind_in.y * axes_offsets.y + ind_in.z;

            if (frozen[ind_in_lin]) continue;

            float3 axes_mins{big_active_const, big_active_const, big_active_const};

            for (int dim = 0; dim < 3; ++dim)
            {
              if (ind_in[dim] > 0 && ind_in[dim] < size_in[dim] - 1)
              {
                axes_mins[dim] = dist_out[ind_in_lin - axes_offsets[dim]];
                float dist_tmp = dist_out[ind_in_lin + axes_offsets[dim]];
                if (std::abs(dist_tmp) < std::abs(axes_mins[dim]))
                  axes_mins[dim] = dist_tmp;
              }
              else if (ind_in[dim] >  0 && ind_in[dim] == size_in[dim] - 1)
                axes_mins[dim] = dist_out[ind_in_lin - axes_offsets[dim]];
              else if (ind_in[dim] == 0 && ind_in[dim] <  size_in[dim] - 1)
                axes_mins[dim] = dist_out[ind_in_lin + axes_offsets[dim]];
            }

            bool pos_part = axes_mins.x >= 0.f && axes_mins.y >= 0.f && axes_mins.z >= 0.f;
            axes_mins = abs(axes_mins);

            if (axes_mins.x < big_active_const || axes_mins.y < big_active_const || axes_mins.z < big_active_const)
            {

              float val_before = dist_out[ind_in_lin];
              if (pos_part)
              {
                dist_out[ind_in_lin] = std::min( solve_eikonal(axes_mins, grid_spacing), val_before);
              }
              else
              {
                val_before = -std::abs(val_before);
                dist_out[ind_in_lin] = std::max(-solve_eikonal(axes_mins, grid_spacing), val_before);
              }
              if (dist_out[ind_in_lin] != val_before)
                updated_values[ind_in_lin] = true;
            }
          }
      }
    }

    for (uint32_t upd_i = 0u; upd_i < N_in; ++upd_i)
    {
      bool noninit_value = big_active_const == dist_out[upd_i];
      if (noninit_value)
        has_updatable_values = true;
      frozen[upd_i] |= (!noninit_value && !updated_values[upd_i]);
      updated_values[upd_i] = false;
    }
  }
}

void redistance_fast_sweep(float *dist_in, LiteMath::uint3 size_in, bool freeze_surface, float grid_spacing, uint32_t num_iters, 
                           float *dist_bord, bool *updated_values, bool *frozen, float redist_thr = 0.0f);
void calculate_eikonal_mean(const LiteMath::uint3 &size_in, const LiteMath::int3 &axes_offsets, float *dist_in, float grid_spacing);
