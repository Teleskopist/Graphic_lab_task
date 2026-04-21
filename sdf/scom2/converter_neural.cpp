// Compilation of this file is temporary disabled.
// The file is preserved in case we will resurrect neural embeddings in the future
// Do not add any new code to this file, change it or remove it.

#include <fstream>
#include <cstdlib>

namespace scom2
{

    // TODO: reimplement and test
    // else if (settings.embedding_type == scom2::EmbeddingType::NEURAL)
    // {
    //   if (settings.brick_size == 3 && dag.header.dim == 3)
    //   {
    //     dataset = std::make_unique<scom2::SComDatasetNEBricks>(&dag, settings.internal_brick_use, 
    //                                                            settings.bricks_transform_subgroup, settings.surface_sensitivity,
    //                                                            64, "../neural_brick/nbrick.py", "../neural_brick/checkpoint_64_64_def.pt");
    //   }
    //   else
    //   {
    //     printf("[ERROR] Neural embedding supports only 3x3x3 bricks (given size=%d, dim=%d)\n", 
    //            settings.brick_size, dag.header.dim);
    //     return;
    //   }
    // }
    
  // class SComDatasetNEBricks : public SComDatasetSDFBricks
  // {
  // public:
  //   SComDatasetNEBricks(const SdfDAG *_dag,
  //                       InternalBrickUse _internal_brick_use,
  //                       TransformSubgroup _transform_subgroup,
  //                       float _surface_sensitivity = 1.0f,
  //                       uint32_t _descriptor_size = 1024,
  //                       const char *_script_path = "../neural_brick/nbrick.py",
  //                       const char *_model_path = "../neural_brick/checkpoint.pt");
  //   virtual ~SComDatasetNEBricks() { }
  //   virtual void calculate_descriptors(uint32_t count, const uint32_t *p_ids, const uint16_t *rot_ids, 
  //                                      float *out_desc, uint32_t desc_stride) const override;
  // private:
  //   InternalBrickUse internal_brick_use;
  //   uint32_t raw_descriptor_size;
  //   std::string script_path;
  //   std::string model_path;
  // };

  // SComDatasetNEBricks::SComDatasetNEBricks(const SdfDAG *_dag,
  //                                          InternalBrickUse _internal_brick_use,
  //                                          TransformSubgroup _transform_subgroup,
  //                                          float _surface_sensitivity,
  //                                          uint32_t _descriptor_size,
  //                                          const char *_script_path,
  //                                          const char *_model_path) : 
  //   SComDatasetSDFBricks(_dag, nullptr, _internal_brick_use, _transform_subgroup, _surface_sensitivity)
  // {
  //   raw_descriptor_size = _descriptor_size;
  //   script_path = _script_path;
  //   model_path = _model_path;
  // }
  // void SComDatasetNEBricks::calculate_descriptors(uint32_t count, const uint32_t *p_ids, const uint16_t *rot_ids, 
  //                                                 float *out_desc, uint32_t desc_stride) const
  // {
  //   const char *bricks_filename = "saves/bricks.bin";
  //   const char *embed_filename =  "saves/embeddings.bin";
  //   constexpr uint32_t MAX_CMD_LEN = 512;
  //   char cmd[MAX_CMD_LEN];
  //   snprintf(cmd, MAX_CMD_LEN, "python3 %s %s %s %s %d", 
  //            script_path.c_str(), model_path.c_str(), bricks_filename, embed_filename, count);

  //   //save bricks
  //   {
  //     //fill array of all bricks
  //     std::vector<float> bricks(count * brick_values_count);
  //     auto subdataset = sub_datasets.find(cur_sub_desc);
  //     if (subdataset == sub_datasets.end())
  //     {
  //       assert(false);
  //       return;
  //     }

  //     #pragma omp parallel for schedule(static)
  //     for (uint32_t d_id = 0; d_id < count; d_id++)
  //     {
  //       uint32_t p_id = subdataset->second[p_ids[d_id]];
  //       uint32_t rot_id = rot_ids[d_id];

  //       float sum = 0.0f;
  //       for (uint32_t idx = 0; idx < brick_values_count; idx++)
  //       {
  //         uint64_t idx_rot = transform_subgroup->elements[rot_id][idx];
  //         float v = (dag->distances[data_points[p_id].values_offset + idx_rot] - data_points[p_id].average_val);
  //         bricks[d_id*brick_values_count + idx] = v;
  //         sum += v * v;
  //       }

  //       float i_sum = 1.0f / sqrtf(sum);
  //       for (int j = 0; j < brick_values_count; j++)
  //         bricks[d_id*brick_values_count + j] *= i_sum;
  //     }

  //     //save to file
  //     std::ofstream bricks_file(bricks_filename, std::ios::binary);
  //     bricks_file.write((const char *)bricks.data(), count * brick_values_count * sizeof(float));
  //     bricks_file.flush();
  //     bricks_file.close();
  //   }

  //   //call python script
  //   {
  //     printf("converting %d bricks to embeddings\n", count);
  //     int ret = system(cmd);
  //     if (ret != 0)
  //     {
  //       printf("[SComDatasetNEBricks] script failed with error code %d\n", ret);
  //       printf("[SComDatasetNEBricks] cmd: %s\n", cmd);
  //       return;
  //     }
  //   }

  //   //load embeddings
  //   {
  //     std::vector<float> embeds(count * raw_descriptor_size);
  //     std::ifstream embed_file(embed_filename, std::ios::binary);
  //     embed_file.read((char *)embeds.data(), count * raw_descriptor_size * sizeof(float));
  //     embed_file.close();

  //     for (uint32_t d_id = 0; d_id < count; d_id++)
  //     {
  //       memcpy(out_desc + d_id * desc_stride, embeds.data() + d_id * raw_descriptor_size, raw_descriptor_size * sizeof(float));

  //       float level_sensitivity = get_level_sensitivity(data_points[p_ids[d_id]].level, base_level, 2.0f);
  //       encode_sdf_brick_default(brick_values_count, data_points[p_ids[d_id]].type, level_sensitivity, surface_sensitivity, out_desc);
  //     }
  //   }
  // }
}