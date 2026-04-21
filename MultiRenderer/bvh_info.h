#pragma once
#include "BVH2Common.h"

namespace bvh_info
{

    struct GeomNodes
    {
        // List of BVHNodes per layer
        std::vector<std::vector<BVHNode>> nodes;
    };

    GeomNodes get_geometry_nodes(const BVHRT *bvh, uint32_t geomId);

}
