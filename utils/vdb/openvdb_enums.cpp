#include "blk/blk.h"
#include "openvdb_common.h"
#include "openvdb_enums.h"

REGISTER_ENUM(OpenVDBGridSampler,
              ([]()
               { return std::vector<std::pair<std::string, unsigned>>{
                     {"POINT_SAMPLER", POINT_SAMPLER},
                     {"BOX_SAMPLER", BOX_SAMPLER},
                     {"QUADRATIC_SAMPLER", QUADRATIC_SAMPLER},
                 }; })());

void force_register_OpenVDB_enums(){}

void load_from_blk(OpenVDBDensityGridCreateInfo &settings, const Block *block)
{
  settings = OpenVDBDensityGridCreateInfo{};
  
  settings.resolution = block->get_int("resolution", settings.resolution);
  settings.prune_tolerance = block->get_double("prune_tolerance", settings.prune_tolerance);
  settings.sampler = (OpenVDBGridSampler)block->get_enum("sampler", settings.sampler);
}

void save_to_blk(const OpenVDBDensityGridCreateInfo &settings, Block *block)
{
  block->set_int("resolution", settings.resolution);
  block->set_double("prune_tolerance", settings.prune_tolerance);
  block->set_enum("sampler", "OpenVDBGridSampler", settings.sampler);
}