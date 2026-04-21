#pragma once
#include "LiteMath.h"
#include <string>
#include <cstdint>
#include "sdf/common/global_octree.h"
#include "sdf/scom2/scom.h"

using LiteMath::float3;

struct RadianceField {
    static constexpr unsigned MAX_DEPTH = 16;

    int8_t sh_degree;
    float3 scene_center;
    float inside_extent;
    float scene_extent;
    std::vector<int64_t> octpath;
    std::vector<int8_t> octlevel;
    std::vector<float> _geo_grid_pts;
    std::vector<float3> _sh0;
    std::vector<float3> _shs; // [num_voxels, ((sh_degree+1)^2 - 1)] as 1-D vector
};

void load_radiance_field(RadianceField &radiance_field, const std::string &path);
void load_radiance_field(SdfDAG &dag, const std::string &path);

void radiance_field_to_global_octree(const RadianceField &grid, sdf_converter::GlobalOctree &octree);