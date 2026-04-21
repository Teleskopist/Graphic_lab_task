#include "redistancing.h"

#include <cstdio>

using LiteMath::uint3;
using LiteMath::int3;
using LiteMath::float3;

void calculate_eikonal_mean(const LiteMath::uint3 &size_in, const LiteMath::int3 &axes_offsets, float *dist_in, float grid_spacing)
{
  double eikonal_mean = 0.f;

  for (int i = 0; i < size_in.x; ++i)
  {
    for (int j = 0; j < size_in.y; ++j)
    {
      for (int k = 0; k < size_in.z; ++k)
      {
        int3 ind_in{i, j, k};
        uint32_t ind_in_lin = ind_in.x * axes_offsets.x + ind_in.y * axes_offsets.y + ind_in.z;

        float3 axes_mins{99999, 99999, 99999};

        for (int dim = 0; dim < 3; ++dim)
        {
          if (ind_in[dim] > 0 && ind_in[dim] < size_in[dim] - 1)
          {
            axes_mins[dim] = dist_in[ind_in_lin - axes_offsets[dim]];
            float dist_tmp = dist_in[ind_in_lin + axes_offsets[dim]];
            if (std::abs(dist_tmp) < std::abs(axes_mins[dim]))
              axes_mins[dim] = dist_tmp;
          }
          else if (ind_in[dim] > 0 && ind_in[dim] == size_in[dim] - 1)
            axes_mins[dim] = dist_in[ind_in_lin - axes_offsets[dim]];
          else if (ind_in[dim] == 0 && ind_in[dim] < size_in[dim] - 1)
            axes_mins[dim] = dist_in[ind_in_lin + axes_offsets[dim]];
        }

        float eik_val = ((dist_in[ind_in_lin] - axes_mins.x) * (dist_in[ind_in_lin] - axes_mins.x) +
                         (dist_in[ind_in_lin] - axes_mins.y) * (dist_in[ind_in_lin] - axes_mins.y) +
                         (dist_in[ind_in_lin] - axes_mins.z) * (dist_in[ind_in_lin] - axes_mins.z)) /
                        (grid_spacing * grid_spacing);
        //printf("%12.6f, ", eik_val);
        eikonal_mean += eik_val;
      }
      //printf("\n");
    }
    //printf("\n");
  }
  printf("Input eikonal mean = %f\n", eikonal_mean / (size_in.x * size_in.y * size_in.z));
}

void print_grid(float *dist_in, uint3 size_in)
{
  const int3 axes_offsets{(int) (size_in.y * size_in.z), (int) size_in.z, 1};
  for (int i = 0; i < size_in.x; ++i)
  {
    for (int j = 0; j < size_in.y; ++j)
    {
      for (int k = 0; k < size_in.z; ++k)
      {
        int3 ind_in = { i, j, k };
        uint32_t ind_in_lin = ind_in.x * axes_offsets.x + ind_in.y * axes_offsets.y + ind_in.z;
        printf("%12.6f, ", dist_in[ind_in_lin]);
      }
      printf("\n");
    }
    printf("\n");
  }
}

void redistance_fast_sweep(float *dist_in, uint3 size_in, bool freeze_surface, float grid_spacing, uint32_t num_iters, 
                           float *dist_bord, bool *updated_values, bool *frozen, float redist_thr)
{
  const uint32_t N_in = size_in.x * size_in.y * size_in.z;
  const int3 axes_offsets{(int) (size_in.y * size_in.z), (int) size_in.z, 1};

  //print_grid(dist_in, size_in);
  //printf("===================\n\n");
  //calculate_eikonal_mean(size_in, axes_offsets, dist_in, grid_spacing);

  // Init

  // auto t1 = std::chrono::high_resolution_clock::now();

  const float big_active_const = 99999.f;
  const float Infin = 1e38f;
  //#pragma omp parallel for collapse(3)
  for (int i = 0; i < size_in.x; ++i)
    for (int j = 0; j < size_in.y; ++j)
      for (int k = 0; k < size_in.z; ++k)
      {
        int3 ind_in = { i, j, k };
        uint32_t ind_in_lin = ind_in.x * axes_offsets.x + ind_in.y * axes_offsets.y + ind_in.z;

        frozen[ind_in_lin] = false;

        // Initialize with a big value
        dist_bord[ind_in_lin] = big_active_const;

        float dist_val = dist_in[ind_in_lin];

        // Surface itself is not moved
        if (std::abs(dist_val) <= redist_thr)
        {
          frozen[ind_in_lin] = true;
          dist_bord[ind_in_lin] = dist_val;
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
            dist_bord[ind_in_lin] = dist_val;
          }
        }
      }
  

  // Update loop
  // auto t2 = std::chrono::high_resolution_clock::now();

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

        //#pragma omp parallel for collapse(2)
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
                axes_mins[dim] = dist_bord[ind_in_lin - axes_offsets[dim]];
                float dist_tmp = dist_bord[ind_in_lin + axes_offsets[dim]];
                if (std::abs(dist_tmp) < std::abs(axes_mins[dim]))
                  axes_mins[dim] = dist_tmp;
              }
              else if (ind_in[dim] >  0 && ind_in[dim] == size_in[dim] - 1)
                axes_mins[dim] = dist_bord[ind_in_lin - axes_offsets[dim]];
              else if (ind_in[dim] == 0 && ind_in[dim] <  size_in[dim] - 1)
                axes_mins[dim] = dist_bord[ind_in_lin + axes_offsets[dim]];
            }

            bool pos_part = axes_mins.x >= 0.f && axes_mins.y >= 0.f && axes_mins.z >= 0.f;
            axes_mins = abs(axes_mins);

            if (axes_mins.x < big_active_const || axes_mins.y < big_active_const || axes_mins.z < big_active_const)
            {

              float val_before = dist_bord[ind_in_lin];
              if (pos_part)
              {
                dist_bord[ind_in_lin] = std::min( solve_eikonal(axes_mins, grid_spacing), val_before);
              }
              else
              {
                val_before = -std::abs(val_before);
                dist_bord[ind_in_lin] = std::max(-solve_eikonal(axes_mins, grid_spacing), val_before);
              }
              if (dist_bord[ind_in_lin] != val_before)
                updated_values[ind_in_lin] = true;
            }
          }
      }
    }
    // Is it a better loop exit condition?
    //#pragma omp parallel for
    for (uint32_t upd_i = 0u; upd_i < N_in; ++upd_i)
    {
      bool noninit_value = big_active_const == dist_bord[upd_i];
      if (noninit_value)
        has_updatable_values = true;
      frozen[upd_i] |= (!noninit_value && !updated_values[upd_i]);
      updated_values[upd_i] = false;
    }
  }

  // Copy dist_bord to dist_in (input array)
  // Save eikonal check values to the now unused dist_bord

  // eikonal_mean = 0.;
  // auto t3 = std::chrono::high_resolution_clock::now();

  //#pragma omp parallel for collapse(3)
  for (int i = 0; i < size_in.x; ++i)
    for (int j = 0; j < size_in.y; ++j)
      for (int k = 0; k < size_in.z; ++k)
      {
        uint32_t ind_in_lin = i * axes_offsets.x + j * axes_offsets.y + k;
        dist_in[ind_in_lin] = dist_bord[ind_in_lin];
      }
  
  //print_grid(dist_in, size_in);
  //printf("===================\n\n");
  //calculate_eikonal_mean(size_in, axes_offsets, dist_in, grid_spacing);
}
