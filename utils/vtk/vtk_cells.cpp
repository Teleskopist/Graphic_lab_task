#include "vtk_cells.h"

namespace vtk
{
  const char* vtkCellTypesStrings[] = { "vtkEmptyCell", "vtkVertex", "vtkPolyVertex", "vtkLine",
  "vtkPolyLine", "vtkTriangle", "vtkTriangleStrip", "vtkPolygon", "vtkPixel", "vtkQuad", "vtkTetra",
  "vtkVoxel", "vtkHexahedron", "vtkWedge", "vtkPyramid", "vtkPentagonalPrism", "vtkHexagonalPrism",
  "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass", "vtkQuadraticEdge",
  "vtkQuadraticTriangle", "vtkQuadraticQuad", "vtkQuadraticTetra", "vtkQuadraticHexahedron",
  "vtkQuadraticWedge", "vtkQuadraticPyramid", "vtkBiQuadraticQuad", "vtkTriQuadraticHexahedron",
  "vtkQuadraticLinearQuad", "vtkQuadraticLinearWedge", "vtkBiQuadraticQuadraticWedge",
  "vtkBiQuadraticQuadraticHexahedron", "vtkBiQuadraticTriangle", "vtkCubicLine",
  "vtkQuadraticPolygon", "vtkTriQuadraticPyramid", "UnknownClass", "UnknownClass", "UnknownClass",
  "vtkConvexPointSet", "vtkPolyhedron", "UnknownClass", "UnknownClass", "UnknownClass",
  "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass", "UnknownClass",
  "vtkParametricCurve", "vtkParametricSurface", "vtkParametricTriSurface",
  "vtkParametricQuadSurface", "vtkParametricTetraRegion", "vtkParametricHexRegion", "UnknownClass",
  "UnknownClass", "UnknownClass", "vtkHigherOrderEdge", "vtkHigherOrderTriangle",
  "vtkHigherOrderQuad", "vtkHigherOrderPolygon", "vtkHigherOrderTetrahedron", "vtkHigherOrderWedge",
  "vtkHigherOrderPyramid", "vtkHigherOrderHexahedron", "vtkLagrangeCurve",
  "vtkLagrangeQuadrilateral", "vtkLagrangeTriangle", "vtkLagrangeTetra", "vtkLagrangeHexahedron",
  "vtkLagrangeWedge", "vtkLagrangePyramid", "vtkBezierCurve", "vtkBezierQuadrilateral",
  "vtkBezierTriangle", "vtkBezierTetra", "vtkBezierHexahedron", "vtkBezierWedge",
  "vtkBezierPyramid", nullptr };

  std::string get_name(VTKCellType type)
  {
    return vtkCellTypesStrings[(int)type];
  }

  bool is_supported(VTKCellType type)
  {
    if (type == VTK_TRIANGLE ||
        type == VTK_TRIANGLE_STRIP ||
        type == VTK_POLYGON ||
        type == VTK_PIXEL ||
        type == VTK_QUAD ||
        type == VTK_TETRA ||
        type == VTK_VOXEL ||
        type == VTK_HEXAHEDRON ||
        type == VTK_WEDGE ||
        type == VTK_PYRAMID ||
        type == VTK_PENTAGONAL_PRISM ||
        type == VTK_HEXAGONAL_PRISM)
      return true;
    
    return false;
  }

  bool is_2d_cell(VTKCellType type)
  {
    return (int)type >= (int)VTK_TRIANGLE &&
           (int)type <= (int)VTK_QUAD;
  }

  bool is_3d_cell(VTKCellType type)
  {
    return (int)type >= (int)VTK_TETRA &&
           (int)type <= (int)VTK_HEXAGONAL_PRISM;
  }

  LegacyCell create_cell_triangle(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count != 3)
    {
      printf("Triangle must have 3 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_TRIANGLE;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 1;

    all_polygons.push_back({(id_t)all_points.size(), 3});

    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[2]);

    return cell;
  }

  LegacyCell create_cell_triangle_strip(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count < 3)
    {
      printf("Triangle strip must have at least 3 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_TRIANGLE_STRIP;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = indices_count - 2;

    for (int i = 0; i < indices_count - 2; i++)
    {
      all_polygons.push_back({(id_t)all_points.size(), 3});

      all_points.push_back(indices[i]);
      all_points.push_back(indices[i+1]);
      all_points.push_back(indices[i+2]);
    }
    return cell;
  }

  LegacyCell create_cell_polygon(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count < 3)
    {
      printf("Polygon must have at least 3 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_POLYGON;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 1;

    all_polygons.push_back({(id_t)all_points.size(), (uint16_t)indices_count});

    for (int i = 0; i < indices_count; i++)
      all_points.push_back(indices[i]);

    return cell;
  }

  LegacyCell create_cell_pixel(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count != 4)
    {
      printf("Pixel must have 4 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_PIXEL;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 1;

    all_polygons.push_back({(id_t)all_points.size(), 4});

    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[2]);

    return cell;
  }

  LegacyCell create_cell_quad(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count != 4)
    {
      printf("Quad must have 4 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_QUAD;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 1;

    all_polygons.push_back({(id_t)all_points.size(), 4});

    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[3]);

    return cell;
  }

  LegacyCell create_cell_tetrahedron(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count != 4)
    {
      printf("Tetrahedron must have 4 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_TETRA;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 4;

    all_polygons.push_back({(id_t)all_points.size(), 3});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[2]);

    all_polygons.push_back({(id_t)all_points.size(), 3});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[1]);

    all_polygons.push_back({(id_t)all_points.size(), 3});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[3]);

    all_polygons.push_back({(id_t)all_points.size(), 3});
    all_points.push_back(indices[1]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[2]);

    return cell;
  }

  LegacyCell create_cell_voxel(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count != 8)
    {
      printf("Voxel must have 8 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_VOXEL;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 6;

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[2]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[4]);
    all_points.push_back(indices[5]);
    all_points.push_back(indices[7]);
    all_points.push_back(indices[6]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[5]);
    all_points.push_back(indices[4]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[1]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[7]);
    all_points.push_back(indices[5]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[3]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[6]);
    all_points.push_back(indices[7]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[2]);
    all_points.push_back(indices[0]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[6]);

    return cell;
  }

  LegacyCell create_cell_hexahedron(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count != 8)
    {
      printf("Hexahedron must have exactly 8 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_HEXAHEDRON;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 6;

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[1]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[4]);
    all_points.push_back(indices[5]);
    all_points.push_back(indices[6]);
    all_points.push_back(indices[7]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[5]);
    all_points.push_back(indices[4]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[1]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[6]);
    all_points.push_back(indices[5]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[2]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[7]);
    all_points.push_back(indices[6]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[3]);
    all_points.push_back(indices[0]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[7]);

    return cell;
  }

  LegacyCell create_cell_wedge(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count != 6)
    {
      printf("Wedge must have 6 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_WEDGE;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 5;

    all_polygons.push_back({(id_t)all_points.size(), 3});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[2]);

    all_polygons.push_back({(id_t)all_points.size(), 3});
    all_points.push_back(indices[3]);
    all_points.push_back(indices[5]);
    all_points.push_back(indices[4]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[1]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[5]);
    all_points.push_back(indices[2]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[5]);
    all_points.push_back(indices[3]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[1]);

    return cell;
  }

  LegacyCell create_cell_pyramid(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count != 5)
    {
      printf("Pyramid must have 5 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_PYRAMID;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 5;

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[3]);

    all_polygons.push_back({(id_t)all_points.size(), 3});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[1]);

    all_polygons.push_back({(id_t)all_points.size(), 3});
    all_points.push_back(indices[1]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[2]);

    all_polygons.push_back({(id_t)all_points.size(), 3});
    all_points.push_back(indices[2]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[3]);

    all_polygons.push_back({(id_t)all_points.size(), 3});
    all_points.push_back(indices[3]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[0]);

    return cell;
  }

  LegacyCell create_cell_pentagonal_prism(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count != 10)
    {
      printf("PentagonalPrism must have 10 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_PENTAGONAL_PRISM;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 7;

    all_polygons.push_back({(id_t)all_points.size(), 5});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[4]);

    all_polygons.push_back({(id_t)all_points.size(), 5});
    all_points.push_back(indices[5]);
    all_points.push_back(indices[6]);
    all_points.push_back(indices[7]);
    all_points.push_back(indices[8]);
    all_points.push_back(indices[9]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[6]);
    all_points.push_back(indices[5]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[1]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[7]);
    all_points.push_back(indices[6]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[2]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[8]);
    all_points.push_back(indices[7]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[3]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[9]);
    all_points.push_back(indices[8]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[4]);
    all_points.push_back(indices[0]);
    all_points.push_back(indices[5]);
    all_points.push_back(indices[9]);

    return cell;
  }

  LegacyCell create_cell_hexagonal_prism(const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    if (indices_count != 12)
    {
      printf("HexagonalPrism must have 12 indices\n");
      return LegacyCell();
    }

    LegacyCell cell;
    cell.type = VTK_HEXAGONAL_PRISM;
    cell.polygons_offset = all_polygons.size();
    cell.polygons_count = 8;

    all_polygons.push_back({(id_t)all_points.size(), 6});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[5]);

    all_polygons.push_back({(id_t)all_points.size(), 6});
    all_points.push_back(indices[6]);
    all_points.push_back(indices[7]);
    all_points.push_back(indices[8]);
    all_points.push_back(indices[9]);
    all_points.push_back(indices[10]);
    all_points.push_back(indices[11]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[0]);
    all_points.push_back(indices[1]);
    all_points.push_back(indices[7]);
    all_points.push_back(indices[6]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[1]);
    all_points.push_back(indices[2]);
    all_points.push_back(indices[8]);
    all_points.push_back(indices[7]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[2]);
    all_points.push_back(indices[3]);
    all_points.push_back(indices[9]);
    all_points.push_back(indices[8]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[3]);
    all_points.push_back(indices[4]);
    all_points.push_back(indices[10]);
    all_points.push_back(indices[9]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[4]);
    all_points.push_back(indices[5]);
    all_points.push_back(indices[11]);
    all_points.push_back(indices[10]);

    all_polygons.push_back({(id_t)all_points.size(), 4});
    all_points.push_back(indices[5]);
    all_points.push_back(indices[0]);
    all_points.push_back(indices[6]);
    all_points.push_back(indices[11]);

    return cell;
  }
  
  LegacyCell create_cell(VTKCellType type, const id_t *indices, int indices_count,
                   std::vector<Polygon> &all_polygons, std::vector<id_t> &all_points)
  {
    static bool cell_debug[VTK_NUMBER_OF_CELL_TYPES] = {false};
    switch (type)
    {
    case VTK_TRIANGLE:
      return create_cell_triangle(indices, indices_count, all_polygons, all_points);
    case VTK_TRIANGLE_STRIP:
      return create_cell_triangle_strip(indices, indices_count, all_polygons, all_points);  
    case VTK_POLYGON:
      return create_cell_polygon(indices, indices_count, all_polygons, all_points);
    case VTK_QUAD:
      return create_cell_quad(indices, indices_count, all_polygons, all_points);
    case VTK_TETRA:
      return create_cell_tetrahedron(indices, indices_count, all_polygons, all_points);
    case VTK_VOXEL:
      return create_cell_voxel(indices, indices_count, all_polygons, all_points);
    case VTK_HEXAHEDRON:
      return create_cell_hexahedron(indices, indices_count, all_polygons, all_points);
    case VTK_WEDGE:
      return create_cell_wedge(indices, indices_count, all_polygons, all_points);
    case VTK_PYRAMID:
      return create_cell_pyramid(indices, indices_count, all_polygons, all_points);
    case VTK_PENTAGONAL_PRISM:
      return create_cell_pentagonal_prism(indices, indices_count, all_polygons, all_points);
    case VTK_HEXAGONAL_PRISM:
      return create_cell_hexagonal_prism(indices, indices_count, all_polygons, all_points);
    default:
      if (cell_debug[(int)type] == false)
      {
        cell_debug[(int)type] = true;
        printf("ERROR: Unsupported cell type: %d\n", (int)type);
      }
    }
    LegacyCell c;
    c.type = type;
    return c;
  }

  #define ADD_POLY_3(i1,i2,i3) \
   {fc.polygons_offsets[cur_poly++] = cur_idx; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i1]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i2]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i3];}
  
  #define ADD_POLY_4(i1,i2,i3,i4) \
   {fc.polygons_offsets[cur_poly++] = cur_idx; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i1]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i2]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i3]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i4];}
  
  #define ADD_POLY_5(i1,i2,i3,i4,i5) \
   {fc.polygons_offsets[cur_poly++] = cur_idx; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i1]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i2]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i3]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i4]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i5];}
  
  #define ADD_POLY_6(i1,i2,i3,i4,i5,i6) \
   {fc.polygons_offsets[cur_poly++] = cur_idx; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i1]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i2]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i3]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i4]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i5]; \
    fc.indices[cur_idx++] = indices[cell.indices_offset + i6];}

  void unpack_cell(PackedCell cell, const id_t *indices, FullCell *out_cell)
  {
    static bool cell_debug[VTK_NUMBER_OF_CELL_TYPES] = {false};

    FullCell &fc = *out_cell;
    fc.type = cell.type;
    fc.polygons_count = 0;

    //used in macros
    unsigned cur_poly = 0;
    unsigned cur_idx  = 0;

    switch (cell.type)
    {
    case VTK_TRIANGLE:
      fc.polygons_count = 1;
      ADD_POLY_3(0,1,2);
      break;
    case VTK_TRIANGLE_STRIP:
      if (cell.indices_count > MAX_INDICES_COUNT || cell.indices_count - 2 > MAX_POLYGON_COUNT)
        printf("[vtk::unpack_cell] VTK_TRIANGLE_STRIP cell has too many triangles\n");
      fc.polygons_count = std::min<uint32_t>(cell.indices_count - 2, MAX_POLYGON_COUNT);
      for (int i=0;i<fc.polygons_count;i++)
        ADD_POLY_3(i, i+1, i+2);
      break;
    case VTK_POLYGON:
      if (cell.indices_count > MAX_INDICES_COUNT)
        printf("[vtk::unpack_cell] VTK_POLYGON cell has too many points\n");
      fc.polygons_count = 1;
      fc.polygons_offsets[cur_poly++] = cur_idx;
      for (int i=0;i<std::min<uint32_t>(cell.indices_count, MAX_INDICES_COUNT);i++)
        fc.indices[cur_idx++] = indices[cell.indices_offset + i];
      break;
    case VTK_PIXEL:
      fc.polygons_count = 1;
      ADD_POLY_4(0,1,3,2);
      break;
    case VTK_QUAD:
      fc.polygons_count = 1;
      ADD_POLY_4(0,1,2,3);
      break;
    case VTK_TETRA:
      fc.polygons_count = 4;
      ADD_POLY_3(0,1,2);
      ADD_POLY_3(0,3,1);
      ADD_POLY_3(0,2,3);
      ADD_POLY_3(1,3,2);
      break;
    case VTK_VOXEL:
      fc.polygons_count = 6;
      ADD_POLY_4(0,1,3,2);
      ADD_POLY_4(4,5,7,6);
      ADD_POLY_4(0,1,5,4);
      ADD_POLY_4(1,3,7,5);
      ADD_POLY_4(3,2,6,7);
      ADD_POLY_4(2,0,4,6);
      break;
    case VTK_HEXAHEDRON:
      fc.polygons_count = 6;
      ADD_POLY_4(0,3,2,1);
      ADD_POLY_4(4,5,6,7);
      ADD_POLY_4(0,1,5,4);
      ADD_POLY_4(1,2,6,5);
      ADD_POLY_4(2,3,7,6);
      ADD_POLY_4(3,0,4,7);
      break;
    case VTK_WEDGE:
      fc.polygons_count = 5;
      ADD_POLY_3(0,1,2);
      ADD_POLY_3(3,5,4);
      ADD_POLY_4(1,4,5,2);
      ADD_POLY_4(0,2,5,3);
      ADD_POLY_4(0,3,4,1);
      break;
    case VTK_PYRAMID:
      fc.polygons_count = 5;
      ADD_POLY_4(0,1,2,3);
      ADD_POLY_3(0,4,1);
      ADD_POLY_3(1,4,2);
      ADD_POLY_3(2,4,3);
      ADD_POLY_3(3,4,0);
      break;
    case VTK_PENTAGONAL_PRISM:
      fc.polygons_count = 7;
      ADD_POLY_5(0,1,2,3,4);
      ADD_POLY_5(5,6,7,8,9);
      ADD_POLY_4(0,1,6,5);
      ADD_POLY_4(1,2,7,6);
      ADD_POLY_4(2,3,8,7);
      ADD_POLY_4(3,4,9,8);
      ADD_POLY_4(4,0,5,9);
      break;
    case VTK_HEXAGONAL_PRISM:
      fc.polygons_count = 8;
      ADD_POLY_6(0,1,2,3,4,5);
      ADD_POLY_6(6,7,8,9,10,11);
      ADD_POLY_4(0,1,7,6);
      ADD_POLY_4(1,2,8,7);
      ADD_POLY_4(2,3,9,8);
      ADD_POLY_4(3,4,10,9);
      ADD_POLY_4(4,5,11,10);
      ADD_POLY_4(5,0,6,11);
      break;
    default:
      if (cell_debug[(int)cell.type] == false)
      {
        cell_debug[(int)cell.type] = true;
        printf("ERROR: Unsupported cell type: %d\n", (int)cell.type);
      }
      break;
    }

    fc.polygons_offsets[cur_poly] = cur_idx; //last offset is a total index count
  }
}