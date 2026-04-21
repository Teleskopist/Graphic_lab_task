#include "terrain.h"
#include "utils/mesh/mesh_internal.h"
#include "utils/common/bit_cast.h"
#include <iostream>

namespace terrain
{
    cmesh4::SimpleMesh create_mesh(const std::vector<size_t> &layer_counts, bool safe)
    {

        cmesh4::SimpleMesh mesh(0, 0);

        size_t total_size = 0;
        {
            size_t cell_size = 1;
            for (size_t count : layer_counts)
            {
                total_size += total_size ? (count + 1) * cell_size : count * cell_size;
                if (safe && total_size % (cell_size * 2) != 0)
                {
                    total_size += cell_size;
                }
                cell_size *= 2;
            }
        }
        size_t from = 0;
        size_t cell_size = 1;
        float scale = 1.0f / total_size;
        for (size_t count : layer_counts)
        {
            if (from > 0)
            {
                cell_size *= 2;
                add_connection_layer(mesh, from, from + cell_size, scale);
                from += cell_size;
            }
            for (size_t i = 0; i < count; i++)
            {
                add_default_layer(mesh, from, from + cell_size, scale);
                from += cell_size;
            }

            if (safe && from % (cell_size * 2) != 0)
            {
                add_default_layer(mesh, from, from + cell_size, scale);
                from += cell_size;
            }
        }

        double eps = scale / 4;

        // cmesh4::compress_close_vertices(mesh, eps);

        return mesh;
    }

    static_vector<LiteMath::float2, 3 * MAX_TRIANGLES_IN_CELL> cell_to_triangles(Cell cell)
    {
        if (cell == Cell::DEFAULT)
        {
            return {
                {0, 0},
                {1, 0},
                {1, 1},
                {0, 0},
                {1, 1},
                {0, 1}};
        }
        else
        {
            return {
                {0, 0},
                {1, 0},
                {0, 0.5f},
                {0, 0.5f},
                {1, 0},
                {1, 1},
                {0, 0.5f},
                {1, 1},
                {0, 1}};
        }
    }

    void add_triangle(cmesh4::SimpleMesh &mesh, std::array<LiteMath::float3, 3> triangle)
    {
        size_t verticies_count = mesh.VerticesNum();
        size_t indicies_count = mesh.IndicesNum();
        mesh.Resize(verticies_count + 3, indicies_count + 3);
        for (size_t i = 0; i < 3; i++)
        {
            mesh.vPos4f[verticies_count + i] = to_float4(triangle[i], 1.0f);
            mesh.indices[indicies_count + i] = verticies_count + i;
        }
    }

    void add_cell(cmesh4::SimpleMesh &mesh, Cell cell, Orientation orientation, int size, LiteMath::int2 translate, float scale)
    {
        auto verticies = cell_to_triangles(cell);
        static_vector<LiteMath::float3, verticies.capacity()> transformed_verticies{};

        for (auto i : verticies)
        {
            i = apply_orientation(i, orientation);
            i *= float(size);
            i += LiteMath::float2(translate.x, translate.y);
            i *= scale;
            transformed_verticies.push_back({i.x, 0, i.y});
        }

        for (size_t i = 0; i < transformed_verticies.size(); i += 3)
        {
            add_triangle(mesh, {transformed_verticies[i], transformed_verticies[i + 1], transformed_verticies[i + 2]});
        }
    }

    void add_layer(cmesh4::SimpleMesh &mesh, int from, int to, Cell right_border, Cell top_right_corner, float scale)
    {
        int size = to - from;
        if (!(to >= 0 && from >= 0 && size > 0 && from % size == 0))
        {
            throw std::logic_error("Invalid terrain mesh layer parameters");
        }
        int n = from / size;

        for (int i = -n; i < n; i++)
        {
            add_cell(mesh, right_border, Orientation::RIGHT, size, {from, i * size}, scale);
            add_cell(mesh, right_border, Orientation::TOP, size, {i * size, from}, scale);
            add_cell(mesh, right_border, Orientation::LEFT, size, {-from - size, i * size}, scale);
            add_cell(mesh, right_border, Orientation::BOTTOM, size, {i * size, -from - size}, scale);
        }
        add_cell(mesh, top_right_corner, Orientation::RIGHT, size, {from, from}, scale);
        add_cell(mesh, top_right_corner, Orientation::TOP, size, {-from - size, from}, scale);
        add_cell(mesh, top_right_corner, Orientation::LEFT, size, {-from - size, -from - size}, scale);
        add_cell(mesh, top_right_corner, Orientation::BOTTOM, size, {from, -from - size}, scale);
    }

}
