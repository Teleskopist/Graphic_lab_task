#pragma once
#include "mesh.h"
#include "mesh_load_obj.h"

//this header contains some internal functions of the mesh module
//they are not part of the public interface and should not be used
//outside this folder, unless you know what you're doing
namespace cmesh4
{
  bool check_watertight_mesh(const cmesh4::SimpleMesh& mesh, bool verbose = false);  //checks if mesh is watertight (it is required to build proper SDF from it)
  cmesh4::SimpleMesh removing_holes(cmesh4::SimpleMesh& mesh, int& ind, bool& fl);
  cmesh4::SimpleMesh before_removing_holes(cmesh4::SimpleMesh mesh, int& ind, bool& fl);
  void compress_close_vertices(cmesh4::SimpleMesh &mesh, double threshold, bool force_merge_distinct_normals = false, bool verbose = false);
  void fix_normals(cmesh4::SimpleMesh &mesh, bool verbose);
  void fix_missing(cmesh4::SimpleMesh &mesh, int default_mat_id = 0);  //fills missing normals, material id, texture coordinates and tangets with default values

  void load_mesh_array_from_obj(const char* a_fileName, std::vector<cmesh4::SimpleMesh>& meshes, 
                                std::vector<std::string> *mesh_names = nullptr, bool verbose = false);
}