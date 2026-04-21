#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

static constexpr uint32_t QUADTREE_LEAF_BIT = 0x80000000u;
static constexpr uint32_t QUADTREE_NODE_MASK = 0x7fffffffu;

std::vector<uint32_t> create_quadtree_from_image(const float* image, size_t w, size_t h);