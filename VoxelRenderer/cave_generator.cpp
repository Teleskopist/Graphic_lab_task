#include "cave_generator.h"
#include <cmath>
#include <queue>
#include <vector>
#include <limits>
#include <cstdio>

// ============================================================================
// Fast Deterministic Random Number Generator (xorshift64*)
// ============================================================================

class XORShift64 {
private:
    uint64_t state;
    
public:
    explicit XORShift64(uint32_t seed) {
        // Mix seed into a 64-bit state using a simple hash
        uint64_t x = seed;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        state = x ? x : 1;
    }
    
    inline uint32_t next() {
        uint64_t x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        state = x;
        return static_cast<uint32_t>(x >> 32);
    }
    
    inline float next_float() {
        return (next() >> 8) * (1.0f / 16777216.0f);
    }
};

// ============================================================================
// Neighbor Counting (Optimized)
// ============================================================================

inline uint8_t count_neighbors_moore(
    const uint8_t* data,
    uint32_t x, uint32_t y, uint32_t z,
    uint32_t size_x, uint32_t size_y, uint32_t size_z
) {
    uint8_t count = 0;
    
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                
                int nx = static_cast<int>(x) + dx;
                int ny = static_cast<int>(y) + dy;
                int nz = static_cast<int>(z) + dz;
                
                if (nx >= 0 && nx < static_cast<int>(size_x) &&
                    ny >= 0 && ny < static_cast<int>(size_y) &&
                    nz >= 0 && nz < static_cast<int>(size_z)) {
                    
                    uint32_t idx = static_cast<uint32_t>(nx) * size_y * size_z +
                                   static_cast<uint32_t>(ny) * size_z +
                                   static_cast<uint32_t>(nz);
                    count += data[idx] ? 1 : 0;
                }
            }
        }
    }
    
    return count;
}

inline uint8_t count_neighbors_von_neumann(
    const uint8_t* data,
    uint32_t x, uint32_t y, uint32_t z,
    uint32_t size_x, uint32_t size_y, uint32_t size_z
) {
    uint8_t count = 0;
    
    // 6-neighborhood: (±1,0,0), (0,±1,0), (0,0,±1)
    const int dx[] = {-1, 1, 0, 0, 0, 0};
    const int dy[] = {0, 0, -1, 1, 0, 0};
    const int dz[] = {0, 0, 0, 0, -1, 1};
    
    for (int i = 0; i < 6; ++i) {
        int nx = static_cast<int>(x) + dx[i];
        int ny = static_cast<int>(y) + dy[i];
        int nz = static_cast<int>(z) + dz[i];
        
        if (nx >= 0 && nx < static_cast<int>(size_x) &&
            ny >= 0 && ny < static_cast<int>(size_y) &&
            nz >= 0 && nz < static_cast<int>(size_z)) {
            
            uint32_t idx = static_cast<uint32_t>(nx) * size_y * size_z +
                           static_cast<uint32_t>(ny) * size_z +
                           static_cast<uint32_t>(nz);
            count += data[idx] ? 1 : 0;
        }
    }
    
    return count;
}

// ============================================================================
// Cellular Automata Iteration
// ============================================================================

void cellular_automata_step(
    const uint8_t* current,
    uint8_t* next,
    uint32_t size_x, uint32_t size_y, uint32_t size_z,
    uint8_t birth_threshold, uint8_t survival_threshold,
    bool use_moore_neighborhood
) {
    auto count_neighbors = use_moore_neighborhood ?
        count_neighbors_moore : count_neighbors_von_neumann;
    
    #pragma omp parallel for collapse(3)
    for (uint32_t x = 0; x < size_x; ++x) {
        for (uint32_t y = 0; y < size_y; ++y) {
            for (uint32_t z = 0; z < size_z; ++z) {
                uint32_t idx = x * size_y * size_z + y * size_z + z;
                uint8_t neighbors = count_neighbors(current, x, y, z, size_x, size_y, size_z);
                
                if (current[idx]) {
                    // Cell is filled: survive if neighbors >= survival_threshold
                    next[idx] = (neighbors >= survival_threshold) ? 1 : 0;
                } else {
                    // Cell is empty: birth if neighbors >= birth_threshold
                    next[idx] = (neighbors >= birth_threshold) ? 1 : 0;
                }
            }
        }
    }
}

// ============================================================================
// Flood Fill for Connected Component Analysis
// ============================================================================

uint32_t flood_fill_count(
    uint8_t* data,
    uint32_t start_x, uint32_t start_y, uint32_t start_z,
    uint32_t size_x, uint32_t size_y, uint32_t size_z
) {
    if (start_x >= size_x || start_y >= size_y || start_z >= size_z) {
        return 0;
    }
    
    uint32_t start_idx = start_x * size_y * size_z + start_y * size_z + start_z;
    
    if (data[start_idx] == 0) {
        return 0;
    }
    
    std::queue<uint32_t> queue;
    uint32_t count = 0;
    
    queue.push(start_idx);
    data[start_idx] = 0xFF; // Mark as visited using 0xFF
    
    while (!queue.empty()) {
        uint32_t idx = queue.front();
        queue.pop();
        count++;
        
        uint32_t x = idx / (size_y * size_z);
        uint32_t y = (idx % (size_y * size_z)) / size_z;
        uint32_t z = idx % size_z;
        
        // Check 26 neighbors
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    if (dx == 0 && dy == 0 && dz == 0) continue;
                    
                    int nx = static_cast<int>(x) + dx;
                    int ny = static_cast<int>(y) + dy;
                    int nz = static_cast<int>(z) + dz;
                    
                    if (nx >= 0 && nx < static_cast<int>(size_x) &&
                        ny >= 0 && ny < static_cast<int>(size_y) &&
                        nz >= 0 && nz < static_cast<int>(size_z)) {
                        
                        uint32_t nidx = static_cast<uint32_t>(nx) * size_y * size_z +
                                        static_cast<uint32_t>(ny) * size_z +
                                        static_cast<uint32_t>(nz);
                        
                        if (data[nidx] == 1) {
                            queue.push(nidx);
                            data[nidx] = 0xFF;
                        }
                    }
                }
            }
        }
    }
    
    return count;
}

// ============================================================================
// Main Cave Generation Function
// ============================================================================

void create_procedural_cave(
    CaveGeneratorSettings settings,
    uint32_t size_x,
    uint32_t size_y,
    uint32_t size_z,
    uint32_t seed,
    uint8_t* out_data
) {
    uint64_t total_size = static_cast<uint64_t>(size_x) * size_y * size_z;
    
    // Allocate working buffers
    uint8_t* current = new uint8_t[total_size];
    uint8_t* next = new uint8_t[total_size];
    
    XORShift64 rng(seed);
    
    // ========================================================================
    // Phase 1: Initialize with random values
    // ========================================================================
    for (uint64_t i = 0; i < total_size; ++i) {
        current[i] = (rng.next_float() < settings.initial_fill_probability) ? 1 : 0;
    }
    
    // ========================================================================
    // Phase 3: Cellular automata iterations
    // ========================================================================
    for (uint32_t iter = 0; iter < settings.smoothing_iterations; ++iter) {
        cellular_automata_step(
            current, next, size_x, size_y, size_z,
            settings.birth_threshold, settings.survival_threshold,
            settings.use_moore_neighborhood
        );
        
        std::swap(current, next);
    }
    
    // ========================================================================
    // Phase 4: Filter small caves (only if needed)
    // ========================================================================
    if (settings.min_cave_size > 0 || settings.keep_largest_only) {
        // Create temporary working copy for component analysis
        uint8_t* temp_data = new uint8_t[total_size];
        std::memcpy(temp_data, current, total_size);
        
        uint32_t largest_size = 0;
        uint32_t largest_start = UINT32_MAX;
        
        // Find all connected components of empty space (0 = empty)
        for (uint64_t i = 0; i < total_size; ++i) {
            if (temp_data[i] == 0) {
                uint32_t x = i / (size_y * size_z);
                uint32_t y = (i % (size_y * size_z)) / size_z;
                uint32_t z = i % size_z;
                
                // Flood fill to find component size
                uint32_t component_size = 0;
                std::queue<uint32_t> fill_queue;
                fill_queue.push(i);
                temp_data[i] = 2; // Mark as visited (using 2 to distinguish from 0 and 1)
                
                while (!fill_queue.empty()) {
                    uint32_t idx = fill_queue.front();
                    fill_queue.pop();
                    component_size++;
                    
                    uint32_t cx = idx / (size_y * size_z);
                    uint32_t cy = (idx % (size_y * size_z)) / size_z;
                    uint32_t cz = idx % size_z;
                    
                    // Check 26 neighbors
                    for (int dx = -1; dx <= 1; ++dx) {
                        for (int dy = -1; dy <= 1; ++dy) {
                            for (int dz = -1; dz <= 1; ++dz) {
                                if (dx == 0 && dy == 0 && dz == 0) continue;
                                
                                int nx = static_cast<int>(cx) + dx;
                                int ny = static_cast<int>(cy) + dy;
                                int nz = static_cast<int>(cz) + dz;
                                
                                if (nx >= 0 && nx < static_cast<int>(size_x) &&
                                    ny >= 0 && ny < static_cast<int>(size_y) &&
                                    nz >= 0 && nz < static_cast<int>(size_z)) {
                                    
                                    uint32_t nidx = static_cast<uint32_t>(nx) * size_y * size_z +
                                                    static_cast<uint32_t>(ny) * size_z +
                                                    static_cast<uint32_t>(nz);
                                    
                                    if (temp_data[nidx] == 0) {
                                        fill_queue.push(nidx);
                                        temp_data[nidx] = 2;
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (component_size > largest_size) {
                    largest_size = component_size;
                    largest_start = i;
                }
            }
        }
        
        if (settings.keep_largest_only && largest_start != UINT32_MAX) {
            // Fill everything, then restore the largest component
            std::memset(current, 1, total_size);
            
            // Flood fill the largest component and mark it as empty (0)
            std::queue<uint32_t> fill_queue;
            fill_queue.push(largest_start);
            current[largest_start] = 0;
            
            while (!fill_queue.empty()) {
                uint32_t idx = fill_queue.front();
                fill_queue.pop();
                
                uint32_t x = idx / (size_y * size_z);
                uint32_t y = (idx % (size_y * size_z)) / size_z;
                uint32_t z = idx % size_z;
                
                // Check 26 neighbors
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dz = -1; dz <= 1; ++dz) {
                            if (dx == 0 && dy == 0 && dz == 0) continue;
                            
                            int nx = static_cast<int>(x) + dx;
                            int ny = static_cast<int>(y) + dy;
                            int nz = static_cast<int>(z) + dz;
                            
                            if (nx >= 0 && nx < static_cast<int>(size_x) &&
                                ny >= 0 && ny < static_cast<int>(size_y) &&
                                nz >= 0 && nz < static_cast<int>(size_z)) {
                                
                                uint32_t nidx = static_cast<uint32_t>(nx) * size_y * size_z +
                                                static_cast<uint32_t>(ny) * size_z +
                                                static_cast<uint32_t>(nz);
                                
                                if (current[nidx] == 1) {
                                    fill_queue.push(nidx);
                                    current[nidx] = 0;
                                }
                            }
                        }
                    }
                }
            }
        } else if (settings.min_cave_size > 0) {
            // Restore current from temp_data and filter small caves
            std::memcpy(current, temp_data, total_size);
            
            // Mark small empty components as filled
            for (uint64_t i = 0; i < total_size; ++i) {
                if (current[i] == 0) {
                    uint32_t x = i / (size_y * size_z);
                    uint32_t y = (i % (size_y * size_z)) / size_z;
                    uint32_t z = i % size_z;
                    
                    uint32_t component_size = 0;
                    std::queue<uint32_t> fill_queue;
                    fill_queue.push(i);
                    current[i] = 2; // Temporarily mark as visited
                    
                    std::vector<uint32_t> component_cells;
                    component_cells.push_back(i);
                    
                    while (!fill_queue.empty()) {
                        uint32_t idx = fill_queue.front();
                        fill_queue.pop();
                        component_size++;
                        
                        uint32_t cx = idx / (size_y * size_z);
                        uint32_t cy = (idx % (size_y * size_z)) / size_z;
                        uint32_t cz = idx % size_z;
                        
                        // Check 26 neighbors
                        for (int dx = -1; dx <= 1; ++dx) {
                            for (int dy = -1; dy <= 1; ++dy) {
                                for (int dz = -1; dz <= 1; ++dz) {
                                    if (dx == 0 && dy == 0 && dz == 0) continue;
                                    
                                    int nx = static_cast<int>(cx) + dx;
                                    int ny = static_cast<int>(cy) + dy;
                                    int nz = static_cast<int>(cz) + dz;
                                    
                                    if (nx >= 0 && nx < static_cast<int>(size_x) &&
                                        ny >= 0 && ny < static_cast<int>(size_y) &&
                                        nz >= 0 && nz < static_cast<int>(size_z)) {
                                        
                                        uint32_t nidx = static_cast<uint32_t>(nx) * size_y * size_z +
                                                        static_cast<uint32_t>(ny) * size_z +
                                                        static_cast<uint32_t>(nz);
                                        
                                        if (current[nidx] == 0) {
                                            fill_queue.push(nidx);
                                            current[nidx] = 2;
                                            component_cells.push_back(nidx);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    // If component is too small, fill it
                    if (component_size < settings.min_cave_size) {
                        for (uint32_t cell_idx : component_cells) {
                            current[cell_idx] = 1;
                        }
                    } else {
                        // Otherwise restore the 0 values
                        for (uint32_t cell_idx : component_cells) {
                            current[cell_idx] = 0;
                        }
                    }
                }
            }
        }
        
        delete[] temp_data;
    }

    int count = 0;
    for (uint64_t i = 0; i < total_size; ++i) {
        if (current[i] != 1) {
            count++;
        }
    }
    printf("Count: %d\n", count);
    
    // ========================================================================
    // Phase 2: Add border walls AFTER random initialization
    // ========================================================================
    for (uint32_t x = 0; x < size_x; ++x) {
        for (uint32_t y = 0; y < size_y; ++y) {
            for (uint32_t z = 0; z < size_z; ++z) {
                uint32_t dist_x = std::min(x, size_x - 1 - x);
                uint32_t dist_y = std::min(y, size_y - 1 - y);
                uint32_t dist_z = std::min(z, size_z - 1 - z);
                uint32_t min_dist = std::min({dist_x, dist_y, dist_z});
                
                if (min_dist < settings.border_thickness) {
                    uint32_t idx = x * size_y * size_z + y * size_z + z;
                    current[idx] = 1;
                }
            }
        }
    }

    // ========================================================================
    // Phase 5: Copy to output
    // ========================================================================
    for (uint64_t i = 0; i < total_size; ++i) {
        out_data[i] = current[i] == 1 ? 0 : 1;
    }
    //std::memcpy(out_data, current, total_size);
    
    // ========================================================================
    // Cleanup
    // ========================================================================
    delete[] current;
    delete[] next;
}