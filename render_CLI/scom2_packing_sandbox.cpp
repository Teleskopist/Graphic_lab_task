#include "scom2_packing_sandbox.h"
#include "sdf/scom2/scom_builder.h"

void scom2_test_packing_modes(const char *filename)
{
  {
  scom2::Settings settings;
  settings.packed_reference_type = scom2::PackedReferenceType::SHORT_6_8_18;
  settings.packed_surface_limit = scom2::PackedSurfaceLimit::STRICT;
  settings.packed_brick_type = scom2::PackedBrickType::FULL;
  settings.bits_per_value = 8;
  
  SdfDAG dag;
  SCom2Tree scom2_tree;

  load_sdf_DAG(dag, filename);
  settings.brick_size = dag.header.brick_size;
  scom2::pack_SCom2(settings, dag, scom2_tree);
  }

  {
  scom2::Settings settings;
  settings.packed_reference_type = scom2::PackedReferenceType::LONG_6_26_32;
  settings.packed_surface_limit = scom2::PackedSurfaceLimit::STRICT;
  settings.packed_brick_type = scom2::PackedBrickType::BITMASK;
  settings.bits_per_value = 8;
  
  SdfDAG dag;
  SCom2Tree scom2_tree;

  load_sdf_DAG(dag, filename);
  settings.brick_size = dag.header.brick_size;
  scom2::pack_SCom2(settings, dag, scom2_tree);
  }
}