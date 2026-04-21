//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_slang.h"

RWStructuredBuffer<float4> out_color; // 
RWStructuredBuffer<float4> m_colorBuffer; // 
RWStructuredBuffer<float4> m_colored_grid; // 
RWStructuredBuffer<float> m_grid; // 
RWStructuredBuffer<uint> m_packedXY; // 
StructuredBuffer<VolumeRenderer_slang_UBO_Data> ubo;

float4 Sample(float3 pos) ;

float4 SampleDC(float3 p, uint a_frame_id) ;

void RaytraceGrid_SS_shadow(float4 rayPos, float4 rayDir, uint x, uint y, uint bounce, bool is_shadow, inout float3 out_C, inout float3 out_T) ;

void RaytraceGrid_SS(float4 rayPos, float4 rayDir, uint x, uint y, uint bounce, bool is_shadow, inout float3 out_C, inout float3 out_T) ;

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;

float4 RaymarchGrid_SS(float4 posAndNear, float4 dirAndFar, uint x, uint y) ;

float SampleD(float3 p, uint a_frame_id) ;



float4 Sample(float3 pos) {
  float4 sample0 = ubo[0].m_grid_type == TYPE_DENSITY_GRID ? 
                   float4(ubo[0].m_baseColor.x,ubo[0].m_baseColor.y,ubo[0].m_baseColor.z,SampleD(pos, ubo[0].frame0)) : 
                   SampleDC(pos, ubo[0].frame0);
  float4 sample1 = ubo[0].frame0 == ubo[0].frame1 ? sample0 : (ubo[0].m_grid_type == TYPE_DENSITY_GRID ? 
                                                 float4(ubo[0].m_baseColor.x,ubo[0].m_baseColor.y,ubo[0].m_baseColor.z,SampleD(pos, ubo[0].frame1)) : 
                                                 SampleDC(pos, ubo[0].frame1));
  return (1.0f - ubo[0].m_dframe) * sample0 + ubo[0].m_dframe * sample1;
}

float4 SampleDC(float3 p, uint a_frame_id) {
  uint off = a_frame_id*ubo[0].m_vox_cnt;
  float3 fp = 0.5f*float(ubo[0].m_sz - 1) * (p + 1.0f);
  uint3 ip = uint3(fp);
  uint3 ip0 = clamp(ip, uint3(0u), uint3(ubo[0].m_sz-1));
  uint3 ip1 = clamp(ip + uint3(1u), uint3(0u), uint3(ubo[0].m_sz-1));
  float3 dp = fp - float3(ip);
  
  return (1.0f-dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * m_colored_grid[off+ip0.x*ubo[0].m_sz*ubo[0].m_sz + ip0.y*ubo[0].m_sz + ip0.z] +
         (     dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * m_colored_grid[off+ip0.x*ubo[0].m_sz*ubo[0].m_sz + ip0.y*ubo[0].m_sz + ip1.z] +
         (1.0f-dp.z) * (     dp.y) * (1.0f-dp.x) * m_colored_grid[off+ip0.x*ubo[0].m_sz*ubo[0].m_sz + ip1.y*ubo[0].m_sz + ip0.z] +
         (     dp.z) * (     dp.y) * (1.0f-dp.x) * m_colored_grid[off+ip0.x*ubo[0].m_sz*ubo[0].m_sz + ip1.y*ubo[0].m_sz + ip1.z] +
         (1.0f-dp.z) * (1.0f-dp.y) * (     dp.x) * m_colored_grid[off+ip1.x*ubo[0].m_sz*ubo[0].m_sz + ip0.y*ubo[0].m_sz + ip0.z] +
         (     dp.z) * (1.0f-dp.y) * (     dp.x) * m_colored_grid[off+ip1.x*ubo[0].m_sz*ubo[0].m_sz + ip0.y*ubo[0].m_sz + ip1.z] +
         (1.0f-dp.z) * (     dp.y) * (     dp.x) * m_colored_grid[off+ip1.x*ubo[0].m_sz*ubo[0].m_sz + ip1.y*ubo[0].m_sz + ip0.z] +
         (     dp.z) * (     dp.y) * (     dp.x) * m_colored_grid[off+ip1.x*ubo[0].m_sz*ubo[0].m_sz + ip1.y*ubo[0].m_sz + ip1.z];
}

void RaytraceGrid_SS_shadow(float4 rayPos, float4 rayDir, uint x, uint y, uint bounce, bool is_shadow, inout float3 out_C, inout float3 out_T) {
  float3 m_LightDir = normalize(float3(1,1,1));
  float3 m_LightColor = float3(1,1,1);

  // Normalize rayDir
  const float3 dir = to_float3(rayDir);
  const float3 dir_inv = SafeInverse(dir);

  float step = 1.0f / ubo[0].m_sz; // half voxel size for smooth interpolation

  float2 tNearFar = RayBoxIntersection2(to_float3(rayPos), SafeInverse(dir), float3(-1.0f,-1.0f,-1.0f), float3(1.0f,1.0f,1.0f));
  tNearFar.x = max(tNearFar.x, rayPos.w);
  tNearFar.y = min(tNearFar.y, rayDir.w);
  float t = tNearFar.x + (is_shadow ? 0.0f*step : rand2(ubo[0].m_seed, x, y, ubo[0].m_AAFrameNum).x*step);
  float3 pos = to_float3(rayPos) + t * dir;

  float3 T = float3(1.0f);
  float3 C = float3(0.0f);

  while (t < tNearFar.y && T.x > ubo[0].m_preset.alpha_thr)
  {
    float3 pos = to_float3(rayPos) + t * dir;
    float4 sample = Sample(pos);
    float density = sample.w;
    float3 sampleT = float3(exp(-1.0f*density * step));

    float3 light_attenuation = float3(1.0f);
    // if (density > m_preset.alpha_thr && is_shadow == false)
    // {
    //   float3 lightC;
    //   RaytraceGrid_SS_shadow(rayPos, rayDir, x, y, bounce+1, true, &C, &light_attenuation);
    // }

    C += T * (1-sampleT) * m_LightColor * light_attenuation * to_float3(sample);
    T *= sampleT; // composite


    t += step;
  }

  out_C = C;
  out_T = T;
}

void RaytraceGrid_SS(float4 rayPos, float4 rayDir, uint x, uint y, uint bounce, bool is_shadow, inout float3 out_C, inout float3 out_T) {
  float3 m_LightDir = normalize(float3(1,1,1));
  float3 m_LightColor = float3(1,1,1);

  // Normalize rayDir
  const float3 dir = to_float3(rayDir);
  const float3 dir_inv = SafeInverse(dir);

  float step = 1.0f / float(ubo[0].m_preset.voxel_traversal_steps*ubo[0].m_sz); // half voxel size for smooth interpolation

  float2 tNearFar = RayBoxIntersection2(to_float3(rayPos), SafeInverse(dir), float3(-1.0f,-1.0f,-1.0f), float3(1.0f,1.0f,1.0f));
  tNearFar.x = max(tNearFar.x, rayPos.w);
  tNearFar.y = min(tNearFar.y, rayDir.w);
  float t = tNearFar.x + (is_shadow ? 0.5f*step : rand2(ubo[0].m_seed, x, y, ubo[0].m_AAFrameNum).x*step);
  float3 pos = to_float3(rayPos) + t * dir;

  float3 T = float3(1.0f);
  float3 C = float3(0.0f);

  while (t < tNearFar.y && T.x > ubo[0].m_preset.alpha_thr)
  {
    float3 pos = to_float3(rayPos) + t * dir;
    float4 sample = Sample(pos);
    float density = sample.w;
    float3 sampleT = float3(exp(-1.0f*density * step));

    float3 light_attenuation = float3(1.0f);
    if (ubo[0].m_preset.use_lighting == 1 && density > 3.0f*ubo[0].m_preset.alpha_thr)
    {
      float3 lightC;
      RaytraceGrid_SS_shadow(to_float4(pos, 0.0001f), to_float4(m_LightDir, rayDir.w), x, y, bounce+1, true, lightC, light_attenuation);
    }

    C += T * (1-sampleT) * m_LightColor * light_attenuation * to_float3(sample);
    T *= sampleT; // composite

    t += step;
  }

  out_C = C;
  out_T = T;
}

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) {
  uint spp_sqrt = uint32_t(sqrt(ubo[0].m_preset.spp));
  float i_spp_sqrt = 1.0f/spp_sqrt;
  float2 d = (ubo[0].m_AAFrameNum >= 2) ? rand2(ubo[0].m_seed, x, y, s_id + ubo[0].m_AAFrameNum + ubo[0].m_seed % 2) : 
                                   i_spp_sqrt*float2(s_id/spp_sqrt+0.5,s_id%spp_sqrt+0.5);

  float3 rayDir = EyeRayDirNormalized((float(x)+d.x)/float(ubo[0].m_width), (float(y)+d.y)/float(ubo[0].m_height), ubo[0].m_projInv);
  float3 rayPos = float3(0,0,0);

  transform_ray3f(ubo[0].m_worldViewInv, 
                  rayPos, rayDir);
  
  float rayNear = 1e-5f;
  float rayFar  =  1e5f;
  float sgn = sign(dot(rayPos - ubo[0].m_cuttingPlane.pos, ubo[0].m_cuttingPlane.normal));
  float denom = ubo[0].m_cuttingPlane.is_active > 0 ? dot(ubo[0].m_cuttingPlane.normal, rayDir) : 0.0f;
  rayNear = sgn*abs(denom) < -1e-6f ? dot(ubo[0].m_cuttingPlane.pos - rayPos, ubo[0].m_cuttingPlane.normal) / denom : rayNear;
  rayFar  = sgn*abs(denom) >  1e-6f ? dot(ubo[0].m_cuttingPlane.pos - rayPos, ubo[0].m_cuttingPlane.normal) / denom : rayFar;
  rayPosAndNear = to_float4(rayPos, rayNear < 0 ? rayFar : rayNear);
  rayDirAndFar  = to_float4(rayDir, rayFar < 0 ?    1e9f :  rayFar);
}

float4 RaymarchGrid_SS(float4 posAndNear, float4 dirAndFar, uint x, uint y) {
  float3 C = float3(0.0f);
  float3 T = float3(1.0f);
  RaytraceGrid_SS(posAndNear, dirAndFar, 0, 0, 0, false, C, T);
  return to_float4(C + T*to_float3(ubo[0].m_backgroundColor), 1);
}

float SampleD(float3 p, uint a_frame_id) {
  uint off = a_frame_id*ubo[0].m_vox_cnt;
  float3 fp = 0.5f*float(ubo[0].m_sz - 1) * (p + 1.0f);
  uint3 ip = uint3(fp);
  uint3 ip0 = clamp(ip, uint3(0u), uint3(ubo[0].m_sz-1));
  uint3 ip1 = clamp(ip + uint3(1u), uint3(0u), uint3(ubo[0].m_sz-1));
  float3 dp = fp - float3(ip);
  
  return (1.0f-dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * m_grid[off+ip0.x*ubo[0].m_sz*ubo[0].m_sz + ip0.y*ubo[0].m_sz + ip0.z] +
         (     dp.z) * (1.0f-dp.y) * (1.0f-dp.x) * m_grid[off+ip0.x*ubo[0].m_sz*ubo[0].m_sz + ip0.y*ubo[0].m_sz + ip1.z] +
         (1.0f-dp.z) * (     dp.y) * (1.0f-dp.x) * m_grid[off+ip0.x*ubo[0].m_sz*ubo[0].m_sz + ip1.y*ubo[0].m_sz + ip0.z] +
         (     dp.z) * (     dp.y) * (1.0f-dp.x) * m_grid[off+ip0.x*ubo[0].m_sz*ubo[0].m_sz + ip1.y*ubo[0].m_sz + ip1.z] +
         (1.0f-dp.z) * (1.0f-dp.y) * (     dp.x) * m_grid[off+ip1.x*ubo[0].m_sz*ubo[0].m_sz + ip0.y*ubo[0].m_sz + ip0.z] +
         (     dp.z) * (1.0f-dp.y) * (     dp.x) * m_grid[off+ip1.x*ubo[0].m_sz*ubo[0].m_sz + ip0.y*ubo[0].m_sz + ip1.z] +
         (1.0f-dp.z) * (     dp.y) * (     dp.x) * m_grid[off+ip1.x*ubo[0].m_sz*ubo[0].m_sz + ip1.y*ubo[0].m_sz + ip0.z] +
         (     dp.z) * (     dp.y) * (     dp.x) * m_grid[off+ip1.x*ubo[0].m_sz*ubo[0].m_sz + ip1.y*ubo[0].m_sz + ip1.z];
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct KernelArgs
{
  uint sample_id;
  uint count; 
  uint iNumElementsY; 
  uint iNumElementsZ; 
  uint tFlagsMask;    
};



[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 a_globalTID: SV_DispatchThreadID, uint3 a_localTID : SV_GroupThreadID, uniform KernelArgs kgenArgs)
{
  bool runThisThread = true;
  const uint tidX = uint(a_globalTID[0]); 
  if(tidX >= kgenArgs.count + 0)
    runThisThread = false;
    // KERNEL BODY:
  //
  if(runThisThread)
  {
  
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, kgenArgs.sample_id, rayPos, rayDir);
    float4 res = RaymarchGrid_SS(rayPos, rayDir, x, y);
    m_colorBuffer[y*ubo[0].m_width + x] = kgenArgs.sample_id == 0 ? res : m_colorBuffer[y*ubo[0].m_width + x] + res;
  
  }
}

