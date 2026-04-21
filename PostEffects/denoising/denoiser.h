#pragma once

#include <vector>
#include <cstdint>

#include "LiteMath.h"

using LiteMath::float4;
using LiteMath::float3;
using vec4 = LiteMath::float4;
using LiteMath::float2;
using LiteMath::uint4;
using LiteMath::length;
using LiteMath::max;
using LiteMath::min;

class Denoiser
{
public:
    Denoiser() {}
    virtual ~Denoiser() {}
    virtual void Denoise(int width, int height, const float4* in_data [[size("width*height")]], uint32_t* out_data [[size("width*height")]]);
    virtual void DenoiseFloat(int width, int height, const float4* in_data [[size("width*height")]], float4* out_data [[size("width*height")]]);
    virtual void CommitDeviceData() {};                                       // will be overriden in generated class
    virtual void GetExecutionTime(const char* a_funcName, float a_out[4]) { } // will be overriden in generated class

    float m_SigmaR = 1.0f;
    float m_SigmaS = 1.0f;
    int m_KernelSize = 5;

protected:
    virtual void kernel2D_filter(int width, int height, const float4* in_data, uint32_t* out_data);
    virtual void kernel2D_filterFloat(int width, int height, const float4* in_data, float4* out_data);
    virtual void kernel2D_fastFilter(int width, int height, const float4* in_data, uint32_t* out_data);
    virtual void kernel2D_fastFilterFloat(int width, int height, const float4* in_data, float4* out_data);

    float Kr(float p);
    float Ks(float p);
};