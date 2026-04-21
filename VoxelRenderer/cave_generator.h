#pragma once
#include <cstdint>
#include <algorithm>
#include <cstring>

/**
 * Settings for procedural cave generation using cellular automata
 */
struct CaveGeneratorSettings {
    // Initial fill probability (0.0 - 1.0)
    // Higher values = denser initial state
    float initial_fill_probability = 0.5f;
    
    // Number of iterations for cellular automata smoothing
    uint32_t smoothing_iterations = 100;
    
    // Threshold for cell birth (number of filled neighbors required)
    // Range: 0-26 (for 3D 26-neighborhood)
    uint8_t birth_threshold = 16;
    
    // Threshold for cell survival (number of filled neighbors required)
    // Range: 0-26
    uint8_t survival_threshold = 12;
    
    // Minimum cave size (connected component size threshold)
    // Smaller caves will be filled in
    uint32_t min_cave_size = 100;
    
    // Enable post-processing to keep only largest connected component
    bool keep_largest_only = false;
    
    // Border wall thickness (voxels)
    uint8_t border_thickness = 1;
    
    // Use Moore neighborhood (26) vs Von Neumann neighborhood (6)
    bool use_moore_neighborhood = true;
};

/**
 * Generate a procedural cave system using cellular automata
 * 
 * @param settings Configuration for cave generation
 * @param size_x Width of the cave system (X dimension)
 * @param size_y Height of the cave system (Y dimension)
 * @param size_z Depth of the cave system (Z dimension)
 * @param seed Random seed for deterministic generation
 * @param out_data Output array (must be at least size_x*size_y*size_z bytes)
 *                 0 = empty voxel, 1 = filled voxel
 * 
 * Voxel (i,j,k) address: i*size_y*size_z + j*size_z + k
 */
void create_procedural_cave(
    CaveGeneratorSettings settings,
    uint32_t size_x,
    uint32_t size_y,
    uint32_t size_z,
    uint32_t seed,
    uint8_t* out_data
);