#pragma once
#include "octahedral_groups.h"
#include <vector>
#include <cstdint>

namespace scom2
{
  struct Transposition
  {
    std::vector<uint32_t> indices;

    const uint32_t &operator[](int i) const { return indices[i]; }
    uint32_t &operator[](int i) { return indices[i]; }
  };

  struct Subgroup
  {
    std::vector<Transposition> elements;
    std::vector<uint32_t> inverse_indices; // number of inverse elements for each element in group
    std::vector<std::vector<uint32_t>> cayley_table;
  };


  // which set of transformations is used during similarty compression
  enum class TransformSubgroup
  {
    DEFAULT, // rotations + mirroring, 48 transformations for octree
    LARGE_DIAGONAL, // rotations + mirroring + large diagonal swap, 48*8 or 48*16 transformations for octree
    SIDE_SWAP,
    ALL_TRANSPOSITIONS,
    NO_TRANSFORMS
  };

  const Subgroup *create_subgroup(TransformSubgroup subgroup_type, int v_size, int dim = 3);

  void transposition_tables_experiments();
}