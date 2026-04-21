#pragma once
#include "scom.h"
#include "scom_builder.h"
#include "scom_internal.h"

#include "utils/common/position_hash.h"

namespace scom2
{
  struct ChannelOffsets
  {
    uint32_t data_off = 0;
    uint32_t transforms_off = 0;
    uint32_t descriptor_off = 0;
  };

  struct ChannelInfo
  {
    bool is_point_channel = false;
    const DataChannel *channel = nullptr;
    ChannelCustomSettings settings;
  };

  struct ConsolidatedNode
  {
    uint32_t original_id = 0;   // id of the node in DAG
    BrickInfo::Type type = BrickInfo::Type::EMPTY;   // type of all the bricks
    uint8_t level = 0;          // level of the node in the octree
    uint16_t brick_count = 0;   // number of bricks in this node
    uint32_t values_offset = 0; // offset of the values for this node (values for all bricks)
    uint32_t channels_values_offset = 0; // offset of the channel values for this node (channel descriptors)
    uint32_t ch_transforms_offset = 0; // offset in ch_transforms for this node
  };

  struct ConsolidationLevel
  {
    uint32_t all_nodes_count = 0;               // number of all nodes on this level
    uint32_t active_nodes_count = 0;            // number of active nodes on this level
    uint32_t brick_size = 1;                    // number of voxels in brick on this consolidated level
    uint32_t brick_pad  = 0;                    // padding in brick on this consolidated level (same on all levels)
    uint32_t descriptor_size = 8;
    uint32_t channels_descriptor_size = 0;
    uint32_t channels_data_size = 0;
    uint32_t channels_transforms_size = 0;

    const Subgroup *transform_subgroup = nullptr;
    const Subgroup *channel_transform_subgroup = nullptr;
    std::vector<ConsolidatedNode> active_nodes; // list of active nodes
    std::vector<uint32_t> node_id_by_global_id; // if node with this global idx is active, it has this local idx, otherwise INVALID_IDX
    std::vector<float> values;                  // distance values for all consolidated bricks of this level
    std::vector<float> ch_values;               // channel values for all consolidated bricks of this level
    std::vector<uint16_t> ch_transforms;        // channel transforms for all consolidated bricks of this level
    SubDatasetMap<std::vector<uint32_t>> sub_datasets; // separate datasets with different descriptors
    std::vector<ChannelOffsets> channel_offsets;
  };

  // Data collected in SComDatasetBranches methods
  // Useful for debug and collecting statistics for compression
  // Can consume a lot of memory and slow down compression process
  struct BranchesCompressionLog
  {
    enum class BranchType
    {
      EMPTY,
      LEAF,
      CONSOLIDATED
    };
    struct MultiBrickInfo
    {
      BrickInfo::Type type;
      uint32_t v_size = 0;
      uint32_t brick_count = 0;
      std::vector<float> values;
    };
    struct LevelInfo
    {
      uint32_t grid_size = 1;
      std::vector<MultiBrickInfo> multibricks;
    };
    
    struct BranchInfo
    {
      uint32_t start_depth = 0;
      BranchType type;
      std::vector<LevelInfo> levels;
    };

    CodeMap<BranchInfo> branch_map;
  };

  class SComDatasetBranches : public ISComDataset
  {
  public:
    static constexpr uint32_t LEVEL_UNKNOWN        = 1000; // We haven't calculated consolidated level for this node yet
    static constexpr uint32_t NO_LEVEL             = 1001; // This node is empty, thus has no consolidation level
    static constexpr uint32_t LEVEL_UNCONSOLIDATED = 1002; // We tried to consolidate this node, but failed, probably
                                                           // it should be on higher consolidation levels
    SComDatasetBranches(const SdfDAG *_dag, 
                        std::shared_ptr<IDescriptorMaker> _brick_descriptor_maker,
                        std::shared_ptr<IDescriptorBinner> _brick_descriptor_binner,
                        const std::vector<ChannelCustomSettings> &_pc_custom_settings,
                        const std::vector<ChannelCustomSettings> &_vc_custom_settings,
                        EmbeddingType _brick_embedding_type,
                        InternalBrickUse _internal_brick_use,
                        TransformSubgroup _bricks_transform_subgroup,
                        TransformSubgroup _transform_subgroup,
                        BranchDescriptor _branch_descriptor,
                        float _merge_surface_threshold);
    virtual ~SComDatasetBranches() { }
    virtual const Subgroup *get_transform_group() const override { return cons_levels.back().transform_subgroup; }
    virtual uint32_t get_descriptor_size() const override;
    virtual uint32_t get_point_count() const override { 
      auto it = cons_levels.back().sub_datasets.find(cur_sub_desc);
      if (it == cons_levels.back().sub_datasets.end())
        return 0;
      return it->second.size();
    };

    virtual void calculate_descriptors(uint32_t count, const uint32_t *p_ids, const uint16_t *rot_ids, 
                                       float *out_desc, uint32_t desc_stride) const override;

    void create_next_consolidated_level();

    std::vector<ConsolidationLevel> cons_levels;
    std::vector<uint32_t> nodes_cons_level; //consolidation level for each node, level or LEVEL_UNKNOWN, NO_LEVEL, CONSOLIDATION_FAILED
    SubDatasetDescriptor cur_sub_desc;

  private:
    void create_initial_consolidated_level();
    void create_initial_consolidated_level_rec(uint32_t node_id, uint32_t level, uint4 code);
    void write_initial_cons_node_to_bc_log(uint32_t level, uint32_t active_brick_count, uint32_t v_size, uint4 code);
    void create_next_consolidated_level_rec(MergeInData &in, MergeTmpData &tmp, uint32_t node_id, uint32_t level, uint4 code);

    void write_cons_node_to_bc_log(scom2::ConsolidationLevel &prev_level, scom2::ConsolidationLevel &next_level, uint32_t level, 
                                   scom2::ConsolidatedNode &cnode, uint32_t ch_n, uint32_t ch_count, const SdfDAGNode &node, 
                                   sdf_converter::MergeInData &in, uint4 code);

    void load_ch_data(uint32_t channel_id, const SdfDAGNode &node, bool is_point, ConsolidatedNode &cnode, MergeInData &in, MergeTmpData &tmp);

    EmbeddingType brick_embedding_type;
    InternalBrickUse internal_brick_use;
    TransformSubgroup bricks_transform_subgroup;
    TransformSubgroup transform_subgroup;
    BranchDescriptor branch_descriptor;
    float merge_surface_threshold = 0.01f;

    std::vector<std::vector<float>> upsample_tmp_values;
    std::vector<std::vector<uint16_t>> upsample_tmp_transforms;
    std::vector<uint32_t> upsample_tmp_indices;
    BranchesCompressionLog *bc_log = nullptr;
    const SdfDAG *dag;
    std::vector<ChannelInfo> channels; // all the channels that are used for similarity compression and settings for them
    std::shared_ptr<IDescriptorMaker> brick_descriptor_maker = nullptr;
    std::shared_ptr<IDescriptorBinner> brick_descriptor_binner = nullptr;
    mutable std::vector<IDescriptorMaker::Node> nodes;
  };
}