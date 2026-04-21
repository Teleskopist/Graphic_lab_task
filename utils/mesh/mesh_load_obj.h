#pragma once
#include "mesh.h"

namespace cmesh4
{
  SimpleMesh LoadMeshFromObj(const char* a_fileName, bool aVerbose = false, const char* a_mtlBaseDir = nullptr);
}
