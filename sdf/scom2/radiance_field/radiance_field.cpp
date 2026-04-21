#include "radiance_field.h"
#include "sdf/scom2/scom_utils.h"

#include <iostream>
#include <fstream>

void load_radiance_field(RadianceField &radiance_field, const std::string &path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);

    int32_t num_voxels;
    int32_t num_grid_pts;

    file.read((char *)&radiance_field.sh_degree, sizeof(radiance_field.sh_degree));
    file.read((char *)&radiance_field.scene_center, sizeof(radiance_field.scene_center));
    file.read((char *)&radiance_field.inside_extent, sizeof(radiance_field.inside_extent));
    file.read((char *)&radiance_field.scene_extent, sizeof(radiance_field.scene_extent));
    
    file.read((char *)&num_voxels, sizeof(num_voxels));
    file.read((char *)&num_grid_pts, sizeof(num_grid_pts));

    radiance_field.octpath.resize(num_voxels);
    radiance_field.octlevel.resize(num_voxels);
    radiance_field._geo_grid_pts.resize(num_grid_pts);
    radiance_field._sh0.resize(num_voxels);
    radiance_field._shs.resize(num_voxels * ((radiance_field.sh_degree + 1) * (radiance_field.sh_degree + 1) - 1));

    file.read((char *)radiance_field.octpath.data(), sizeof(radiance_field.octpath[0]) * num_voxels);
    file.read((char *)radiance_field.octlevel.data(), sizeof(radiance_field.octlevel[0]) * num_voxels);
    file.read((char *)radiance_field._geo_grid_pts.data(), sizeof(radiance_field._geo_grid_pts[0]) * num_grid_pts);
    file.read((char *)radiance_field._sh0.data(), sizeof(radiance_field._sh0[0]) * num_voxels);
    file.read((char *)radiance_field._shs.data(), sizeof(radiance_field._shs[0]) * num_voxels * ((radiance_field.sh_degree + 1) * (radiance_field.sh_degree + 1) - 1));

    file.close();
}

struct ijk {
    uint32_t i, j, k;
    int8_t level;
    int32_t index;
};

static float exp_linear_11(float x)
{
    return (x > 1.1f) ? x : expf(0.909090909091f * x - 0.904689820196f);
}

void radiance_field_to_global_octree(const RadianceField &radiance_field, sdf_converter::GlobalOctree &octree) {
    const size_t value_size = 8;
    const size_t color_data_size = (radiance_field.sh_degree + 1) * (radiance_field.sh_degree + 1) * 3;
    const size_t shs_count = (radiance_field.sh_degree + 1) * (radiance_field.sh_degree + 1) - 1;

    std::vector<ijk> coords;
    std::vector<ijk> grid_pts;
    coords.resize(radiance_field.octpath.size());
    grid_pts.resize(radiance_field.octpath.size() * 8);
    for (size_t idx = 0; idx < radiance_field.octpath.size(); ++idx) {
        int64_t path = radiance_field.octpath[idx];
        path >>= 3 * (RadianceField::MAX_DEPTH - radiance_field.octlevel[idx]);
        int64_t i = 0, j = 0, k = 0;
        for (int l = 0; l < radiance_field.octlevel[idx]; ++l) {
            i |= ((path & 0b100) >> 2) << l;
            j |= ((path & 0b010) >> 1) << l;
            k |= (path & 0b001) << l;

            path >>= 3;
        }
        coords[idx].i = i << (RadianceField::MAX_DEPTH - radiance_field.octlevel[idx]);
        coords[idx].j = j << (RadianceField::MAX_DEPTH - radiance_field.octlevel[idx]);
        coords[idx].k = k << (RadianceField::MAX_DEPTH - radiance_field.octlevel[idx]);
        coords[idx].level = radiance_field.octlevel[idx];
        coords[idx].index = idx;
        for (int jdx = 0; jdx < 8; ++jdx) {
            grid_pts[8 * idx + jdx].i = coords[idx].i + (((jdx & 0b100) >> 2) << (RadianceField::MAX_DEPTH - radiance_field.octlevel[idx]));
            grid_pts[8 * idx + jdx].j = coords[idx].j + (((jdx & 0b010) >> 1) << (RadianceField::MAX_DEPTH - radiance_field.octlevel[idx]));
            grid_pts[8 * idx + jdx].k = coords[idx].k + ((jdx & 0b001) << (RadianceField::MAX_DEPTH - radiance_field.octlevel[idx]));
            grid_pts[8 * idx + jdx].level = radiance_field.octlevel[idx];
            grid_pts[8 * idx + jdx].index = 8 * idx + jdx;
        }
    }

    std::sort(grid_pts.begin(), grid_pts.end(), [](const ijk& a, const ijk& b) {
        return a.i < b.i || (a.i == b.i && a.j < b.j) || (a.i == b.i && a.j == b.j && a.k < b.k);
    });

    std::vector<size_t> vox_keys;
    vox_keys.resize(radiance_field.octpath.size() * 8);
    size_t unique_count = 0;
    for (size_t idx = 0; idx < grid_pts.size(); ++idx) {
        if (idx > 0 && (grid_pts[idx].i != grid_pts[idx - 1].i || grid_pts[idx].j != grid_pts[idx - 1].j || grid_pts[idx].k != grid_pts[idx - 1].k)) {
            unique_count++;
        }
        vox_keys[grid_pts[idx].index] = unique_count;
    }

    std::vector<sdf_converter::GlobalOctreeNode> nodes{ { sdf_converter::GlobalOctreeNodeType::NODE, true, false, false, 0, 0, 1 } };
    std::vector<float> values{ 1000.0f, 1000.0f, 1000.0f, 1000.0f, 1000.0f, 1000.0f, 1000.0f, 1000.0f };
    std::vector<float> color;
    values.resize((radiance_field.octpath.size() + 1) * value_size);
    color.resize(color_data_size, 0);

    for (size_t idx = 0; idx < coords.size(); ++idx) {
        size_t node_idx = 0;
        for (int l = 1; l <= coords[idx].level; ++l) {
            nodes[node_idx].is_outside = false;
            if (nodes[node_idx].offset == 0) {
                nodes[node_idx].offset = nodes.size();
                nodes.insert(nodes.end(), {
                    { sdf_converter::GlobalOctreeNodeType::EMPTY_NODE, true, false, false, 0, 0, 1 },
                    { sdf_converter::GlobalOctreeNodeType::EMPTY_NODE, true, false, false, 0, 0, 1 },
                    { sdf_converter::GlobalOctreeNodeType::EMPTY_NODE, true, false, false, 0, 0, 1 },
                    { sdf_converter::GlobalOctreeNodeType::EMPTY_NODE, true, false, false, 0, 0, 1 },
                    { sdf_converter::GlobalOctreeNodeType::EMPTY_NODE, true, false, false, 0, 0, 1 },
                    { sdf_converter::GlobalOctreeNodeType::EMPTY_NODE, true, false, false, 0, 0, 1 },
                    { sdf_converter::GlobalOctreeNodeType::EMPTY_NODE, true, false, false, 0, 0, 1 },
                    { sdf_converter::GlobalOctreeNodeType::EMPTY_NODE, true, false, false, 0, 0, 1 }
                });
                color.resize(color.size() + color_data_size * 8, 0);
            }
            node_idx = nodes[node_idx].offset + ((((coords[idx].i >> (RadianceField::MAX_DEPTH - l)) & 0b1) << 2) | (((coords[idx].j >> (RadianceField::MAX_DEPTH - l)) & 0b1) << 1) | ((coords[idx].k >> (RadianceField::MAX_DEPTH - l)) & 0b1));
        }

        nodes[node_idx].is_outside = false;
        nodes[node_idx].is_internal = true;
        nodes[node_idx].type = sdf_converter::GlobalOctreeNodeType::LEAF;
        nodes[node_idx].val_off = (coords[idx].index + 1) * 8;
        for (int i = 0; i < 8; ++i) 
        {
            float raw_val = radiance_field._geo_grid_pts[vox_keys[coords[idx].index * 8 + i]];
            values[(coords[idx].index + 1) * 8 + i] = -1.0f*raw_val;
            //printf("val = %f\n", values[(coords[idx].index + 1) * 8 + i]);
        }

        color[node_idx * color_data_size + 0] = radiance_field._sh0[coords[idx].index].x;
        color[node_idx * color_data_size + 1] = radiance_field._sh0[coords[idx].index].y;
        color[node_idx * color_data_size + 2] = radiance_field._sh0[coords[idx].index].z;
        //printf("idx %d, sh0 %f %f %f\n", coords[idx].index, color[node_idx * color_data_size + 0], color[node_idx * color_data_size + 1], color[node_idx * color_data_size + 2]);
        for (int i = 0; i < shs_count; ++i) {
            color[node_idx * color_data_size + 3 * (i + 1) + 0] = radiance_field._shs[coords[idx].index * shs_count + i].x;
            color[node_idx * color_data_size + 3 * (i + 1) + 1] = radiance_field._shs[coords[idx].index * shs_count + i].y;
            color[node_idx * color_data_size + 3 * (i + 1) + 2] = radiance_field._shs[coords[idx].index * shs_count + i].z;
        }
    }

    octree.nodes = nodes;
    octree.values_f = values;

    DataChannel dc;
    dc.name = "sh";
    dc.num_components = color_data_size;
    dc.data_f = color;

    dc.min_val = dc.data_f[0];
    dc.max_val = dc.data_f[0];
    for (size_t idx = 1; idx < dc.data_f.size(); ++idx) 
    {
        dc.min_val = std::min<double>(dc.min_val, dc.data_f[idx]);
        dc.max_val = std::max<double>(dc.max_val, dc.data_f[idx]);
    }

    //large negatve values are probably artifacts of RF conversion and should be clamped
    dc.min_val = std::max(dc.min_val, -dc.max_val);
    for (auto &v : dc.data_f) 
    {
        v = std::clamp<float>(v, dc.min_val, dc.max_val);
    }

    octree.voxel_channels.clear();
    octree.voxel_channels.push_back(dc);

    for (auto &n : octree.nodes) 
    {
        if (n.offset != 0 && n.type == sdf_converter::GlobalOctreeNodeType::LEAF)
            n.type = sdf_converter::GlobalOctreeNodeType::EMPTY_NODE;

        if (n.type != sdf_converter::GlobalOctreeNodeType::LEAF) 
        {
            n.bricks_count = 0;
            n.val_off = 0;
        }
    }
}

void load_radiance_field(SdfDAG &dag, const std::string &path)
{
    RadianceField rf;
    sdf_converter::GlobalOctree go;
    load_radiance_field(rf, path);
    radiance_field_to_global_octree(rf, go);
    sdf_converter::global_octree_to_DAG8_direct(go, dag);

    //scom2::rotate_DAG(LiteMath::float3x3(1,0,0, 0,-1,0, 0,0,1), dag);

    //no channel data in empty nodes (without surfaces)
    for (auto &node : dag.nodes)
    {
        if (DAG_extract_count(node.voxel_count_flags) == 0)
        {
            node.channels_edge.child_offset = 0;
        }
    }
    dag.header.user_params[0] = rf.scene_extent;
    dag.header.user_params[1] = rf.scene_center.x;
    dag.header.user_params[2] = rf.scene_center.y;
    dag.header.user_params[3] = rf.scene_center.z;

    uint32_t max_level = scom2::calculate_DAG_max_depth_rec(dag, 0);
    printf("RF max depth: %u\n", max_level);

    if (max_level > sdf_converter::GlobalOctree::MAX_OCTREE_DEPTH) 
    {
        printf("DAG max depth exceeded\n");
        //TODO: make DAG shallower
    }
}