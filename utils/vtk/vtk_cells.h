#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace vtk
{
  using id_t = uint32_t; //we expect datasets to have ~1 billion points at most, so uint32_t should be enough

  // taken from vtk source code 
  // /https://gitlab.kitware.com/vtk/vtk/-/blob/master/Common/DataModel/vtkCellType.h
  enum VTKCellType : uint8_t
  {
    // Linear cells
    VTK_EMPTY_CELL = 0,
    VTK_VERTEX = 1,
    VTK_POLY_VERTEX = 2,
    VTK_LINE = 3,
    VTK_POLY_LINE = 4,
    VTK_TRIANGLE = 5,
    VTK_TRIANGLE_STRIP = 6,
    VTK_POLYGON = 7,
    VTK_PIXEL = 8,
    VTK_QUAD = 9,
    VTK_TETRA = 10,
    VTK_VOXEL = 11,
    VTK_HEXAHEDRON = 12,
    VTK_WEDGE = 13,
    VTK_PYRAMID = 14,
    VTK_PENTAGONAL_PRISM = 15,
    VTK_HEXAGONAL_PRISM = 16,

    // Quadratic, isoparametric cells
    VTK_QUADRATIC_EDGE = 21,
    VTK_QUADRATIC_TRIANGLE = 22,
    VTK_QUADRATIC_QUAD = 23,
    VTK_QUADRATIC_POLYGON = 36,
    VTK_QUADRATIC_TETRA = 24,
    VTK_QUADRATIC_HEXAHEDRON = 25,
    VTK_QUADRATIC_WEDGE = 26,
    VTK_QUADRATIC_PYRAMID = 27,
    VTK_BIQUADRATIC_QUAD = 28,
    VTK_TRIQUADRATIC_HEXAHEDRON = 29,
    VTK_TRIQUADRATIC_PYRAMID = 37,
    VTK_QUADRATIC_LINEAR_QUAD = 30,
    VTK_QUADRATIC_LINEAR_WEDGE = 31,
    VTK_BIQUADRATIC_QUADRATIC_WEDGE = 32,
    VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON = 33,
    VTK_BIQUADRATIC_TRIANGLE = 34,

    // Cubic, isoparametric cell
    VTK_CUBIC_LINE = 35,

    // Special class of cells formed by convex group of points
    VTK_CONVEX_POINT_SET = 41,

    // Polyhedron cell (consisting of polygonal faces)
    VTK_POLYHEDRON = 42,

    // Higher order cells in parametric form
    VTK_PARAMETRIC_CURVE = 51,
    VTK_PARAMETRIC_SURFACE = 52,
    VTK_PARAMETRIC_TRI_SURFACE = 53,
    VTK_PARAMETRIC_QUAD_SURFACE = 54,
    VTK_PARAMETRIC_TETRA_REGION = 55,
    VTK_PARAMETRIC_HEX_REGION = 56,

    // Higher order cells
    VTK_HIGHER_ORDER_EDGE = 60,
    VTK_HIGHER_ORDER_TRIANGLE = 61,
    VTK_HIGHER_ORDER_QUAD = 62,
    VTK_HIGHER_ORDER_POLYGON = 63,
    VTK_HIGHER_ORDER_TETRAHEDRON = 64,
    VTK_HIGHER_ORDER_WEDGE = 65,
    VTK_HIGHER_ORDER_PYRAMID = 66,
    VTK_HIGHER_ORDER_HEXAHEDRON = 67,

    // Arbitrary order Lagrange elements (formulated separated from generic higher order cells)
    VTK_LAGRANGE_CURVE = 68,
    VTK_LAGRANGE_TRIANGLE = 69,
    VTK_LAGRANGE_QUADRILATERAL = 70,
    VTK_LAGRANGE_TETRAHEDRON = 71,
    VTK_LAGRANGE_HEXAHEDRON = 72,
    VTK_LAGRANGE_WEDGE = 73,
    VTK_LAGRANGE_PYRAMID = 74,

    // Arbitrary order Bezier elements (formulated separated from generic higher order cells)
    VTK_BEZIER_CURVE = 75,
    VTK_BEZIER_TRIANGLE = 76,
    VTK_BEZIER_QUADRILATERAL = 77,
    VTK_BEZIER_TETRAHEDRON = 78,
    VTK_BEZIER_HEXAHEDRON = 79,
    VTK_BEZIER_WEDGE = 80,
    VTK_BEZIER_PYRAMID = 81,

    VTK_NUMBER_OF_CELL_TYPES
  };

  //Polygon is not an independent entity, it stores
  //only ids, actual points are stored in UnstructuredGrid
  //Polygon is guaranteed to be planar
  struct Polygon
  {
    uint32_t indices_offset = 0;
    uint16_t indices_count = 0;
    bool is_1d = false;      //1d polygon represents line (or point)
    bool is_border = false;  //is the polygon on the border of a grid, filled with mark_border_cells procedure
  };

  //cell stores structural information, not the actual
  //points or attributes, thus should be treated as a
  //part of UnstructuredGrid, not as an independent entity
  struct LegacyCell
  {
    uint32_t polygons_offset = 0;
    uint16_t polygons_count = 0;
    VTKCellType type = VTK_EMPTY_CELL;
    bool is_border = false;  //is the cell on the border of a grid, filled with mark_border_cells procedure
  };

  static constexpr uint32_t MAX_POLYGON_COUNT = 16; // limited by border_bits size
  static constexpr uint32_t MAX_INDICES_COUNT = 64; // arbitrary limit

  struct PackedCell
  {
    uint32_t indices_offset = 0u;
    uint8_t indices_count = 0u;
    VTKCellType type = VTK_EMPTY_CELL;
    uint16_t border_bits = 0u; //border bit for each polygon, create FullCell to get polygons 
  };

  struct FullCell
  {
    VTKCellType type;
    uint32_t polygons_count = 0;
    uint32_t polygons_offsets[MAX_POLYGON_COUNT+1] = {0}; //polygons_offsets[polygons_count] is the end
    uint32_t indices[MAX_INDICES_COUNT] = {0};
  };

  void unpack_cell(PackedCell cell, const id_t *indices, FullCell *out_cell);

  std::string get_name(VTKCellType type);
  bool is_supported(VTKCellType type);
  bool is_2d_cell(VTKCellType type);
  bool is_3d_cell(VTKCellType type);
  LegacyCell create_cell(VTKCellType type, const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons,
                   std::vector<id_t> &all_points);
}
