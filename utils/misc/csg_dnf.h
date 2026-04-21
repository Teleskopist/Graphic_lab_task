#pragma once

#include <vector>
#include <LiteMath.h>
#include <string>
#include <fstream>
#include <sstream>

using LiteMath::float3, LiteMath::float2;

#ifndef KERNEL_SLICER

struct CSGDNFnode
{
  std::vector<unsigned> figures;
  std::vector<std::vector<float>> params;
};

struct CSGDNF
{
  std::vector<CSGDNFnode> blocks;
  float get_distance(const float3 &point) const;

  void save_model(const std::string& path) const;
  void load_model(const std::string& path);
};

#else
struct CSGDNF
{
  int lol;
};
#endif