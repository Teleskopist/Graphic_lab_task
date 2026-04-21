#pragma once
#include <array>
#include <map>
#include <unordered_map>

template <int N, typename K>
struct ArrayHasher
{
  std::size_t operator()(const std::array<K, N> &array) const
  {
    //https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-array/72073933#72073933
    std::size_t hash = array.size();
    for(uint32_t i = 0; i < array.size(); i++) 
    {
      uint32_t x = array[i];
      x = ((x >> 16) ^ x) * 0x45d9f3b;
      x = ((x >> 16) ^ x) * 0x45d9f3b;
      x = (x >> 16) ^ x;
      hash ^= x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash; 
  }
};

template <int N, typename K>
struct ArrayEqual
{
  bool operator()(const std::array<K, N> &lhs, const std::array<K, N> &rhs) const
  {
    if (lhs.size() != rhs.size())
      return false;
    for(uint32_t i = 0; i < lhs.size(); i++)
      if (lhs[i] != rhs[i])
        return false;
    return true;
  }
};

template <int N, typename K, typename V>
using ArrayMap = std::unordered_map<std::array<K, N>, V, ArrayHasher<N, K>, ArrayEqual<N, K>>;


template <int N, typename K>
struct ArrayLess {
  bool operator()(const std::array<K, N> &lhs, const std::array<K, N> &rhs) const
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

template <int N, typename K, typename V>
using OrderedArrayMap = std::map<std::array<K, N>, V, ArrayLess<N, K>>;