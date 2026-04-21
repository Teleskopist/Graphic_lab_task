#pragma once
#include "utils/mesh/mesh.h"
#include "utils/common/static_vector.h"

namespace terrain
{
    /*
        Vector specifies number of layers of each size
        Each size is two time bigger then privious
        Sizes must be proper to satisfy relations between layers othewise std::logic_error is thrown
        With safe = true additional layers are added to satisfy relations
    */
    cmesh4::SimpleMesh create_mesh(const std::vector<size_t> &layer_counts, bool safe = true);

    enum class Cell
    {
        DEFAULT,
        CONNECTION
    };

    constexpr size_t MAX_TRIANGLES_IN_CELL = 3;

    static_vector<LiteMath::float2, 3 * MAX_TRIANGLES_IN_CELL> cell_to_triangles(Cell cell);

    enum class Orientation
    {
        RIGHT,
        TOP,
        LEFT,
        BOTTOM
    };

    inline LiteMath::float2 apply_orientation(LiteMath::float2 v, Orientation o)
    {
        int n = int(o);
        for (int i = 0; i < n; i++)
            v = {1.0f - v.y, v.x};
        return v;
    }

    void add_triangle(cmesh4::SimpleMesh &mesh, std::array<LiteMath::float3, 3> triangle);

    void add_cell(cmesh4::SimpleMesh &mesh, Cell cell, Orientation orientation, int size, LiteMath::int2 translate, float scale);

    void add_layer(cmesh4::SimpleMesh &mesh, int from, int to, Cell right_border, Cell top_right_corner, float scale);

    inline void add_default_layer(cmesh4::SimpleMesh &mesh, int from, int to, float scale)
    {
        add_layer(mesh, from, to, Cell::DEFAULT, Cell::DEFAULT, scale);
    }

    inline void add_connection_layer(cmesh4::SimpleMesh &mesh, int from, int to, float scale)
    {
        add_layer(mesh, from, to, Cell::CONNECTION, Cell::DEFAULT, scale);
    }

}
