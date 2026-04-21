#pragma once
#include "LiteScene/scene.h"
#include "core/render_settings.h"
#include "utils/mesh/augmented_mesh.h"

#include <string>
#include <locale>

struct MultiRendererMaterial;

namespace LiteScene
{
#define REGISTER_TYPE(typeId, GeometryClassName, GeometryData, xml_name, save_func, load_func, get_info_func) \
  class GeometryClassName : public Geometry\
  {\
  public:\
    static std::string get_type_name() { return xml_name; }\
    virtual ~GeometryClassName() {}\
    bool load_node(pugi::xml_node node) override\
    {\
      bool ok = load_node_base(node);\
      if (!ok) return false;\
      if (node.attribute(L"loc").empty())\
      {\
        relative_file_path = INVALID_PATH;\
        printf("["#GeometryClassName"::load_node] No location\n");\
        return false;\
      }\
      relative_file_path = ws2s(node.attribute(L"loc").as_string());\
      type_id = typeId;\
      return true;\
    }\
    bool save_node(pugi::xml_node &node) const override\
    {\
      if (relative_file_path == INVALID_PATH)\
      {\
        printf("["#GeometryClassName"::save_node] No location is specified. Save data first\n");\
        return false;\
      }\
      save_node_base(node);\
      node.set_name(L##xml_name);\
      auto loc = s2ws(relative_file_path);\
      if (node.attribute(L"loc").empty()) \
        node.append_attribute(L"loc").set_value(loc.c_str());\
      else \
        node.attribute(L"loc").set_value(loc.c_str());\
      return true;\
    }\
    bool load_data(const SceneMetadata &metadata) override\
    {\
      if (is_loaded)\
        return true;\
      if (relative_file_path == INVALID_PATH)\
      {\
        printf("["#GeometryClassName"load_data] No location is specified. Load node first\n");\
        return false;\
      }\
      std::string path = metadata.scene_xml_folder + "/" + relative_file_path;\
      load_func(model, path.c_str());\
      bool ok = true;\
      if (!ok)\
      {\
        printf("["#GeometryClassName"::load_data] Failed to load model %s\n", path.c_str());\
        return false;\
      }\
      is_loaded = true;\
      return true;\
    }\
    bool save_data(const SceneMetadata &metadata) override\
    {\
      if (!is_loaded)\
      {\
        printf("["#GeometryClassName"::save_data]Model is not loaded\n");\
        return false;\
      }\
      std::string filename = std::string(xml_name)+"_" + std::to_string(id);\
      relative_file_path = metadata.geometry_folder_relative + "/" + filename + ".bin";\
      std::string file_path = metadata.scene_xml_folder == "" ? relative_file_path : \
                                                                metadata.scene_xml_folder + "/" + relative_file_path;\
      save_func(model, file_path.c_str());\
      return true;\
    }\
    void init(const GeometryData &_model)\
    {\
      model = _model;\
      name = "unnamed_"+std::string(xml_name); \
      relative_file_path = std::string(xml_name)+"_"+std::to_string(id) + ".bin"; \
      type_name = xml_name; \
      bytesize = get_info_func(model).bytesize; \
      custom_data.append_attribute(L"num_primitives").set_value(get_info_func(model).num_primitives); \
      is_loaded = true; \
    } \
    void init(const GeometryData &&_model)\
    {\
      model = std::move(_model);\
      name = "unnamed_"+std::string(xml_name); \
      relative_file_path = std::string(xml_name)+"_"+std::to_string(id) + ".bin"; \
      type_name = xml_name; \
      bytesize = get_info_func(model).bytesize; \
      custom_data.append_attribute(L"num_primitives").set_value(get_info_func(model).num_primitives); \
      is_loaded = true; \
    } \
    bool is_loaded = false;\
    std::string relative_file_path = INVALID_PATH;\
    GeometryData model; /*empty when not loaded*/\
  };

  #define cast_or_create(T, var, metadata, custom_node) \
  std::unique_ptr<T> __m_##var;\
  T*var = dynamic_cast<T*>(custom_node); \
  if (!var) {\
    __m_##var = std::make_unique<T>(); \
    __m_##var->load_node(custom_node->custom_data); \
    var = __m_##var.get();\
  }\
  var->load_data(metadata);

  REGISTER_TYPE(TYPE_MESH_TRIANGLE   , AugmentedMeshGeometry,  cmesh4::AugmentedMesh,           "augmented_mesh",   save_augmented_mesh,   load_augmented_mesh,   get_info_augmented_mesh)
  
  // material that is used to indicate that is wasn't loaded properly
  MultiRendererMaterial get_error_material();

  // material with some default values in case not all values are present or needed
  MultiRendererMaterial get_default_material();

  // load material from the scene
  MultiRendererMaterial load_material(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap);
}