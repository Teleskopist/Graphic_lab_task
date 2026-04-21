#pragma once
#include <vector>
#include <map>
#include <unordered_map>

template <typename K>
static std::size_t array_hash(const K *array, uint32_t size)
{
  // https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector/72073933#72073933
  std::size_t hash = size;
  for (uint32_t i = 0; i < size; i++)
  {
    uint32_t x = array[i];
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    hash ^= x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  }
  return hash;
}

template <typename K>
struct VectorHasher
{
  std::size_t operator()(const std::vector<K> &vector) const
  {
    //https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector/72073933#72073933
    std::size_t hash = vector.size();
    for(uint32_t i = 0; i < vector.size(); i++) 
    {
      uint32_t x = vector[i];
      x = ((x >> 16) ^ x) * 0x45d9f3b;
      x = ((x >> 16) ^ x) * 0x45d9f3b;
      x = (x >> 16) ^ x;
      hash ^= x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash; 
  }
};

template <typename K>
struct VectorEqual
{
  bool operator()(const std::vector<K> &lhs, const std::vector<K> &rhs) const
  {
    if (lhs.size() != rhs.size())
      return false;
    for(uint32_t i = 0; i < lhs.size(); i++)
      if (lhs[i] != rhs[i])
        return false;
    return true;
  }
};

template <typename K, typename V>
using VectorMap = std::unordered_map<std::vector<K>, V, VectorHasher<K>, VectorEqual<K>>;


template <typename K>
struct VectorLess {
  bool operator()(const std::vector<K> &lhs, const std::vector<K> &rhs) const
  {
    if (lhs.size() != rhs.size())
      return lhs.size() < rhs.size();
    for(uint32_t i = 0; i < lhs.size(); i++)
    {
      if (lhs[i] < rhs[i])
        return true;
      else if (lhs[i] > rhs[i])
        return false;
    }
    return false;
  }
};

template <typename K, typename V>
using OrderedVectorMap = std::map<std::vector<K>, V, VectorLess<K>>;