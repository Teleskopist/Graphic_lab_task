#include "octahedral_groups.h"

namespace scom2
{

  void initialize_all_transpositions_rec(int idx, std::vector<int> &fixed_indices, std::vector<bool> &free_mask, 
                                         std::vector<std::vector<int>> &transpositions)
  {
    if (idx == fixed_indices.size())
    {
      transpositions.emplace_back();
      transpositions.back() = fixed_indices;
      return;
    }

    for (int i=0;i<fixed_indices.size();i++)
    {
      if (free_mask[i])
      {
        fixed_indices[idx] = i;
        free_mask[i] = false;
        initialize_all_transpositions_rec(idx+1, fixed_indices, free_mask, transpositions);
        free_mask[i] = true;
      }
    }
  }

  std::vector<std::vector<int>> initialize_all_transpositions(int size)
  {
    std::vector<int>  fixed_indices(size, 0);
    std::vector<bool> free_mask(size, true);
    std::vector<std::vector<int>> transpositions;
    initialize_all_transpositions_rec(0, fixed_indices, free_mask, transpositions);
    return transpositions;
  }

  void initialize_rot_transforms(int dim, int v_size, std::vector<LiteMath::Matrix<float>> &rot_transforms)
  {
    std::vector<std::vector<int>> transpositions = initialize_all_transpositions(dim);
    // printf("%d transpositions\n", (int)transpositions.size());
    // for (auto &t : transpositions)
    // {
    //   for (auto &v : t)
    //     printf("%d ", v);
    //   printf("\n");
    // }

    uint32_t sign_combinations = 1 << dim;
    std::vector<float> signs(dim), v1(dim), v2(dim);

    for (int d=0;d<dim;d++)
    {
      v1[d] = v_size/2.0f - 0.5f;
      v2[d] = -(v_size/2.0f - 0.5f);
    }
    LiteMath::Matrix<float> T1 = LiteMath::translate(dim+1, v1.data());
    LiteMath::Matrix<float> T2 = LiteMath::translate(dim+1, v2.data());

    //printf("%d rotations\n", int(transpositions.size()*sign_combinations));
    rot_transforms.resize(transpositions.size()*sign_combinations);
    for (uint32_t t_id=0; t_id<transpositions.size(); t_id++)
    {
      for (uint32_t sign=0; sign<sign_combinations; sign++)
      {
        for (int d=0;d<dim;d++)
          signs[d] = ((sign >> d) % 2 == 0) ? 1 : -1;

        LiteMath::Matrix<float> &T = rot_transforms[t_id*sign_combinations + sign];
        T = LiteMath::Matrix<float>(dim+1, dim+1);
        for (int d=0;d<dim;d++)
        {
          T[transpositions[t_id][d]][d] = signs[d];
        }
        T[dim][dim] = 1;

        T = mul(T1, mul(T, T2));
        //printf("[%d]\n", t_id*sign_combinations + sign);
        //print(T);
      }
    }
  }

  void initialize_rot_transforms(std::vector<float4x4> &rot_transforms, int v_size)
  {
    int universal_to_legacy_id_remap[48];
    std::vector<LiteMath::Matrix<float>> transforms_4x4;
    initialize_rot_transforms(3, v_size, transforms_4x4);
    int prev_size = rot_transforms.size();
    rot_transforms.resize(prev_size + ROT_COUNT, float4x4());

    for (int i=0; i<ROT_COUNT; i++)
    {
      const LiteMath::Matrix<float> &T = transforms_4x4[legacy_to_universal_id_remap[i]];
      float4x4 rot = LiteMath::float4x4(T[0][0], T[0][1], T[0][2], T[0][3], 
                                        T[1][0], T[1][1], T[1][2], T[1][3],
                                        T[2][0], T[2][1], T[2][2], T[2][3], 
                                        T[3][0], T[3][1], T[3][2], T[3][3]);    
      rot_transforms[prev_size + i] = rot;
    }
    return;
  }

  void initialize_rot_modifiers(std::vector<int4> &rot_modifiers, int v_size)
  {
    std::vector<float4x4> rotations4;
    initialize_rot_transforms(rotations4, v_size);

    int prev_size = rot_modifiers.size();
    rot_modifiers.resize(prev_size + ROT_COUNT, int4(0,0,0,0));

    for (int i=0; i<ROT_COUNT; i++)
    {
      constexpr int possible_modifiers_count = (2*4)*(2*4)*(2*4) * (2*2*2*2);
      std::vector<bool> modifier_possible(possible_modifiers_count, true);
      std::vector<int4> modifiers(possible_modifiers_count, int4(-1));
      for (int k=0; k<possible_modifiers_count; k++)
      {
        int j = k;
        int4 modifier(0,0,0,0);
        int4 mults(1,v_size, v_size*v_size, v_size*v_size*v_size);
        int4 mults2(1,v_size-1, v_size*(v_size-1), v_size*v_size*(v_size-1));
        modifier.x = mults[j % 4]; j /= 4;
        modifier.x *= (j%2) ? -1 : 1; j /= 2;
        modifier.y = mults[j % 4]; j /= 4;
        modifier.y *= (j%2) ? -1 : 1; j /= 2;
        modifier.z = mults[j % 4]; j /= 4;
        modifier.z *= (j%2) ? -1 : 1; j /= 2;
        modifier.w += j%2 ? mults2[0] : 0; j /= 2;
        modifier.w += j%2 ? mults2[1] : 0; j /= 2;
        modifier.w += j%2 ? mults2[2] : 0; j /= 2;
        modifier.w += j%2 ? mults2[3] : 0; j /= 2;
        modifiers[k] = modifier;

        for (int x = 0; x < v_size; x++)
        {
          for (int y = 0; y < v_size; y++)
          {
            for (int z = 0; z < v_size; z++)
            {
              int idx_a = dot(modifier, int4(x,y,z,1));
              float3 V = float3(x,y,z);
              int3 rot_vec = int3(LiteMath::mul4x3(rotations4[i], V));
              int idx_b = rot_vec.x * v_size*v_size + rot_vec.y * v_size + rot_vec.z;
              if (idx_a != idx_b)
              {
                modifier_possible[k] = false;
              }
            }
          }
        }
      }
      for (int j=0; j<possible_modifiers_count; j++)
      {
        if (modifier_possible[j])
        {
          rot_modifiers[prev_size+i] = modifiers[j];
        }
      }
    }
  }

std::vector<unsigned> generate_inverse_transform_indices(std::vector<float4x4> &rot_transforms)
  {
    std::vector<unsigned> inverse_indices(rot_transforms.size());
    int inverses_found = 0;
    for (int i=0; i<rot_transforms.size(); i++)
    {
      float4x4 inv = LiteMath::inverse4x4(rot_transforms[i]);
      for (int j=0; j<rot_transforms.size(); j++)
      {
        float diff = 0;
        for (int k=0; k<4; k++)
          for (int l=0; l<4; l++)
            diff += abs(rot_transforms[j][k][l] - inv[k][l]);
        if (diff < 1e-6f)
        {
          inverse_indices[i] = j;
          inverses_found++;
          break;
        }
      }
    }
    printf("inverse indices = {");
    for (int i=0; i<rot_transforms.size(); i++)
      printf("%d, ", inverse_indices[i]);
    printf("}\n");
    assert(inverses_found == rot_transforms.size());
    return inverse_indices;
  }

  void generate_rot_add_table(const std::vector<float4x4> &rot_transforms)
  {
    int sz = rot_transforms.size();
    printf("uint32_t rot_add_table[%d][%d] = {\n", sz, sz);
    for (int i=0; i<sz; i++)
    {
      printf("{");
      for (int j=0; j<sz; j++)
      {
        float4x4 res = rot_transforms[i] * rot_transforms[j];
        int idx = -1;
        float min_diff = 10000;
        for (int k=0; k<sz; k++)
        {
          float diff = 0;
          for (int l=0; l<4; l++)
            for (int m=0; m<4; m++)
              diff += abs(rot_transforms[k](l, m) - res(l, m));

          if (diff < 0.01f)
          {
            idx = k;
            break;
          }
        }
        assert(idx != -1);
        printf("%2d%s", idx, j < sz-1 ? ", " : "");
      }
      printf("}%s\n", i < sz-1 ? "," : "");
    }
    printf("};\n");
  }
}