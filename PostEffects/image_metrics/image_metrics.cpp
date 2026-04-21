#include "image_metrics.h"
#include <chrono>

static float4 decode_RGBA8(uint32_t c)
  {
    uint4 col = uint4(c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF, (c >> 24) & 0xFF);
    return float4(col.x * (1.0f / 255.0f), col.y * (1.0f / 255.0f), col.z * (1.0f / 255.0f), col.w * (1.0f / 255.0f));
  }

void ImgMetricsKernel::CalcArraySumm(const uint32_t* rt_data, const uint32_t* ref_data, size_t dataSize)
{
  auto before = std::chrono::high_resolution_clock::now();
  kernel1D_ArraySumm(rt_data, ref_data, dataSize);
  m_executionTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - before).count()/1000.f;
}

void ImgMetricsKernel::kernel1D_ArraySumm(const uint32_t* rt_data, const uint32_t* ref_data, size_t dataSize)
{
  m_summ = 0;
#if !defined(_MSC_VER) || defined(__clang__)
  #pragma omp parallel for reduction(+:m_summ) // num_threads(4)
#endif
  for(int i=0; i<dataSize; i++)
  {
    float4 col1 = decode_RGBA8(rt_data[i]);
    float4 col2 = decode_RGBA8(ref_data[i]);
    m_summ += dot(float4(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f, 0.0f), (col1 - col2)*(col1 - col2));
  }
}