#include "LiteScene/hydraxml.h"
#include "utils/mesh/mesh.h"
#include "sdf/scom2/scom.h"
#include "sdf/multi/sdf_multi.h"
#include "utils/common/data_channel.h"

#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

// Remaps material IDs in binary geometry files
bool remap_mat_id(const std::string &old_bin_path, const std::string &new_bin_path, const std::string &geometry_type,
                  int mat_id_offset);

/**
 * @brief Merges multiple scene XML files and their associated assets into a single scene
 * 
 * @param in_folders Vector of folder paths, each containing a single .xml file and associated assets
 * @param out_folder Output folder path where the merged scene will be saved
 * @return true if merge was successful, false otherwise
 */
bool merge_scenes(const std::vector<std::string> &in_folders, const std::string &out_folder) {
    namespace fs = std::filesystem;
    
    // Validate input
    if (in_folders.empty() || out_folder.empty()) {
        return false;
    }
    
    // Create output folder if it doesn't exist
    std::error_code ec;
    if (!fs::exists(out_folder)) {
        if (!fs::create_directories(out_folder, ec)) {
            return false;
        }
    }
    
    // Initialize the merged document with XML declaration
    pugi::xml_document merged_doc;
    auto decl = merged_doc.prepend_child(pugi::node_declaration);
    decl.append_attribute(L"version") = "1.0";
    
    // Create the required top-level nodes in the merged document
    auto merged_textures = merged_doc.append_child(L"textures_lib");
    auto merged_materials = merged_doc.append_child(L"materials_lib");
    auto merged_geometry = merged_doc.append_child(L"geometry_lib");
    auto merged_lights = merged_doc.append_child(L"lights_lib");
    auto merged_cam = merged_doc.append_child(L"cam_lib");
    auto merged_render = merged_doc.append_child(L"render_lib");
    auto merged_scenes = merged_doc.append_child(L"scenes");
    
    // Create the single scene node that will contain all instances
    auto merged_scene = merged_scenes.append_child(L"scene");
    merged_scene.append_attribute(L"id").set_value(L"0");
    merged_scene.append_attribute(L"bbox").set_value(L"-1 -1 -1 1 1 1");
    merged_scene.append_attribute(L"name").set_value(L"");
    
    // Counters for tracking global indices across all scenes
    int total_materials = 0;
    int total_geometries = 0;
    int total_instances = 0;
    
    // Process each input folder sequentially
    for (size_t folder_idx = 0; folder_idx < in_folders.size(); ++folder_idx) {
        const std::string &folder = in_folders[folder_idx];
        
        // Validate folder exists and is a directory
        if (!fs::exists(folder) || !fs::is_directory(folder)) {
            return false;
        }
        
        // Find the XML file in the current folder
        std::string xml_file_path;
        for (const auto &entry : fs::directory_iterator(folder)) {
            if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                xml_file_path = entry.path().string();
                break;
            }
        }
        
        if (xml_file_path.empty()) {
            return false; // No XML file found in folder
        }
        
        // Load the current scene's XML document
        pugi::xml_document current_doc;
        if (!current_doc.load_file(xml_file_path.c_str())) {
            return false; // Failed to parse XML
        }
        
        // Store material offset for this scene (for binary file remapping)
        int mat_id_offset = total_materials;
        int geometry_offset = total_geometries;
        
        // Process textures_lib - concatenate all texture nodes
        auto textures_lib = current_doc.child(L"textures_lib");
        if (textures_lib) {
            for (auto texture : textures_lib.children()) {
                merged_textures.append_copy(texture);
            }
        }
        
        // Process materials_lib - concatenate and renumber material IDs
        auto materials_lib = current_doc.child(L"materials_lib");
        if (materials_lib) {
            for (auto material : materials_lib.children()) {
                auto new_material = merged_materials.append_copy(material);
                // Update the material ID to maintain global uniqueness
                if (auto id_attr = new_material.attribute(L"id")) {
                    id_attr.set_value(total_materials);
                }
                total_materials++;
            }
        }
        
        // Process geometry_lib - this must not be empty per requirements
        auto geometry_lib = current_doc.child(L"geometry_lib");
        if (!geometry_lib) {
            return false; // geometry_lib node is missing
        }
        
        // Count geometries in current scene for validation
        int current_scene_geometries = 0;
        for (auto geometry : geometry_lib.children()) {
            current_scene_geometries++;
        }
        
        if (current_scene_geometries == 0) {
            return false; // geometry_lib must not be empty
        }
        
        // Process each geometry object
        for (auto geometry : geometry_lib.children()) {
            auto new_geometry = merged_geometry.append_copy(geometry);
            
            // Update geometry ID to match its position in the merged list
            if (auto id_attr = new_geometry.attribute(L"id")) {
                id_attr.set_value(total_geometries);
            }
            
            // Handle geometry file copying and material ID remapping
            auto loc_attr = new_geometry.attribute(L"loc");
            auto type_attr = new_geometry.attribute(L"type");
            
            if (loc_attr && type_attr) {
                std::string old_loc = hydra_xml::ws2s(loc_attr.value());
                std::string geometry_type = hydra_xml::ws2s(type_attr.value());
                
                // Remove relative path prefix if present
                if (old_loc.length() >= 2 && old_loc.substr(0, 2) == "./") {
                    old_loc = old_loc.substr(2);
                }
                
                // Construct paths
                fs::path source_path = fs::path(folder) / old_loc;
                std::string new_filename = "geometry_" + std::to_string(total_geometries) + 
                                          fs::path(old_loc).extension().string();
                fs::path dest_path = fs::path(out_folder) / new_filename;
                
                // Handle binary files that may contain material IDs
                if (geometry_type == "scom2" || geometry_type == "vsgf") {
                    // Use the provided remap function for these geometry types
                    if (!remap_mat_id(source_path.string(), dest_path.string(), geometry_type, mat_id_offset)) {
                        return false;
                    }
                } else {
                    // For other geometry types, just copy the file
                    if (!fs::copy_file(source_path, dest_path, ec)) {
                        return false;
                    }
                }
                
                // Update the location attribute to point to the new file
                loc_attr.set_value(hydra_xml::s2ws("./" + new_filename).c_str());
            }
            
            total_geometries++;
        }
        
        // Process lights_lib - concatenate all light nodes
        auto lights_lib = current_doc.child(L"lights_lib");
        if (lights_lib) {
            for (auto light : lights_lib.children()) {
                merged_lights.append_copy(light);
            }
        }
        
        // Process cam_lib - concatenate all camera nodes
        auto cam_lib = current_doc.child(L"cam_lib");
        if (cam_lib) {
            for (auto camera : cam_lib.children()) {
                merged_cam.append_copy(camera);
            }
        }
        
        // Process render_lib - concatenate all render settings
        auto render_lib = current_doc.child(L"render_lib");
        if (render_lib) {
            for (auto render_setting : render_lib.children()) {
                merged_render.append_copy(render_setting);
            }
        }
        
        // Process scenes node - must contain exactly one scene child
        auto scenes_node = current_doc.child(L"scenes");
        if (!scenes_node) {
            return false; // scenes node is missing
        }
        
        auto scene_node = scenes_node.child(L"scene");
        if (!scene_node) {
            return false; // scene child is missing
        }
        
        // Process all instance nodes in the current scene
        for (auto instance : scene_node.children(L"instance")) {
            auto new_instance = merged_scene.append_copy(instance);
            
            // Update instance ID to maintain global order
            if (auto id_attr = new_instance.attribute(L"id")) {
                id_attr.set_value(total_instances);
            }
            
            // Update mesh_id to reference the correct geometry in the merged scene
            if (auto mesh_id_attr = new_instance.attribute(L"mesh_id")) {
                int original_mesh_id = mesh_id_attr.as_int();
                int new_mesh_id = geometry_offset + original_mesh_id;
                mesh_id_attr.set_value(new_mesh_id);
            }
            
            total_instances++;
        }
        
        // Copy additional files (textures, etc.) from the current folder
        // Skip XML files and binary geometry files (already handled above)
        for (const auto &entry : fs::directory_iterator(folder)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string extension = entry.path().extension().string();
                
                // Skip XML files and geometry binary files
                if (extension != ".xml" && 
                    filename.find("scom2_") != 0 && 
                    filename.find("mesh_") != 0 &&
                    extension != ".bin" && 
                    extension != ".vsgf") {
                    
                    fs::path dest_file = fs::path(out_folder) / filename;
                    // Only copy if destination doesn't exist to avoid conflicts
                    if (!fs::exists(dest_file)) {
                        fs::copy_file(entry.path(), dest_file, ec);
                        // Don't return false here - texture copying failures are not critical
                    }
                }
            }
        }
    }
    
    // Save the merged XML document to the output folder
    fs::path output_xml_path = fs::path(out_folder) / "scene.xml";
    if (!merged_doc.save_file(output_xml_path.wstring().c_str())) {
        return false;
    }
    
    return true;
}


bool remap_mat_id_mesh(const std::string &old_bin_path, const std::string &new_bin_path, int mat_id_offset)
{
  auto mesh = cmesh4::LoadMeshFromVSGF(old_bin_path.c_str());
  if (mesh.indices.size() == 0 || mesh.vPos4f.size() == 0)
    return false;
  printf("remap mesh with %d indices, offset = %d\n", (int)mesh.indices.size(), mat_id_offset);
  for (auto &matId : mesh.matIndices)
    matId += mat_id_offset;
  cmesh4::SaveMeshToVSGF(new_bin_path.c_str(), mesh);
  return true;
}

bool remap_mat_id_scom2(const std::string &old_bin_path, const std::string &new_bin_path, int mat_id_offset) 
{
  SCom2Tree tree;
  load_scom2(tree, old_bin_path);
  if (tree.bricks.size() == 0 || tree.voxel_channels.size() == 0)
    return false;
  
  if (tree.header.mat_id_off >= 0)
  {
    printf("remap scom2 with %d voxels, offset = %d\n", (int)tree.voxel_channels[tree.header.mat_id_off].data_i.size(), mat_id_offset);
    assert(tree.voxel_channels.size() > tree.header.mat_id_off);
    assert(tree.voxel_channels[tree.header.mat_id_off].type == DataChannel::Type::INT);
    static_assert(sizeof(int) == sizeof(unsigned));
    for (auto &matId : tree.voxel_channels[tree.header.mat_id_off].data_i)
      matId += mat_id_offset;
  }

  save_scom2(tree, new_bin_path);

  return true;
}

bool remap_mat_id(const std::string &old_bin_path, const std::string &new_bin_path, const std::string &geometry_type,
                  int mat_id_offset) 
{
  if (geometry_type == "vsgf")
    return remap_mat_id_mesh(old_bin_path, new_bin_path, mat_id_offset);
  else if (geometry_type == "scom2")
    return remap_mat_id_scom2(old_bin_path, new_bin_path, mat_id_offset);
  else
    return false;
}