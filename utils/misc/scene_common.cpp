#include "scene_common.h"

#include <cassert>
#include <fstream>
#include <filesystem>

using LiteMath::float3;
using LiteMath::float4;

std::string insert_in_demo_scene_cornell_box(const std::string &geom_info_str, const std::string &scenes_relative_path)
{
  constexpr unsigned MAX_BUF_SIZE = 8192;
  char buf[MAX_BUF_SIZE];

    snprintf(buf, MAX_BUF_SIZE, R""""(
    <?xml version="1.0"?>
    <textures_lib total_chunks="4">
      <texture id="0" name="Map#0" loc="%s/01_simple_scenes/data/chunk_00000.image4ub" offset="8" bytesize="16" width="2" height="2" dl="0" />
    </textures_lib>
    <materials_lib>
      <material id="0" name="mysimplemat" type="hydra_material">
        <diffuse brdf_type="lambert">
          <color val="0.5 0.5 0.5" />
        </diffuse>
      </material>
      <material id="1" name="red" type="hydra_material">
        <diffuse brdf_type="lambert">
          <color val="0.5 0.0 0.0" />
        </diffuse>
      </material>
      <material id="2" name="green" type="hydra_material">
        <diffuse brdf_type="lambert">
          <color val="0.0 0.5 0.0" />
        </diffuse>
      </material>
      <material id="3" name="white" type="hydra_material">
        <diffuse brdf_type="lambert">
          <color val="0.5 0.5 0.5" />
        </diffuse>
      </material>
      <material id="4" name="gold" type="hydra_material">
        <diffuse brdf_type="lambert">
          <color val="0.40 0.4 0" />
        </diffuse>
        <reflectivity brdf_type="ggx">
          <color val="0.20 0.20 0" />
          <glossiness val="0.750000024" />
        </reflectivity>
      </material>
      <material id="5" name="my_area_light_material" type="hydra_material" light_id="0" visible="1">
        <emission>
          <color val="25 25 25" />
        </emission>
      </material>
      <material id="6" name="Silver2_SG" type="hydra_material">
        <emission>
          <color val="0 0 0" />
          <cast_gi val="1" />
          <multiplier val="1" />
        </emission>
        <diffuse brdf_type="lambert">
          <color val="0.27 0.29 0.29" />
          <roughness val="0" />
        </diffuse>
        <reflectivity brdf_type="phong">
          <extrusion val="maxcolor" />
          <color val="0.85 0.85 0.85" />
          <glossiness val="0.8" />
          <fresnel val="1" />
          <fresnel_ior val="1.5" />
        </reflectivity>
      </material>
    </materials_lib>
    <geometry_lib total_chunks="4">
      %s
      <mesh id="1" name="my_box" type="vsgf" bytesize="1304" loc="%s/01_simple_scenes/data/cornell_open.vsgf" offset="0" vertNum="20" triNum="10" dl="0" path="" bbox="    -4 4 -4 4 -4 4">
        <positions type="array4f" bytesize="320" offset="24" apply="vertex" />
        <normals type="array4f" bytesize="320" offset="344" apply="vertex" />
        <tangents type="array4f" bytesize="320" offset="664" apply="vertex" />
        <texcoords type="array2f" bytesize="160" offset="984" apply="vertex" />
        <indices type="array1i" bytesize="120" offset="1144" apply="tlist" />
        <matindices type="array1i" bytesize="40" offset="1264" apply="primitive" />
      </mesh>
      <mesh id="2" name="my_area_light_lightmesh" type="vsgf" bytesize="280" loc="%s/01_simple_scenes/data/chunk_00003.vsgf" offset="0" vertNum="4" triNum="2" dl="0" path="" light_id="0" bbox="    -1 1 0 0 -1 1">
        <positions type="array4f" bytesize="64" offset="24" apply="vertex" />
        <normals type="array4f" bytesize="64" offset="88" apply="vertex" />
        <tangents type="array4f" bytesize="64" offset="152" apply="vertex" />
        <texcoords type="array2f" bytesize="32" offset="216" apply="vertex" />
        <indices type="array1i" bytesize="24" offset="248" apply="tlist" />
        <matindices type="array1i" bytesize="8" offset="272" apply="primitive" />
      </mesh>
    </geometry_lib>
    <lights_lib>
      <light id="0" name="my_area_light" type="area" shape="rect" distribution="diffuse" visible="1" mat_id="5" mesh_id="2">
        <size half_length="1" half_width="1" />
        <intensity>
          <color val="1 1 1" />
          <multiplier val="25" />
        </intensity>
      </light>
    </lights_lib>
    <cam_lib>
      <camera id="0" name="my camera" type="uvn">
        <fov>45</fov>
        <nearClipPlane>0.01</nearClipPlane>
        <farClipPlane>100.0</farClipPlane>
        <up>0 1 0</up>
        <position>0 0 14</position>
        <look_at>0 0 0</look_at>
      </camera>
    </cam_lib>
    <render_lib>
      <render_settings type="HydraModern" id="0">
        <width>512</width>
        <height>512</height>
        <method_primary>pathtracing</method_primary>
        <method_secondary>pathtracing</method_secondary>
        <method_tertiary>pathtracing</method_tertiary>
        <method_caustic>pathtracing</method_caustic>
        <trace_depth>3</trace_depth>
        <diff_trace_depth>4</diff_trace_depth>
        <maxRaysPerPixel>1024</maxRaysPerPixel>
        <qmc_variant>7</qmc_variant>
      </render_settings>
    </render_lib>
    <scenes>
      <scene id="0" name="my scene" discard="1" bbox="    -4 4 -4.137 4 -4 4">
        <remap_lists>
          <remap_list id="0" size="2" val="0 4 " />
        </remap_lists>
        <instance id="0" mesh_id="0" rmap_id="0" scn_id="0" scn_sid="0" matrix="3 0 0 0   0 3 0 -1.4   0 0 -3 0   0 0 0 1 " />
        <instance id="1" mesh_id="1" rmap_id="-1" scn_id="0" scn_sid="0" matrix="-1 0 -8.74228e-08 0 0 1 0 0 8.74228e-08 0 -1 0 0 0 0 1 " />
        <instance_light id="0" light_id="0" matrix="1 0 0 0 0 1 0 3.85 0 0 1 0 0 0 0 1 " lgroup_id="-1" />
        <instance id="2" mesh_id="2" rmap_id="-1" matrix="1 0 0 0 0 1 0 3.85 0 0 1 0 0 0 0 1 " light_id="0" linst_id="0" />
      </scene>
    </scenes>
    )"""",
    scenes_relative_path.c_str(), geom_info_str.c_str(), scenes_relative_path.c_str(), scenes_relative_path.c_str());

    return std::string(buf, strlen(buf));
}

std::string insert_in_demo_scene_single_object(std::string geom_info_str, const std::string &scenes_relative_path)
{
constexpr unsigned MAX_BUF_SIZE = 8192;
  char buf[MAX_BUF_SIZE];

  snprintf(buf, MAX_BUF_SIZE, R""""(
<?xml version="1.0"?>
<textures_lib total_chunks="4">
  <texture id="0" name="Map#0" loc="data/chunk_00000.image4ub" offset="8" bytesize="16" width="2" height="2" dl="0" />
</textures_lib>
<materials_lib>
  <material id="0" name="mysimplemat" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.5 0.5 0.5" />
    </diffuse>
  </material>
  <material id="1" name="red" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.5 0.0 0.0" />
    </diffuse>
  </material>
  <material id="2" name="green" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.0 0.5 0.0" />
    </diffuse>
  </material>
  <material id="3" name="white" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.5 0.5 0.5" />
    </diffuse>
  </material>
  <material id="4" name="gold" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.40 0.4 0" />
    </diffuse>
    <reflectivity brdf_type="torranse_sparrow">
      <color val="0.10 0.10 0" />
      <glossiness val="0.850000024" />
    </reflectivity>
  </material>
  <material id="5" name="my_area_light_material" type="hydra_material" light_id="0" visible="1">
    <emission>
      <color val="25 25 25" />
    </emission>
  </material>
  <material id="6" name="Silver2_SG" type="hydra_material">
    <emission>
      <color val="0 0 0" />
      <cast_gi val="1" />
      <multiplier val="1" />
    </emission>
    <diffuse brdf_type="lambert">
      <color val="0.27 0.29 0.29" />
      <roughness val="0" />
    </diffuse>
    <reflectivity brdf_type="phong">
      <extrusion val="maxcolor" />
      <color val="0.85 0.85 0.85" />
      <glossiness val="0.8" />
      <fresnel val="1" />
      <fresnel_ior val="1.5" />
    </reflectivity>
  </material>
</materials_lib>
<geometry_lib total_chunks="4">
  %s
</geometry_lib>
<lights_lib>
  <light id="0" name="my_area_light" type="area" shape="rect" distribution="diffuse" visible="1" mat_id="5" mesh_id="2">
    <size half_length="1" half_width="1" />
    <intensity>
      <color val="1 1 1" />
      <multiplier val="25" />
    </intensity>
  </light>
</lights_lib>
<cam_lib>
  <camera id="0" name="my camera" type="uvn">
    <fov>60</fov>
    <nearClipPlane>0.01</nearClipPlane>
    <farClipPlane>100.0</farClipPlane>
    <up>0 1 0</up>
    <position>0 0 3</position>
    <look_at>0 0 0</look_at>
  </camera>
</cam_lib>
<render_lib>
  <render_settings type="HydraModern" id="0">
    <width>512</width>
    <height>512</height>
    <method_primary>pathtracing</method_primary>
    <method_secondary>pathtracing</method_secondary>
    <method_tertiary>pathtracing</method_tertiary>
    <method_caustic>pathtracing</method_caustic>
    <trace_depth>3</trace_depth>
    <diff_trace_depth>3</diff_trace_depth>
    <maxRaysPerPixel>1024</maxRaysPerPixel>
    <qmc_variant>7</qmc_variant>
  </render_settings>
</render_lib>
<scenes>
  <scene id="0" name="my scene" discard="1" bbox="   -10 10 -4.137 0.7254 -10 10">
    <remap_lists>
      <remap_list id="0" size="2" val="0 4 " />
    </remap_lists>
    <instance id="0" mesh_id="0" rmap_id="0" scn_id="0" scn_sid="0" matrix="1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 " />
    <instance_light id="0" light_id="0" matrix="1 0 0 0 0 1 0 3.85 0 0 1 0 0 0 0 1 " lgroup_id="-1" />
  </scene>
</scenes>
    )"""",
           geom_info_str.c_str());

  return std::string(buf, strlen(buf));
}

std::string insert_in_demo_scene_single_object_cubemap(std::string geom_info_str, const std::string &scenes_relative_path)
{
constexpr unsigned MAX_BUF_SIZE = 8192;
  char buf[MAX_BUF_SIZE];

  snprintf(buf, MAX_BUF_SIZE, R""""(
<?xml version="1.0"?>
<textures_lib total_chunks="4">
  <texture id="0" name="Map#0" loc="data/chunk_00000.image4ub" offset="8" bytesize="16" width="2" height="2" dl="0" />
  <texture id="1" name="cubemap" path="%s/textures/23_antwerp_night.exr" loc="%s/textures/23_antwerp_night.exr" offset="8" bytesize="128000000" width="4000" height="2000" channels="4" dl="0" />
</textures_lib>
<materials_lib>
  <material id="0" name="mysimplemat" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.5 0.5 0.5" />
    </diffuse>
  </material>
  <material id="1" name="red" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.5 0.0 0.0" />
    </diffuse>
  </material>
  <material id="2" name="green" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.0 0.5 0.0" />
    </diffuse>
  </material>
  <material id="3" name="white" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.5 0.5 0.5" />
    </diffuse>
  </material>
  <material id="4" name="gold" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.40 0.4 0" />
    </diffuse>
    <reflectivity brdf_type="torranse_sparrow">
      <color val="0.10 0.10 0" />
      <glossiness val="0.850000024" />
    </reflectivity>
  </material>
  <material id="5" name="my_area_light_material" type="hydra_material" light_id="0" visible="1">
    <emission>
      <color val="25 25 25" />
    </emission>
  </material>
  <material id="6" name="Silver2_SG" type="hydra_material">
    <emission>
      <color val="0 0 0" />
      <cast_gi val="1" />
      <multiplier val="1" />
    </emission>
    <diffuse brdf_type="lambert">
      <color val="0.27 0.29 0.29" />
      <roughness val="0" />
    </diffuse>
    <reflectivity brdf_type="phong">
      <extrusion val="maxcolor" />
      <color val="0.85 0.85 0.85" />
      <glossiness val="0.8" />
      <fresnel val="1" />
      <fresnel_ior val="1.5" />
    </reflectivity>
  </material>
</materials_lib>
<geometry_lib total_chunks="4">
  %s
</geometry_lib>
<lights_lib>
  <light id="0" name="sky" type="sky" shape="point" distribution="map" visible="1" mat_id="2">
    <intensity>
      <color val="1 1 1">
        <texture id="1" type="texref" matrix="1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1" addressing_mode_u="wrap" addressing_mode_v="wrap" />
      </color>
      <multiplier val="1.0" />
    </intensity>
  </light>
</lights_lib>
<cam_lib>
  <camera id="0" name="my camera" type="uvn">
    <fov>60</fov>
    <nearClipPlane>0.01</nearClipPlane>
    <farClipPlane>100.0</farClipPlane>
    <up>0 1 0</up>
    <position>0 0 3</position>
    <look_at>0 0 0</look_at>
  </camera>
</cam_lib>
<render_lib>
  <render_settings type="HydraModern" id="0">
    <width>512</width>
    <height>512</height>
    <method_primary>pathtracing</method_primary>
    <method_secondary>pathtracing</method_secondary>
    <method_tertiary>pathtracing</method_tertiary>
    <method_caustic>pathtracing</method_caustic>
    <trace_depth>3</trace_depth>
    <diff_trace_depth>3</diff_trace_depth>
    <maxRaysPerPixel>1024</maxRaysPerPixel>
    <qmc_variant>7</qmc_variant>
  </render_settings>
</render_lib>
<scenes>
  <scene id="0" name="my scene" discard="1" bbox="   -10 10 -4.137 0.7254 -10 10">
    <remap_lists>
      <remap_list id="0" size="2" val="0 4 " />
    </remap_lists>
    <instance id="0" mesh_id="0" rmap_id="0" scn_id="0" scn_sid="0" matrix="1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 " />
    <instance_light id="0" light_id="0" matrix="1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 " lgroup_id="-1">
      <transform_sequence transformation="scale * rotation * position" rotation="Euler in dergees" />
    </instance_light>
  </scene>
</scenes>
    )"""",
           scenes_relative_path.c_str(), scenes_relative_path.c_str(), geom_info_str.c_str());

  return std::string(buf, strlen(buf));
}

std::string insert_in_demo_scene_instances_grid(std::string geom_info_str, const std::string &scenes_relative_path,
                                                int N_X, int N_Y, int N_Z, float dist, bool random_rotation,
                                                LiteMath::float4x4 base_transform = LiteMath::float4x4())
{
  std::string inst_full_string;
  constexpr unsigned INST_BUF_SIZE = 256;
  char inst_buf[INST_BUF_SIZE];
  for (unsigned i = 0; i < N_X; ++i)
  {
    for (unsigned j = 0; j < N_Y; ++j)
    {
      for (unsigned k = 0; k < N_Z; ++k)
      {
        float rnd_rot_y = (double)rand()/RAND_MAX * LiteMath::M_PI * 2;
        LiteMath::float4x4 rot_y = base_transform * LiteMath::rotate4x4Y(random_rotation ? (rnd_rot_y - LiteMath::M_PI ) : 0);
        float3 pos = dist*float3(i, j, k);
        snprintf(inst_buf, INST_BUF_SIZE, "<instance id=\"%d\" mesh_id=\"0\" rmap_id=\"0\" scn_id=\"0\" scn_sid=\"0\" \
                                          matrix=\"%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f 0 0 0 1 \" />\n",
                                          i*N_Y*N_Z + j*N_Z + k, 
                                          rot_y.col(0).x, rot_y.col(0).y, rot_y.col(0).z, pos.x,
                                          rot_y.col(1).x, rot_y.col(1).y, rot_y.col(1).z, pos.y,
                                          rot_y.col(2).x, rot_y.col(2).y, rot_y.col(2).z, pos.z);
      
        inst_full_string += std::string(inst_buf);
      }
    }
  }

  unsigned buf_size = 8192 + inst_full_string.size();
  char *buf = new char[buf_size];

  snprintf(buf, buf_size, R""""(
<?xml version="1.0"?>
<textures_lib total_chunks="4">
  <texture id="0" name="Map#0" loc="data/chunk_00000.image4ub" offset="8" bytesize="16" width="2" height="2" dl="0" />
</textures_lib>
<materials_lib>
  <material id="0" name="mysimplemat" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.5 0.5 0.5" />
    </diffuse>
  </material>
  <material id="1" name="red" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.5 0.0 0.0" />
    </diffuse>
  </material>
  <material id="2" name="green" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.0 0.5 0.0" />
    </diffuse>
  </material>
  <material id="3" name="white" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.5 0.5 0.5" />
    </diffuse>
  </material>
  <material id="4" name="gold" type="hydra_material">
    <diffuse brdf_type="lambert">
      <color val="0.40 0.4 0" />
    </diffuse>
    <reflectivity brdf_type="torranse_sparrow">
      <color val="0.10 0.10 0" />
      <glossiness val="0.850000024" />
    </reflectivity>
  </material>
  <material id="5" name="my_area_light_material" type="hydra_material" light_id="0" visible="1">
    <emission>
      <color val="25 25 25" />
    </emission>
  </material>
  <material id="6" name="Silver2_SG" type="hydra_material">
    <emission>
      <color val="0 0 0" />
      <cast_gi val="1" />
      <multiplier val="1" />
    </emission>
    <diffuse brdf_type="lambert">
      <color val="0.27 0.29 0.29" />
      <roughness val="0" />
    </diffuse>
    <reflectivity brdf_type="phong">
      <extrusion val="maxcolor" />
      <color val="0.85 0.85 0.85" />
      <glossiness val="0.8" />
      <fresnel val="1" />
      <fresnel_ior val="1.5" />
    </reflectivity>
  </material>
</materials_lib>
<geometry_lib total_chunks="4">
  %s
</geometry_lib>
<lights_lib>
  <light id="0" name="my_area_light" type="area" shape="rect" distribution="diffuse" visible="1" mat_id="5" mesh_id="2">
    <size half_length="1" half_width="1" />
    <intensity>
      <color val="1 1 1" />
      <multiplier val="25" />
    </intensity>
  </light>
</lights_lib>
<cam_lib>
  <camera id="0" name="my camera" type="uvn">
    <fov>60</fov>
    <nearClipPlane>0.01</nearClipPlane>
    <farClipPlane>100.0</farClipPlane>
    <up>0 1 0</up>
    <position>0 0 3</position>
    <look_at>0 0 0</look_at>
  </camera>
</cam_lib>
<render_lib>
  <render_settings type="HydraModern" id="0">
    <width>512</width>
    <height>512</height>
    <method_primary>pathtracing</method_primary>
    <method_secondary>pathtracing</method_secondary>
    <method_tertiary>pathtracing</method_tertiary>
    <method_caustic>pathtracing</method_caustic>
    <trace_depth>3</trace_depth>
    <diff_trace_depth>3</diff_trace_depth>
    <maxRaysPerPixel>1024</maxRaysPerPixel>
    <qmc_variant>7</qmc_variant>
  </render_settings>
</render_lib>
<scenes>
  <scene id="0" name="my scene" discard="1" bbox="   -10 10 -4.137 0.7254 -10 10">
    <remap_lists>
      <remap_list id="0" size="2" val="0 4 " />
    </remap_lists>
    %s
    <instance_light id="0" light_id="0" matrix="1 0 0 0 0 1 0 3.85 0 0 1 0 0 0 0 1 " lgroup_id="-1" />
  </scene>
</scenes>
    )"""",
           geom_info_str.c_str(), inst_full_string.c_str());

  std::string res = std::string(buf, strlen(buf));
  delete[] buf;

  return res;
}

std::string insert_in_demo_scene(std::string geom_info_str, const std::string &scenes_relative_path, DemoScene scene_type)
{
  switch (scene_type)
  {
  case DemoScene::CORNELL_BOX:
    return insert_in_demo_scene_cornell_box(geom_info_str, scenes_relative_path);
  case DemoScene::SINGLE_OBJECT:
    return insert_in_demo_scene_single_object(geom_info_str, scenes_relative_path);
  case DemoScene::SINGLE_OBJECT_CUBEMAP:
    return insert_in_demo_scene_single_object_cubemap(geom_info_str, scenes_relative_path);
  case DemoScene::INSTANCES_GRID:
    return insert_in_demo_scene_instances_grid(geom_info_str, scenes_relative_path, 32, 1, 32, 2.5f, false);
  case DemoScene::INSTANCES_LARGE_SCENE:
    return insert_in_demo_scene_instances_grid(geom_info_str, scenes_relative_path, 32, 32, 32, 4.0f, true);
  case DemoScene::INSTANCES_100K:
    return insert_in_demo_scene_instances_grid(geom_info_str, scenes_relative_path, 50, 40, 50, 4.0f, true);
  case DemoScene::INSTANCES_ON_PLANE:
    return insert_in_demo_scene_instances_grid(geom_info_str, scenes_relative_path, 50, 1, 50, 4.0f, true,
                                               LiteMath::make_float4x4_from_cols(float4(0,1,0,0), float4(0,0,1,0), float4(1,0,0,0), float4(0,0,0,1)));
  default:
    return "";
  }
}

std::string get_xml_string_model_demo_scene(std::string bin_file_name, std::string scenes_relative_path, ModelInfo info, int mat_id, DemoScene scene)
{
  constexpr unsigned MAX_BUF_SIZE = 1024;
  char buf[MAX_BUF_SIZE];

    snprintf(buf, MAX_BUF_SIZE, R""""(
    <%s id="0" name="demo_model" type="%s" bytesize="%zu" loc="%s" num_primitives="%zu" mat_id="%d">
    </%s>
    )"""",
    info.name.c_str(), info.name.c_str(), info.bytesize, bin_file_name.c_str(), info.num_primitives, mat_id, info.name.c_str());
  
  std::string geom_info_str = std::string(buf, strlen(buf));



  return insert_in_demo_scene(geom_info_str, scenes_relative_path, scene);
}

std::string get_xml_string_model_demo_scene(std::string bin_file_name, std::string scenes_relative_path, const cmesh4::SimpleMesh &mesh, DemoScene scene)
{
  constexpr unsigned MAX_BUF_SIZE = 1024;
  char buf[MAX_BUF_SIZE];

  unsigned bytesize = mesh.SizeInBytes();
  unsigned vertNum = mesh.VerticesNum();
  unsigned triNum = mesh.TrianglesNum();
  float3 bbox_min, bbox_max;
  cmesh4::get_bbox(mesh, &bbox_min, &bbox_max);
  unsigned positions_bytesize = 4*sizeof(float)*vertNum;
  unsigned normals_bytesize = 4*sizeof(float)*vertNum;
  unsigned tangents_bytesize = 4*sizeof(float)*vertNum;
  unsigned texcoords_bytesize = 2*sizeof(float)*vertNum;
  unsigned indices_bytesize = sizeof(unsigned)*triNum*3;
  unsigned matIndices_bytesize = sizeof(unsigned)*triNum;

  unsigned base_offset = 6*sizeof(unsigned);

    snprintf(buf, MAX_BUF_SIZE, R""""(
      <mesh id="0" name="demo_mesh" type="vsgf" bytesize="%d" loc="%s" offset="0" vertNum="%d" triNum="%d" dl="0" path="" bbox="%f %f %f %f %f %f">
        <positions type="array4f" bytesize="%d" offset="%d" apply="vertex" />
        <normals type="array4f" bytesize="%d" offset="%d" apply="vertex" />
        <tangents type="array4f" bytesize="%d" offset="%d" apply="vertex" />
        <texcoords type="array2f" bytesize="%d" offset="%d" apply="vertex" />
        <indices type="array1i" bytesize="%d" offset="%d" apply="tlist" />
        <matindices type="array1i" bytesize="%d" offset="%d" apply="primitive" />
      </mesh>
    )"""",
    bytesize, bin_file_name.c_str(), vertNum, triNum, bbox_min.x, bbox_max.x, bbox_min.y, bbox_max.y, bbox_min.z, bbox_max.z,
    positions_bytesize, base_offset, 
    normals_bytesize, base_offset+positions_bytesize,
    tangents_bytesize, base_offset+positions_bytesize+normals_bytesize,
    texcoords_bytesize, base_offset+positions_bytesize+normals_bytesize+tangents_bytesize,
    indices_bytesize, base_offset+positions_bytesize+normals_bytesize+tangents_bytesize+texcoords_bytesize,
    matIndices_bytesize, base_offset+positions_bytesize+normals_bytesize+tangents_bytesize+texcoords_bytesize+indices_bytesize
    );
  
  std::string geom_info_str = std::string(buf, strlen(buf));

  return insert_in_demo_scene(geom_info_str, scenes_relative_path, scene);
}

void save_xml_string(const std::string xml_string, const std::string &path)
{
  std::ofstream xml_file(path);
  xml_file << xml_string;
  xml_file.close();
}

std::string get_scenes_relative_path(std::string bin_file_name)
{
  std::string scenes_absolute_path = std::filesystem::current_path().string() + "/scenes";
  std::string xml_absolute_path = std::filesystem::absolute(std::filesystem::path(bin_file_name).parent_path()).string();
  std::string scenes_relative_path = std::filesystem::relative(scenes_absolute_path, xml_absolute_path).string();

  return scenes_relative_path;
}
void save_scene_xml(std::string path, std::string bin_file_name, ModelInfo info, int mat_id,
                    DemoScene scene)
{
  std::string scenes_relative_path = get_scenes_relative_path(path);
  save_xml_string(get_xml_string_model_demo_scene(bin_file_name, scenes_relative_path, info, mat_id, scene), path);
}

void save_scene_xml(std::string path, std::string bin_file_name, const cmesh4::SimpleMesh &mesh,
                    DemoScene scene)
{
  std::string scenes_relative_path = get_scenes_relative_path(path);
  save_xml_string(get_xml_string_model_demo_scene(bin_file_name, scenes_relative_path, mesh, scene), path);
}
