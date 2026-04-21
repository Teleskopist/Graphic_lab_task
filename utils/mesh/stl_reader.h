#pragma once
#include "mesh.h"

namespace cmesh4
{
  cmesh4::SimpleMesh LoadMeshFromStl(const std::string& path, bool verbose = false);
}