#include "symmetric_groups.h"
#include "LiteMath/LiteMath.h"
#include "utils/common/matrix.h"
#include "utils/common/vector_map.h"
#include "utils/common/static_vector.h"
#include "utils/common/position_hash.h"
#include "utils/common/int_pow.h"

#include <cstdint>
#include <memory>
#include <cassert>

namespace scom2
{
  bool is_valid_transposition(const Transposition &t)
  {
    std::vector<uint32_t> counts(t.indices.size(), 0);
    for (uint32_t idx : t.indices)
    {
      if (idx >= counts.size())
        return false;
      counts[idx]++;
    }
    for (uint32_t cnt : counts)
    {
      if (cnt != 1)
        return false;
    }
    return true;
  }

  void compose(const Transposition &A, const Transposition &B, Transposition &C)
  {
    for (int i=0;i<A.indices.size();i++)
      C.indices[i] = A.indices[B.indices[i]];
  }

  Transposition transform_code_3D_to_transposition(int4 code, uint32_t v_size)
  {
    Transposition T;
    T.indices.resize(v_size*v_size*v_size, INVALID_IDX);
    for (uint32_t i=0; i<v_size; i++)
    {
      for (uint32_t j=0; j<v_size; j++)
      {
        for (uint32_t k=0; k<v_size; k++)
        {
          uint32_t idx = dot(int4(i,j,k,1), code);
          T.indices[i*v_size*v_size + j*v_size + k] = idx;
        }        
      }      
    }

    assert(is_valid_transposition(T));
    return T;
  }

  Transposition half_rotate_XZ(uint32_t v_size)
  {
    Transposition T;
    T.indices.resize(v_size*v_size*v_size, INVALID_IDX);
    for (uint32_t i=0; i<v_size; i++)
    {
      for (uint32_t j=0; j<v_size; j++)
      {
        for (uint32_t k=0; k<v_size; k++)
        {
          uint32_t i1 = i;
          uint32_t j1 = j;
          uint32_t k1 = i < v_size/2 ? v_size-1 - k : k;
          uint32_t idx1 = i1*v_size*v_size + j1*v_size + k1;
          T.indices[i*v_size*v_size + j*v_size + k] = idx1;
        }        
      }      
    }

    assert(is_valid_transposition(T)); 
    return T;   
  }

  Transposition edge_rotate_XZ(uint32_t v_size)
  {
    Transposition T;
    T.indices.resize(v_size*v_size*v_size, INVALID_IDX);
    for (uint32_t i=0; i<v_size; i++)
    {
      for (uint32_t j=0; j<v_size; j++)
      {
        for (uint32_t k=0; k<v_size; k++)
        {
          uint32_t i1 = i;
          uint32_t j1 = j;
          uint32_t k1 = i == 0 ? v_size-1 - k : k;
          uint32_t idx1 = i1*v_size*v_size + j1*v_size + k1;
          T.indices[i*v_size*v_size + j*v_size + k] = idx1;
        }        
      }      
    }

    assert(is_valid_transposition(T)); 
    return T;   
  }

  Transposition diagonal_mirror(uint32_t v_size)
  {
    Transposition T;
    T.indices.resize(v_size*v_size*v_size, INVALID_IDX);
    for (uint32_t i=0; i<v_size; i++)
    {
      for (uint32_t j=0; j<v_size; j++)
      {
        for (uint32_t k=0; k<v_size; k++)
        {
          uint32_t i1 = (v_size-1) - k;
          uint32_t j1 = j;
          uint32_t k1 = (v_size-1) - i;
          uint32_t idx1 = i1*v_size*v_size + j1*v_size + k1;
          T.indices[i*v_size*v_size + j*v_size + k] = idx1;
        }        
      }      
    }

    assert(is_valid_transposition(T)); 
    return T;   
  }

  Transposition corners_mirror(uint32_t v_size)
  {
    Transposition T;
    T.indices.resize(v_size*v_size*v_size, INVALID_IDX);
    for (uint32_t i=0; i<v_size; i++)
    {
      for (uint32_t j=0; j<v_size; j++)
      {
        for (uint32_t k=0; k<v_size; k++)
        {
          bool diag = (i==j && i==k);
          uint32_t i1 = diag ? (v_size-1) - i : i;
          uint32_t j1 = diag ? (v_size-1) - j : j;
          uint32_t k1 = diag ? (v_size-1) - k : k;
          uint32_t idx1 = i1*v_size*v_size + j1*v_size + k1;
          T.indices[i*v_size*v_size + j*v_size + k] = idx1;
        }        
      }      
    }

    assert(is_valid_transposition(T)); 
    return T;   
  }

  Transposition small_diag_mirror(uint32_t v_size)
  {
    Transposition T;
    T.indices.resize(v_size*v_size*v_size, INVALID_IDX);
    for (uint32_t i=0; i<v_size; i++)
    {
      for (uint32_t j=0; j<v_size; j++)
      {
        for (uint32_t k=0; k<v_size; k++)
        {
          bool diag = (i==j && k==0);
          uint32_t i1 = diag ? (v_size-1) - i : i;
          uint32_t j1 = diag ? (v_size-1) - j : j;
          uint32_t k1 = k;
          uint32_t idx1 = i1*v_size*v_size + j1*v_size + k1;
          T.indices[i*v_size*v_size + j*v_size + k] = idx1;
        }        
      }      
    }

    assert(is_valid_transposition(T)); 
    return T;   
  }

  Transposition swap_on_edge(uint32_t v_size)
  {
    Transposition T;
    T.indices.resize(v_size*v_size*v_size, INVALID_IDX);
    for (uint32_t i=0; i<v_size; i++)
    {
      for (uint32_t j=0; j<v_size; j++)
      {
        for (uint32_t k=0; k<v_size; k++)
        {
          bool edge = (i == 0) && (j == 0);
          uint32_t i1 = i;
          uint32_t j1 = j;
          uint32_t k1 = edge ? (v_size-1) - k : k;
          uint32_t idx1 = i1*v_size*v_size + j1*v_size + k1;
          T.indices[i*v_size*v_size + j*v_size + k] = idx1;
        }        
      }      
    }

    assert(is_valid_transposition(T)); 
    return T;   
  }

  Subgroup extend_to_subgroup(const std::vector<Transposition> &transpositions)
  {
    uint32_t trans_size = transpositions[0].indices.size();
    Subgroup s;
    VectorMap<uint32_t, uint32_t> subgroup_map;

    for (const Transposition &t : transpositions)
    {
      auto it = subgroup_map.find(t.indices);
      if (it == subgroup_map.end())
      {
        uint32_t idx = s.elements.size();
        s.elements.push_back(t);
        subgroup_map[t.indices] = idx;
      }
    }
    //printf("intial subset has %d distinct transpositions\n", (int)subgroup_map.size());
    s.cayley_table.resize(subgroup_map.size(), std::vector<uint32_t>(subgroup_map.size(), INVALID_IDX));

    Transposition A, B, C;
    A.indices.resize(trans_size, INVALID_IDX);
    B.indices.resize(trans_size, INVALID_IDX);
    C.indices.resize(trans_size, INVALID_IDX);

    uint32_t prev_size = 0;
    uint32_t cur_size = subgroup_map.size();
    while (prev_size != cur_size)
    {
      prev_size = cur_size;
      for (int i=0; i<prev_size; i++)
      {
        for (int j=0; j<prev_size; j++)
        {
          if (s.cayley_table[i][j] != INVALID_IDX)
            continue;

          compose(s.elements[i], s.elements[j], C);
          uint32_t idx = INVALID_IDX;
          auto it = subgroup_map.find(C.indices);
          if (it == subgroup_map.end())
          {
            idx = s.elements.size();
            s.elements.push_back(C);
            subgroup_map[C.indices] = idx;
            for (int k=0;k<cur_size;k++)
              s.cayley_table[k].push_back(INVALID_IDX);
            s.cayley_table.push_back(std::vector<uint32_t>(subgroup_map.size(), INVALID_IDX));
            cur_size++;
          }
          else
          {
            idx = it->second;
          }
          s.cayley_table[i][j] = idx;
        }
      }
    }

    //printf("subgroup has %d elements total\n", (int)subgroup_map.size());
    return s;
  }

  Transposition rot_map_to_transposition(const LiteMath::Matrix<float> &rot_map, int v_size, int dim)
  {
    int len = int_pow(v_size, dim);
    std::vector<float> V(dim+1);
    V[dim] = 1;
    Transposition T;
    T.indices.resize(len, INVALID_IDX);  
    for (int i=0;i<len;i++)
    {
      int idx = i;
      //for (int j=0;j<dim;j++)
      for (int j=dim-1;j>=0;j--)
      {
        V[j] = idx % v_size;
        idx /= v_size;
      }
      
      std::vector<float> V_rot = mul(rot_map, V);
      idx = 0;
      //for (int j=dim-1;j>=0;j--)
      for (int j=0;j<dim;j++)
      {
        idx *= v_size;
        idx += int(round(V_rot[j]));
      }
      T.indices[i] = idx;
    }

    assert(is_valid_transposition(T));
    return T;
  }

  Subgroup create_default_subgroup(int v_size, int dim)
  {
    std::vector<Transposition> transpositions;
    std::vector<LiteMath::Matrix<float>> rot_transforms;
    initialize_rot_transforms(dim, v_size, rot_transforms);
    for (int i=0;i<rot_transforms.size();i++)
      transpositions.push_back(rot_map_to_transposition(rot_transforms[i], v_size, dim));

    // for dim == 3 we need to remap legacy ids to support older models
    if (dim == 3)
    {
      std::vector<Transposition> prev_transpositions = transpositions;
      for (int i=0;i<rot_transforms.size();i++)
        transpositions[i] = prev_transpositions[legacy_to_universal_id_remap[i]];
    }

    return extend_to_subgroup(transpositions);
  }

  Subgroup create_LD_subgroup(int v_size)
  {
    // special case. We need subgroups for all v_size to have the same size
    // but if v_size = 2, LD subgroup is smaller, because some transpositions
    // appear identical. TO preserve same size we need to add more transpositions
    // even if they are identical to the ones we already have
    if (v_size == 2)
    {
      std::vector<int4> rot_codes;
      initialize_rot_modifiers(rot_codes, 3);  

      std::vector<Transposition> transpositions;
      for (int i=0;i<rot_codes.size();i++)
        transpositions.push_back(transform_code_3D_to_transposition(rot_codes[i], 3)); 
      transpositions.push_back(corners_mirror(3));

      Subgroup s = extend_to_subgroup(transpositions);
      for (int tid=0; tid<s.elements.size(); tid++)
      {
        Transposition T2;
        T2.indices.resize(2*2*2, INVALID_IDX);
        for (int i0=0; i0<3; i0++)
        {
          for (int j0=0; j0<3; j0++)
          {
            for (int k0=0; k0<3; k0++)
            {
              if (i0 % 2 || j0 % 2 || k0 % 2)
                continue;
              uint32_t idx0 = i0*3*3+j0*3+k0;
              uint32_t idx1 = s.elements[tid].indices[idx0];
              int i1 = idx1/(3*3);
              int j1 = idx1/3%3;
              int k1 = idx1%3;
              T2.indices[(i0/2)*2*2 + (j0/2)*2 + (k0/2)] = (i1/2)*2*2 + (j1/2)*2 + (k1/2);
            }
          }  
        }
        s.elements[tid] = T2;
      }

      return s;
    }
    else
    {
      std::vector<int4> rot_codes;
      initialize_rot_modifiers(rot_codes, v_size);  

      std::vector<Transposition> transpositions;
      for (int i=0;i<rot_codes.size();i++)
        transpositions.push_back(transform_code_3D_to_transposition(rot_codes[i], v_size)); 
      transpositions.push_back(corners_mirror(v_size));

      return extend_to_subgroup(transpositions);
    }
  }

  Subgroup create_side_swap_subgroup(int v_size)
  {
    assert(v_size == 2);
    std::vector<int4> rot_codes;
    initialize_rot_modifiers(rot_codes, v_size);  

    std::vector<Transposition> transpositions;
    for (int i=0;i<rot_codes.size();i++)
      transpositions.push_back(transform_code_3D_to_transposition(rot_codes[i], v_size)); 
    transpositions.push_back(half_rotate_XZ(v_size));

    return extend_to_subgroup(transpositions);
  }

  Subgroup create_full_transposition_subgroup(int v_size)
  {
    assert(v_size == 2);
    int N = v_size * v_size * v_size;

    std::vector<Transposition> transpositions;
    std::vector<uint32_t> perm(N);
    for (uint32_t i = 0; i < N; ++i)
      perm[i] = i;

    do 
    {
      Transposition t;
      t.indices = perm;
      transpositions.push_back(std::move(t));
    } while (std::next_permutation(perm.begin(), perm.end()));

    printf("transp size %d\n", (int)transpositions.size());

    return extend_to_subgroup(transpositions);
  }

  Subgroup create_identity_subgroup(int v_size)
  {
    int N = v_size * v_size * v_size;
    Transposition t;
    t.indices.resize(N);
    for (int i=0;i<N;i++)
      t.indices[i] = i;
    
    return extend_to_subgroup({t});
  }

  void fill_inverse_indices(Subgroup &s)
  {
    s.inverse_indices.resize(s.elements.size());
    for (uint32_t i=0; i<s.elements.size(); i++)
    {
      s.inverse_indices[i] = INVALID_IDX;
      for (uint32_t j=0; j<s.elements.size(); j++)
      {
        if (s.cayley_table[i][j] == 0)
        {
          s.inverse_indices[i] = j;
          break;
        }
      }
    }
  }

  const Subgroup *create_subgroup(TransformSubgroup subgroup_type, int v_size, int dim)
  {
    static CodeMap<std::shared_ptr<Subgroup>> subgroups;
    uint4 code = uint4(uint32_t(subgroup_type), v_size, dim, 0);
    if (subgroup_type == TransformSubgroup::SIDE_SWAP || subgroup_type == TransformSubgroup::ALL_TRANSPOSITIONS)
      code.y = 2;
    if (subgroups.find(code) != subgroups.end())
      return subgroups[code].get();

    Subgroup subgroup;
    switch (subgroup_type)
    {
    case TransformSubgroup::DEFAULT:
      subgroup = create_default_subgroup(v_size, dim);
      break;
    case TransformSubgroup::LARGE_DIAGONAL:
      if (dim != 3)
      {
        printf("TransformSubgroup::LARGE_DIAGONAL supports only dim = 3\n");
        assert(false);
        return nullptr;
      }
      subgroup = create_LD_subgroup(v_size);
      break;
    case TransformSubgroup::SIDE_SWAP:
      if (dim != 3)
      {
        printf("TransformSubgroup::SIDE_SWAP supports only dim = 3\n");
        assert(false);
        return nullptr;
      }
      subgroup = create_side_swap_subgroup(2);
      break;
    case TransformSubgroup::ALL_TRANSPOSITIONS:
      if (dim != 3)
      {
        printf("TransformSubgroup::ALL_TRANSPOSITIONS supports only dim = 3\n");
        assert(false);
        return nullptr;
      }
      subgroup = create_full_transposition_subgroup(2);
      break;
    case TransformSubgroup::NO_TRANSFORMS:
      subgroup = create_identity_subgroup(v_size);
      break;
    default:
      printf("Unknown subgroup type %d\n", (int)subgroup_type);
      assert(false);
      return nullptr;
      break;
    }

    fill_inverse_indices(subgroup);
    subgroups[code] = std::make_shared<Subgroup>(subgroup);

    return subgroups[code].get();
  }

  void transposition_tables_experiments()
  {
    int v_size = 3;

    Subgroup s = create_LD_subgroup(v_size);
    OrderedVectorMap<uint32_t, uint32_t> elem_map;
    for (auto &elem : s.elements)
      elem_map[elem.indices] = elem_map.size();
    printf("Elements\n");
    int i=0;
    for (auto &pair : elem_map)
    {
      printf("%3d] ", i++);
      for (uint32_t elem : pair.first)
        printf("%2d ", elem);
      printf("\n");
    }
    // printf("Cayley table\n");
    // for (int i=0;i<s.elements.size();i++)
    // {
    //   for (int j=0;j<s.elements.size();j++)
    //   {
    //     printf("%3d ", s.cayley_table[i][j]);
    //   }      
    //   printf("\n");
    // }
    // printf("\n");
  }
}