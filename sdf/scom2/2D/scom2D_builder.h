#pragma once
#include "sdf/scom2/scom_builder.h"

namespace scom2
{
  struct RangePyramid
  {
    std::vector<std::vector<float2>> levels;
    std::vector<uint2> level_sizes;
  };

  // building global octree from texture (single channel)
  void create_range_pyramid(const float *data, size_t w, size_t h, RangePyramid &rp);
  void range_pyramid_to_global_octree(const RangePyramid &rp, sdf_converter::GlobalOctree &octree, uint32_t b_size, float stop_thr);

  // debug function to restore texture from SCom DAG
  void restore_texture_from_DAG(const SdfDAG &dag, float *data, uint32_t w, uint32_t h);

  struct SCom2DCompressionStat
  {
    //sizes in bytes
    uint64_t original_size = 0;
    uint64_t dag_size      = 0;
    uint64_t scom_size     = 0;

    //quality metrics
    float mse = 0.0f;
    float mae = 0.0f;
    float psnr = 0.0f;
  };

  // debug function to test SCom2D compression
  SCom2DCompressionStat test_scom2_compression(const float *data, uint32_t w, uint32_t h, const LiteImage::Image2D<float> &image_ref, 
                                               uint32_t brick_size_log2, float sim_thr, bool save_dag = false, std::string save_dir = "", 
                                               std::string name = "");

  // debug function to compare SCom2D against png and jpeg
  void scom2_png_jpeg_comparison(std::string images_folder);
}