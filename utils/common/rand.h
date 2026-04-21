#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <unordered_set>
#include <algorithm>

#include "LiteMath/LiteMath.h"

static std::vector<int> get_random_k_n_combination(int k, int n)
{
  if (k > n || k < 0 || n <= 0)
  {
    // Invalid input cases
    return {};
  }

  std::random_device rd;
  std::mt19937 gen(rd());

  if (k > n / 2)
  {
    // If k is close to n, shuffle the entire range and take the first k elements
    std::vector<int> numbers(n);
    for (int i = 0; i < n; ++i)
    {
      numbers[i] = i;
    }
    std::shuffle(numbers.begin(), numbers.end(), gen);
    numbers.resize(k);
    return numbers;
  }
  else
  {
    // If k is much smaller than n, pick k unique numbers using a hash set
    std::unordered_set<int> selected;
    std::uniform_int_distribution<> dis(0, n - 1);
    while ((int)selected.size() < k)
    {
      selected.insert(dis(gen));
    }
    return std::vector<int>(selected.begin(), selected.end());
  }
}

static float urand(float from=0, float to=1)
{
  return ((double)std::rand() / RAND_MAX) * (to - from) + from;
}

/*
  reentrant version of rand()
  This is an implementation of rand() from GNU standard library.
*/
static constexpr uint32_t IRAND_MAX = 1u << 31u;
static int irand_r(uint32_t *seed)
{
  // Linear congruential generator, identical to rand()
  constexpr uint32_t a = 1103515245;
  constexpr uint32_t c = 12345;
  constexpr uint32_t m = IRAND_MAX;
  *seed = (a * (*seed) + c) % m;
  return (*seed);
}

// reentrant version of urand()
static float urand_r(uint32_t *seed, float from=0, float to=1)
{
  return ((double)irand_r(seed) / IRAND_MAX) * (to - from) + from;
}

static LiteMath::float3 urand3_r(uint32_t *seed, LiteMath::float3 from = LiteMath::float3(0.0f), LiteMath::float3 to = LiteMath::float3(1.0f))
{
  float x = urand_r(seed, from.x, to.x);
  float y = urand_r(seed, from.y, to.y);
  float z = urand_r(seed, from.z, to.z);
  return LiteMath::float3(x,y,z);
}

// sample on sphere with uniform distribution on
static LiteMath::float3 sphere_rand_r(uint32_t *seed, float radius = 1.0f)
{
  float u = urand_r(seed);
  float v = urand_r(seed);
  float theta = 2.0f * LiteMath::M_PI * u;
  float phi = std::acos(2.0f * v - 1.0f);
  float sin_phi = std::sin(phi);
  return LiteMath::float3(
      sin_phi * std::cos(theta),
      sin_phi * std::sin(theta),
      std::cos(phi));
}