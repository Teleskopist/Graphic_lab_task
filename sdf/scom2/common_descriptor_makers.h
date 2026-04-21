#pragma once
#include "scom_builder.h"

namespace scom2
{
  // empty descriptor maker is a dummy that does nothing
  std::shared_ptr<IDescriptorMaker> create_empty_descriptor_maker();

  // trivial descriptor maker copies data to the descriptor as is
  std::shared_ptr<IDescriptorMaker> create_trivial_descriptor_maker(const SdfDAG *_dag, TransformSubgroup _transform_subgroup);

  // integer descriptor maker takes integer part of the data and multiplies it by 10^7, it is meant to be used for integer
  // values like material ids if subdataset split cannot be used (e.g. for branches). If you use Lp distances on theese descriptors
  // the difference between different integer values will be very large and they will never be clustered together
  // NOTE: for large integers float precision won't be enough to represent them, so you might want to create custom descriptor
  //       maker with task-specific encoding
  std::shared_ptr<IDescriptorMaker> create_integer_descriptor_maker(const SdfDAG *_dag, TransformSubgroup _transform_subgroup);

  // default sdf descriptor maker is the one that is used in the original SCom paper. It treats the data as sdf grid, subtracts average
  // value and normalizes given vector. If the input node type is SURFACE, it applies surface sensitivity multiplier, which results in
  // less aggressive compression for surfaces, as the compression artifacts are usually more visible there
  // If the input node type is INTERNAL, it sets all values to -1.0, which results in compressing all internal nodes to a single cluster
  std::shared_ptr<IDescriptorMaker> create_default_sdf_descriptor_maker(const SdfDAG *_dag, TransformSubgroup _transform_subgroup, float _surface_sensitivity);
  
  // normalized descriptor maker transforms each value from _min_max_range to [0.0, target_range]
  std::shared_ptr<IDescriptorMaker> create_normalized_descriptor_maker(const SdfDAG *_dag, TransformSubgroup _transform_subgroup, float2 _min_max_range, float target_range);
}