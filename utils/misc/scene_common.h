#pragma once

#include "utils/mesh/mesh.h"

#include <vector>
#include <string>

struct ModelInfo
{
  std::string name;
  size_t bytesize;
  size_t num_primitives;
};

enum class DemoScene
{
  SINGLE_OBJECT,
  CORNELL_BOX,
  SINGLE_OBJECT_CUBEMAP,
  INSTANCES_GRID,
  INSTANCES_LARGE_SCENE,
  INSTANCES_100K,
  INSTANCES_ON_PLANE
};

void save_scene_xml(std::string path, std::string bin_file_name, ModelInfo info, int mat_id,
                    DemoScene scene = DemoScene::CORNELL_BOX); //for all models except mesh
void save_scene_xml(std::string path, std::string bin_file_name, const cmesh4::SimpleMesh &mesh,
                    DemoScene scene = DemoScene::CORNELL_BOX);