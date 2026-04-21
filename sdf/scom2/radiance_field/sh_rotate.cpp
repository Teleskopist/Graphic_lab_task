#include "sh_rotate.h"

#include "utils/common/rand.h"
#include "utils/common/solve_linear.h"
#include "sdf/scom2/scom_utils.h"

  void calculate_sh_rotate_matrix(uint32_t deg, LiteMath::float4x4 rot, std::vector<float> &out_mat)
  {
    uint32_t max_cnt = (deg + 1) * (deg + 1);
    std::vector<std::vector<float3>> sh_coeffs(max_cnt, std::vector<float3>(max_cnt));
    for (int i = 0; i < max_cnt; i++)
      for (int j = 0; j < max_cnt; j++)
        sh_coeffs[i][j] = float3((i == j) ? 1.0f : 0.0f);

    out_mat.resize(max_cnt * max_cnt);
    out_mat[0] = 1.0f;

    for (int d = 1; d <= deg; d++)
    {
      uint32_t cnt = 2 * d + 1;
      std::vector<std::vector<double>> d_res(cnt, std::vector<double>(cnt));
      for (int m = 0; m < cnt; m++)
      {
        // generate dirs
        std::vector<float3> dirs(cnt);
        dirs[0] = float3(0, 1, 0);
        for (int sample = 1; sample < cnt; sample++)
        {
          float3 dir;
          float max_dot = 1.0f;
          while (max_dot > 0.95f)
          {
            dir = normalize(float3(urand(-1, 1), urand(-1, 1), urand(-1, 1)));
            max_dot = 0.0f;
            for (int i = 0; i < sample; i++)
              max_dot = std::max(max_dot, std::abs(dot(dir, dirs[i])));
          }
          dirs[sample] = dir;
        }

        std::vector<std::vector<double>> A(cnt, std::vector<double>(cnt));
        std::vector<double> b(cnt);
        for (int sample = 0; sample < cnt; sample++)
        {
          for (int k = 0; k < cnt; k++)
            A[sample][k] = computeColorFromSH(float3(0.0f), dirs[sample], 0, deg, cnt, sh_coeffs[d * d + k].data()).x;
          b[sample] = computeColorFromSH(float3(0.0f), to_float3(rot * to_float4(dirs[sample],1.0f)), 0, deg, cnt, sh_coeffs[d * d + m].data()).x;
        }
        d_res[m] = solveLinear(A, b);
      }
      for (int i = d * d; i < (d + 1) * (d + 1); i++)
      {
        for (int j = d * d; j < (d + 1) * (d + 1); j++)
        {
          out_mat[i * max_cnt + j] = d_res[i - d * d][j - d * d];
        }
      }
    }
  }

  //calculates Spherical Harmonics rotate matrices for 48 coded rotations
  void calculate_and_print_all_sh_rotate_matrices()
  {
    std::vector<float4x4> rot_transforms;
    scom2::initialize_rot_transforms(rot_transforms, 2);

    printf("static constexpr float all_sh_rotate_matrices[] = {");
    uint32_t ti = 0;
    for (auto &transform : rot_transforms)
    {
      transform.col(3) = float4(0,0,0,1);
      printf("\n//matrix %d\n", ti++);
      std::vector<float> A;
      calculate_sh_rotate_matrix(SH_MAX_DEGREE, transform, A);
      for (int i=0;i<SH_MAX_COEFFS;i++)
      {
        for (int j=0;j<SH_MAX_COEFFS;j++)
          printf("%9.6f, ", std::abs(A[i*SH_MAX_COEFFS+j])<1e-7f ? 0.0f : A[i*SH_MAX_COEFFS+j]);
        printf("\n");
      }
    }
    printf("};\n");
  }