#pragma once

#include <vector>
#include <cstdint>
#include <fstream>

#include "LiteMath.h"

using LiteMath::float4;
using LiteMath::float4x4;
using LiteMath::uint4;

class Antialiaser
{
public:
    Antialiaser() {}
    virtual ~Antialiaser() {}
    virtual void Antialias(int width, int height, const float4* in_data [[size("width*height")]], uint32_t* out_data [[size("width*height")]]);
    virtual void AntialiasAfterDenoiser(int width, int height, const float4* in_data [[size("width*height")]], uint32_t* out_data [[size("width*height")]]);
    virtual void CommitDeviceData() {};                                       // will be overriden in generated class
    virtual void GetExecutionTime(const char* a_funcName, float a_out[4]) { } // will be overriden in generated class
    std::vector<float4> m_AABuffer;
    std::vector<float4> m_AABuffer1;
    float4x4 m_previousMatrix;
    float4x4 m_currentMatrix;
    float4x4 m_invProj;
    float4x4 m_invWorldView;
    float m_AAFactor = 0.1f;
    uint32_t m_AAFrameNum = 0;

protected:
    virtual void kernel2D_blend(int width, int height, const float4* in_data, uint32_t* out_data);
    void initEyeRay(uint32_t x, uint32_t y, int width, int height, LiteMath::float4* rayPosAndNear, LiteMath::float4* rayDirAndFar);
};