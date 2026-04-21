#pragma once

#include "LiteMath.h"

#include <unordered_map>

struct PositionHasher
{
  std::size_t operator()(const LiteMath::float3 &k) const
  {
    return ((std::hash<float>()(k.x) ^ (std::hash<float>()(k.y) << 1)) >> 1) ^ (std::hash<float>()(k.z) << 1);
  }
};
struct PositionEqual
{
  bool operator()(const LiteMath::float3 &lhs, const LiteMath::float3 &rhs) const
  {
    return std::abs(lhs.x - rhs.x) < 1e-12f && std::abs(lhs.y - rhs.y) < 1e-12f && std::abs(lhs.z - rhs.z) < 1e-12f;
  }
};

struct CodeHasher
{
  std::size_t operator()(const LiteMath::uint4 &code) const
  {
    //https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector/72073933#72073933
    std::size_t hash = 4;
    for(uint32_t i = 0; i < 4; i++) 
    {
      uint32_t x = code[i];
      x = ((x >> 16) ^ x) * 0x45d9f3b;
      x = ((x >> 16) ^ x) * 0x45d9f3b;
      x = (x >> 16) ^ x;
      hash ^= x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash; 
  }
};

struct CodeEqual
{
  bool operator()(const LiteMath::uint4 &lhs, const LiteMath::uint4 &rhs) const
  {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
  }
};

template <typename T>
using PositionMap = std::unordered_map<LiteMath::float3, T, PositionHasher, PositionEqual>;

template <typename T>
using CodeMap = std::unordered_map<LiteMath::uint4, T, CodeHasher, CodeEqual>;
