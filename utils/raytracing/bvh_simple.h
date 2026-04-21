#pragma once

#include "LiteMath.h"
#include "cbvh.h"

/* 
Simple BVH builder, does not depend on any external library (e.g. embree).
It is obviously worse than embree implementation and should not be used in production.
However, it is good enough and can be useful for some projects where we want minimal dependencies.
*/
BVHTree BuildBVHSimple(const LiteMath::float4 *a_vertices, size_t a_vertNum, const uint *a_indices, size_t a_indexNum, BVHPresets a_presets);