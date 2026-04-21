#pragma once 
#include <cstdint>

template <typename T>
static inline T pow2(T x) { return x * x; }

template <typename T>
static inline T pow3(T x) { return x * x * x; }

template <typename T>
static inline T pow4(T x) { return x * x * x * x; }

template <typename T>
static inline T int_pow(T base, uint32_t exp)
{
  T result = 1;
  while (exp)
  {
    if (exp & 1)
      result *= base;
    exp >>= 1;
    base *= base;
  }
  return result;
}