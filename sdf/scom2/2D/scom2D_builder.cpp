#include "scom2D_builder.h"
#include "sdf/scom2/scom_utils.h"
#include "utils/misc/image_metrics.h"
#include "utils/common/int_pow.h"
#include "tinyexr.h"
#include "SimplexNoise.h"
#include "stb_image.h"
#include <filesystem>
#include <iostream>

namespace scom2
{
  bool SaveImage1fToEXR(const float* data, int width, int height, const char* outfilename, float a_normConst = 1.0f, bool a_invertY = false) 
  {
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);
    image.num_channels = 1;

    const float* image_ptr[1];
    image_ptr[0] = data; // R

    image.images = (unsigned char**)image_ptr;
    image.width  = width;
    image.height = height;
    header.num_channels = 1;
    header.channels     = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    // Must be (A)BGR order, since most of EXR viewers expect this channel order.
    strncpy(header.channels[0].name, "R", 255); header.channels[0].name[strlen("R")] = '\0';

    header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
      header.pixel_types[i]           = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
      header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of output image to be stored in .EXR
    }
  
    const char* err = nullptr; 
    int ret = SaveEXRImageToFile(&image, &header, outfilename, &err);
    if (ret != TINYEXR_SUCCESS) {
      fprintf(stderr, "Save EXR err: %s\n", err);
      FreeEXRErrorMessage(err); // free's buffer for an error message
      return false;
    }
    //printf("Saved exr file. [%s] \n", outfilename);

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);

    return true;
  }

  void create_range_pyramid(const float *data, size_t w, size_t h, RangePyramid &rp)
  {
    size_t level_w = w;
    size_t level_h = h;

    rp.levels.clear();

    rp.levels.emplace_back();
    rp.levels.back().resize(level_w * level_h);

    rp.level_sizes.push_back(uint2((uint32_t)level_w, (uint32_t)level_h));

    for (size_t j = 0; j < level_h; j++)
    {
      for (size_t i = 0; i < level_w; i++)
      {
        float v = data[i + j * w];
        rp.levels.back()[i + j * level_w] = float2(v, v);
      }
    }

    while (level_w > 1 || level_h > 1)
    {
      size_t prev_level_w = level_w;
      size_t prev_level_h = level_h;
      level_w = std::max(size_t(1), level_w / 2);
      level_h = std::max(size_t(1), level_h / 2);

      rp.levels.emplace_back();
      rp.levels.back().resize(level_w * level_h);

      rp.level_sizes.push_back(uint2((uint32_t)level_w, (uint32_t)level_h));

      for (size_t j = 0; j < level_h; j++)
      {
        for (size_t i = 0; i < level_w; i++)
        {
          float2 vmin = float2(FLT_MAX, FLT_MAX);
          float2 vmax = float2(-FLT_MAX, -FLT_MAX);

          for (size_t y = 0; y < 2; y++)
          {
            for (size_t x = 0; x < 2; x++)
            {
              size_t src_i = std::min(i * 2 + x, prev_level_w - 1);
              size_t src_j = std::min(j * 2 + y, prev_level_h - 1);
              float2 v = rp.levels[rp.levels.size() - 2][src_i + src_j * prev_level_w];
              vmin = min(vmin, v);
              vmax = max(vmax, v);
            }
          }

          rp.levels.back()[i + j * level_w] = float2(vmin.x, vmax.y);
        }
      }
    }

    // reverse the pyramid
    std::reverse(rp.levels.begin(), rp.levels.end());
    std::reverse(rp.level_sizes.begin(), rp.level_sizes.end());
  }

  void range_pyramid_to_global_octree_rec(const RangePyramid &rp, sdf_converter::GlobalOctree &octree,
                                          uint32_t b_size, uint32_t b_off, float stop_thr,
                                          uint32_t nodeId, uint3 code, uint32_t level)
  {
    auto &node = octree.nodes[nodeId];

    // float
    bool is_leaf = false;

    if (level + b_off == rp.level_sizes.size() - 1)
    {
      is_leaf = true;
    }
    // SCom2 -> image procedure does not support variable depth octrees, and it is
    // not needed for terrain representation, so we always subdivide until the last level
    // else if (level > 0)
    // {
    //   uint32_t level_w = rp.level_sizes[level-1].x;
    //   uint32_t level_h = rp.level_sizes[level-1].y;
    //   uint2 pix_coord = uint2(code.x/2, code.y/2);
    //   pix_coord.x = std::min(pix_coord.x, level_w - 1);
    //   pix_coord.y = std::min(pix_coord.y, level_h - 1);
    //   float2 v = rp.levels[level-1][pix_coord.x + pix_coord.y * level_w];
    //   if (std::abs(v.y - v.x) < stop_thr)
    //     is_leaf = true;
    // }

    if (is_leaf)
    {
      node.type = sdf_converter::GlobalOctreeNodeType::LEAF;
      node.offset = 0;
      node.bricks_count = 1;
      node.is_internal = true;
      node.val_off = octree.values_f.size();

      uint32_t level_w = rp.level_sizes[level + b_off].x;
      uint32_t level_h = rp.level_sizes[level + b_off].y;

      for (int i = 0; i < (b_size + 1) * (b_size + 1); i++)
      {
        uint2 pix_coord = uint2(b_size * code.x + (i / (b_size + 1)), b_size * code.y + (i % (b_size + 1)));
        pix_coord.x = std::min(pix_coord.x, level_w - 1);
        pix_coord.y = std::min(pix_coord.y, level_h - 1);
        float v = 0.5f * (rp.levels[level + b_off][pix_coord.x + pix_coord.y * level_w].x +
                          rp.levels[level + b_off][pix_coord.x + pix_coord.y * level_w].y);
        octree.values_f.push_back(-1.0f * v);
      }
    }
    else
    {
      node.type = sdf_converter::GlobalOctreeNodeType::EMPTY_NODE;
      node.offset = octree.nodes.size();
      node.bricks_count = 0;
      node.val_off = 0;

      octree.nodes.resize(octree.nodes.size() + 4);

      range_pyramid_to_global_octree_rec(rp, octree, b_size, b_off, stop_thr, node.offset + 0, 2 * code + uint3(0, 0, 0), level + 1);
      range_pyramid_to_global_octree_rec(rp, octree, b_size, b_off, stop_thr, node.offset + 1, 2 * code + uint3(0, 1, 0), level + 1);
      range_pyramid_to_global_octree_rec(rp, octree, b_size, b_off, stop_thr, node.offset + 2, 2 * code + uint3(1, 0, 0), level + 1);
      range_pyramid_to_global_octree_rec(rp, octree, b_size, b_off, stop_thr, node.offset + 3, 2 * code + uint3(1, 1, 0), level + 1);
    }
  }

  void range_pyramid_to_global_octree(const RangePyramid &rp, sdf_converter::GlobalOctree &octree, uint32_t b_size, float stop_thr)
  {
    octree.header.brick_size = b_size;
    octree.header.brick_pad = 0;
    octree.header.dim = 2;

    int w = rp.level_sizes.back().x;
    int h = rp.level_sizes.back().y;

    octree.nodes.reserve(2 * w * h);
    octree.values_f.reserve(4 * w * h);

    octree.nodes.emplace_back();
    range_pyramid_to_global_octree_rec(rp, octree, b_size, log2(b_size), stop_thr, 0, uint3(0, 0, 1), 0);

    octree.nodes.shrink_to_fit();
    octree.values_f.shrink_to_fit();
  }

  void restore_texture_from_DAG(const SdfDAG &dag, float *data, uint32_t w, uint32_t h)
  {
    uint32_t b_size = dag.header.brick_size;
    const scom2::Subgroup *sgroup = scom2::create_subgroup(scom2::TransformSubgroup::DEFAULT, b_size + 1, 2);

    struct Pxl
    {
      float4 val = float4(0, 0, 0, 0);
      uint32_t cnt = 0;

      void add(float v)
      {
        assert(cnt < 4);
        val[cnt] = v;
        cnt++;
      }
    };

    std::vector<Pxl> restored_pixels(w * h);
    uint32_t max_size = int_pow(2, ceil(std::log2(std::max(w, h))));
    printf("Restoring texture of size %dx%d (padded to %d)\n", w, h, max_size);
    auto restore_texture_t = [&](const SdfDAG &D, uint32_t nodeId, uint32_t transformId, uint32_t level, uint4 code) -> bool
    {
      const SdfDAGNode &node = D.nodes[nodeId];
      if (node.data_edges_offset == 0 || DAG_extract_count(node.voxel_count_flags) == 0)
        return true;

      assert(int_pow(2, level) <= max_size);

      const auto &de = D.data_edges[node.data_edges_offset];
      float add = de.add;
      uint32_t data_off = de.data_offset;
      uint32_t T = sgroup->cayley_table[de.rotation_id][transformId];
      uint32_t mult = max_size / (int_pow(2, level) * b_size);

      float dists[2][2];
      for (uint32_t i0 = 0; i0 < b_size; i0++)
      {
        for (uint32_t j0 = 0; j0 < b_size; j0++)
        {
          float d = D.distances[data_off + sgroup->elements[T][i0 * (b_size + 1) + j0]] + add;
          uint32_t v1 = b_size * code.y + mult * i0;
          uint32_t v2 = b_size * code.z + mult * j0;
          if (v1 >= w || v2 >= h)
            continue;

          uint32_t pxl_idx = v1 + v2 * w;
          restored_pixels[pxl_idx].add(d);
        }
      }

      return true;
    };

    scom2::traverse_DAG(dag, restore_texture_t);

    for (uint32_t i = 0; i < w * h; i++)
    {
      if (restored_pixels[i].cnt > 0)
        data[i] = -1.0f * restored_pixels[i].val.x / restored_pixels[i].cnt;
      else
        data[i] = 0.0f;
    }
  }

  SCom2DCompressionStat test_scom2_compression(const float *data, uint32_t w, uint32_t h, const LiteImage::Image2D<float> &image_ref, 
                                               uint32_t brick_size_log2, float sim_thr, bool save_dag, std::string save_dir, std::string name)
  {
    uint32_t b_size = 1 << brick_size_log2;
    sdf_converter::GlobalOctree octree;
    {
      RangePyramid rp;
      create_range_pyramid(data, w, h, rp);
      range_pyramid_to_global_octree(rp, octree, b_size, sim_thr);
    }

    scom2::Settings scom_settings;
    scom_settings.brick_size = b_size;
    scom_settings.internal_brick_use = scom2::InternalBrickUse::PROPER_CLUSTERING;
    scom_settings.branch_descriptor = scom2::BranchDescriptor::SIMPLE_MERGE;
    scom_settings.embedding_type = scom2::EmbeddingType::TRIVIAL;
    scom_settings.bricks_use_similarity_compression = true;
    scom_settings.bricks_similarity_threshold = sim_thr;
    scom_settings.branches_use_similarity_compression = true;
    scom_settings.branches_similarity_compression_levels = 6 - brick_size_log2;
    scom_settings.branches_similarity_threshold = sim_thr;
    scom_settings.packed_reference_type = scom2::PackedReferenceType::SHORT_6_8_18;
    scom_settings.bits_per_value = 16;
    scom_settings.support_multi_nodes = false;
    scom_settings.custom_max_val = true;
    scom_settings.support_channels = false;
    scom_settings.use_close_match_heuristic = true;

    SdfDAG dag;
    SCom2Tree scom;
    sdf_converter::global_octree_to_DAG(octree, dag, scom_settings);
    scom2::pack_SCom2(scom_settings, dag, scom);

    LiteImage::Image2D<float> restored_img(w, h);
    restore_texture_from_DAG(dag, restored_img.data(), w, h);

    if (save_dag)
    {
      save_scom2(scom, save_dir + "/" + name + "_scom.bin");
      save_sdf_DAG(dag, save_dir + "/" + name + "_dag.bin");
      SaveImage1fToEXR(restored_img.data(), w, h, (save_dir + "/" + name + "_restored.exr").c_str());
    }

    SCom2DCompressionStat stat;
    stat.original_size = w * h * sizeof(float);
    stat.dag_size = dag.nodes.size() * sizeof(SdfDAGNode) +
                    dag.data_edges.size() * sizeof(SdfDAGDataEdge) +
                    dag.child_edges.size() * sizeof(SdfDAGChildEdge) +
                    dag.distances.size() * sizeof(float);
    stat.scom_size = (scom.bricks.size() + scom.nodes.size()) * sizeof(uint32_t);
    stat.mse  = image_metrics::MSE(image_ref, restored_img);
    stat.mae  = image_metrics::MAE(image_ref, restored_img);
    stat.psnr = image_metrics::PSNR(image_ref, restored_img);

    return stat;
  }

  std::string shrink_exr(float *data, int width, int height, const std::string &name, const std::string &save_folder, std::vector<float> &shrinked_data)
  {
    int w = width / 2, h = height / 2;
    shrinked_data.resize(w * h);
    for (int i = 0; i < h; i++)
    {
      for (int j = 0; j < w; j++)
      {
        shrinked_data[i * w + j] = (data[i * 2 * width + j * 2] + data[(i * 2 + 1) * width + j * 2] + data[i * 2 * width + j * 2 + 1] + data[(i * 2 + 1) * width + j * 2 + 1]) / 4.0f;
      }
    }

    std::string new_path = save_folder + "/" + name + "_mip" + std::to_string(h) + ".exr";
    SaveImage1fToEXR(shrinked_data.data(), w, h, new_path.c_str());
    return new_path;
  }

  struct TextMipInfo
  {
    std::vector<float> data;
    std::string path;
    int w, h;

  };

  std::vector<float> LoadImage1fFromEXRNoFlip(const char* infilename, int* pW, int* pH)
  {
    float* out; // width * height * RGBA
    int width       = 0;
    int height      = 0;
    const char* err = nullptr;

    int ret = LoadEXR(&out, &width, &height, infilename, &err);
    if (ret != TINYEXR_SUCCESS) {
      if (err) {
        fprintf(stderr, "[LoadImage1fFromEXRNoFlip] : %s\n", err);
        std::cerr << "[LoadImage1fFromEXRNoFlip] : " << err;
        std::cerr << " from path : " << infilename << std::endl;
        
        delete err;
      }
      return std::vector<float>();
    }

    const int imgSize = width * height;
    std::vector<float> result(imgSize);
    *pW = uint32_t(width);
    *pH = uint32_t(height);
    
    #pragma omp parallel for
    for(int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        size_t idx = (x + y * width) * 4;
        size_t out_idx = x + y * width;
        if (std::isinf(out[idx]))
          result[out_idx] = 65504.0f;                       // max half float according to ieee
        else
          result[out_idx] = LiteMath::clamp(out[idx], 0.0f, 65504.0f); // max half float according to ieee
        
      }
    }

    free(out);
    return result;
  }

  void add_perlin_noise(std::vector<float> &data, int w, int h, int steps, float scale)
  {
    SimplexNoise noiseGen = SimplexNoise();
    for (int i=0; i < h; i++)
    {
      for (int j=0; j < w; j++)
      {
        float2 p = 2.0f * (float2(i, j) / float2(w - 1, h - 1)) - 1.0f;
        float noise = 0.0f;
        for (int k = 0; k < steps; k++)
        {
          noise += noiseGen.noise(p.x, p.y);
          p.x *= 2.0f;
          p.y *= 2.0f;
        }
        //printf("%d %d %f\n", i, j, noise);
        data[i * w + j] = std::min(std::max(data[i * w + j] + scale * noise, 0.0f), 1.0f);
      }
    }
  }

  void scom2_png_jpeg_comparison(std::string images_folder)
  {
    stbi_set_flip_vertically_on_load(0);
    namespace fs = std::filesystem;
    fs::path dir = images_folder;
    std::ofstream csv(images_folder + "/scom2_png_jpeg_comparison.csv");
    csv << "name,format,width,height,brick_size_log,sim_thr,MSE,MAE,PSNR(dB),size(B)\n";
    float a_gamma = 2.2f;
    std::vector<fs::directory_entry> files(fs::directory_iterator{dir}, fs::directory_iterator{});
    for (auto file: files)
    {
      if (file.path().extension() != ".exr")
        continue;

      int w, h;
      auto path = file.path();
      auto path1 = path;
      std::string name = path1.replace_extension().filename().string();
      printf("%s\n", path.string().c_str());

      {
        auto data = LoadImage1fFromEXRNoFlip(path.string().c_str(), &w, &h);
        float min_v = FLT_MAX;
        float max_v = -FLT_MAX;
        for (auto &v : data)
        {
          min_v = std::min(min_v, v);
          max_v = std::max(max_v, v);
        }
        printf("Terrain height range: %f - %f\n", min_v, max_v);

        // normalize to 0..1
        // for (auto &v : data)
        //   v = (v - min_v) / (max_v - min_v);

        // add perlin noise
        add_perlin_noise(data, w, h, log2(w) - 1, 0.0003f);

        SaveImage1fToEXR(data.data(), w, h, path.string().c_str());
      }

      auto data = LoadImage1fFromEXRNoFlip(path.string().c_str(), &w, &h);
      LiteImage::Image2D<float> image_ref(w, h);
      memcpy(image_ref.data(), data.data(), w * h * sizeof(float));

      int MIN_SZ = 128;

      while (w >= MIN_SZ && h >= MIN_SZ)
      {
        uint32_t brick_size_logs[] = {0, 1, 2, 3};
        float sim_thrs[] = {0.0005f, 0.0010f, 0.0015f, 0.0025f,
                            0.0010f, 0.0020f, 0.0030f, 0.0050f,
                            0.0020f, 0.0040f, 0.0060f, 0.0100f,
                            0.0040f, 0.0080f, 0.0120f, 0.0200f};
        float q = 1.0f;

        for (int i = 12; i < sizeof(sim_thrs)/sizeof(float); i++)
        {
          float st = q*sim_thrs[i];
          int bsz = brick_size_logs[i / 4];
          //std::string scom_name = std::string(name.c_str()) + "_scom_b" + std::to_string(bsz) + "_th" + std::to_string(int(1000 * st));
          char scom_name[1024];
          snprintf(scom_name, 1024, "%s_mip%d_scom_b%d_th%d", name.c_str(), w, bsz, int(1000 * st));
          auto stat = test_scom2_compression(data.data(), w, h, image_ref, bsz, st, true, images_folder, scom_name);
          csv << name << ',' << "exr," << w << ',' << h << ',' << bsz << ','
              << st << ',' << stat.mse << ',' << stat.mae << ',' << stat.psnr << ',' << stat.scom_size << std::endl;
          printf("SCom2D error: MSE=%.2f*10-6, MAE=%.2f*10-3, PSNR=%.1f dB\n", stat.mse * 1000000, stat.mae * 1000, stat.psnr);
          printf("SCom2D size: %llu\n", (unsigned long long)stat.scom_size);
        }

        LiteImage::Image2D<float> image(w, h);
        for (int i = 0; i < w * h; i++)
        {
          image.data()[i] = data[i];
        }

        {
          size_t size = 2 * w * h; // assume 16 bit per pixel
          float mse = image_metrics::MSE(image_ref, image);
          float mae = image_metrics::MAE(image_ref, image);
          float psnr = image_metrics::PSNR(image_ref, image);

          csv << name << ',' << "exr," << w << ',' << h << ",,,"
              << mse << ',' << mae << ',' << psnr << ',' << size << std::endl;
        }

        {
          auto path_png = path;
          path_png.replace_extension(".png");
          std::system((std::string("oiiotool \"") + path.string() + "\" -d uint16 -o \"" + path_png.string() + "\"").c_str());

          LiteImage::Image2D<float> image_png(w, h);
          int channels;
          printf("%s\n", path_png.string().c_str());
          auto *data_png = stbi_load_16(path_png.string().c_str(), &w, &h, &channels, 1);
          if (!data_png)
          {
            printf("Couldn't open %s PNG file\n", name.c_str());
          }
          else
          {
            printf("Channels: %d\n", channels);
            size_t size = fs::file_size(path_png);
            for (int i = 0; i < w * h; i++)
            {
              image_png.data()[i] = data_png[channels * i] / 65535.0f;
            }

            stbi_image_free(data_png);
            float mse = image_metrics::MSE(image_ref, image_png);
            float mae = image_metrics::MAE(image_ref, image_png);
            float psnr = image_metrics::PSNR(image_ref, image_png);

            csv << name << ',' << "png," << w << ',' << h << ",,,"
                << mse << ',' << mae << ',' << psnr << ',' << size << std::endl;

            printf("PNG error: MSE=%.2f*10-6, MAE=%.2f*10-3, PSNR=%.1f dB\n", mse * 1000000, mae * 1000, psnr);
            printf("PNG size: %llu\n", (unsigned long long)size);
          }
        }

        {
          auto path_jpg = path;
          path_jpg.replace_extension(".jpg");
          std::system((std::string("oiiotool \"") + path.string() + "\" --compression jpeg:100 -d uint16 -o \"" + path_jpg.string() + "\"").c_str());

          LiteImage::Image2D<float> image_jpg(w, h);
          int channels;
          printf("%s\n", path_jpg.string().c_str());
          auto *data_jpg = stbi_load_16(path_jpg.string().c_str(), &w, &h, &channels, 0);
          if (!data_jpg)
          {
            printf("Couldn't open %s JPEG file\n", name.c_str());
          }
          else
          {
            printf("Channels: %d\n", channels);
            size_t size = fs::file_size(path_jpg);
            for (int j = 0; j < w * h; j++)
            {
              image_jpg.data()[j] = float(data_jpg[j * channels]) / 65535.0f;
            }

            stbi_image_free(data_jpg);
            float mse = image_metrics::MSE(image_jpg, image_ref);
            float mae = image_metrics::MAE(image_jpg, image_ref);
            float psnr = image_metrics::PSNR(image_jpg, image_ref);

            csv << name << ',' << "jpg," << w << ',' << h << ",,,"
                << mse << ',' << mae << ',' << psnr << ',' << size << std::endl;

            printf("JPG error: MSE=%.2f*10-6, MAE=%.2f*10-3, PSNR=%.1f dB\n", mse * 1000000, mae * 1000, psnr);
            printf("JPG size: %llu\n", (unsigned long long)size);
          }
        }

        // install amd compressonator
        {
          auto path_dds = path;
          path_dds.replace_extension(".dds");
          std::system((std::string("compressonatorcli -fd BC6H -Quality 0.6 -EncodeWith GPU \"") + path.string() + "\" \"" + path_dds.string() + "\"").c_str());
          printf("%s\n", path_dds.string().c_str());

          auto path_restored = path;
          path_restored.replace_extension();
          path_restored = fs::path(path_restored.string() + "_BC6H_restored.exr");
          std::system((std::string("compressonatorcli \"" + path_dds.string() + "\" \"" + path_restored.string() + "\"")).c_str());
          printf("%s\n", path_restored.string().c_str());

          auto data_bc = LoadImage1fFromEXRNoFlip(path_restored.string().c_str(), &w, &h);
          LiteImage::Image2D<float> image_bc(w, h);
          if (data_bc.empty())
          {
            printf("Couldn't open %s BC decompressed file\n", name.c_str());
          }
          else
          {
            size_t size = fs::file_size(path_dds);
            for (int j = 0; j < w * h; j++)
            {
              image_bc.data()[j] = data_bc[j];
            }

            float mse = image_metrics::MSE(image_bc, image_ref);
            float mae = image_metrics::MAE(image_bc, image_ref);
            float psnr = image_metrics::PSNR(image_bc, image_ref);

            csv << name << ',' << "BC6H," << w << ',' << h << ",,,"
                << mse << ',' << mae << ',' << psnr << ',' << size << std::endl;

            printf("BC6H error: MSE=%.2f*10-6, MAE=%.2f*10-3, PSNR=%.1f dB\n", mse * 1000000, mae * 1000, psnr);
            printf("BC6H size: %llu\n", (unsigned long long)size);
          }
        }

        if (w > MIN_SZ && h > MIN_SZ)
        {
          std::vector<float> shrinked_data;
          std::string path_name = shrink_exr(image.data(), w, h, name, images_folder, shrinked_data);
          path = fs::path(path_name);
          data = shrinked_data;
        }
        w /= 2;
        h /= 2;
      }
    }
    csv.close();
    stbi_set_flip_vertically_on_load(0);
  }
}