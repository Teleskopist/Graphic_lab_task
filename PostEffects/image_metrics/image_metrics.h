#pragma once
#include <chrono>
#include <cstddef>
#include "LiteMath.h"

using LiteMath::float3;
using LiteMath::float4;
using LiteMath::uint3;
using LiteMath::uint4;

class ImgMetricsKernel
{
public:
  ImgMetricsKernel(){}
  virtual ~ImgMetricsKernel() {}
  virtual void CalcArraySumm(
    const uint32_t* rt_data [[size("dataSize")]], 
    const uint32_t* ref_data [[size("dataSize")]],
    size_t dataSize);
  virtual void kernel1D_ArraySumm(
    const uint32_t* rt_data [[size("dataSize")]], 
    const uint32_t* ref_data [[size("dataSize")]],
    size_t dataSize);
  float m_summ = 0.0f;
  virtual void CommitDeviceData() {}                                                                    // will be overriden in generated class
  virtual void GetExecutionTime(const char* a_funcName, float a_out[4]) { a_out[0] = m_executionTime; } // will be overriden in generated class
  float m_executionTime = 0.0f;
};
