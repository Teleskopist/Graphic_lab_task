#pragma once
#include "utils/common/data_channel_info.h"

struct ICAERenderer
{
  ICAERenderer(){ }
  virtual ~ICAERenderer(){}

  virtual bool hasCAEData() const = 0;
  virtual std::vector<DataChannelInfo> getCAEPointChannelInfo() const = 0;
  virtual std::vector<DataChannelInfo> getCAEVoxelChannelInfo() const = 0;
};