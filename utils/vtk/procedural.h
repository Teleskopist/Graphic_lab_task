#pragma once
#include "vtk_structures.h"
#include "blk/blk.h"

namespace vtk
{
  //unified interface to create procedural datasets from blk
  void create_procedural_dataset(const Block &settings, Dataset &dataset);
  void create_procedural_dataset(const char *blk_file, Dataset &dataset);
}