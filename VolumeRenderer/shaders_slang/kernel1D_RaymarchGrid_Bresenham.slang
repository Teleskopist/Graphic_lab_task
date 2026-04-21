//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_slang.h"

RWStructuredBuffer<float4> out_color; // 
RWStructuredBuffer<float4> m_colorBuffer; // 
RWStructuredBuffer<float4> m_colored_grid; // 
RWStructuredBuffer<float> m_grid; // 
RWStructuredBuffer<uint> m_packedXY; // 
StructuredBuffer<VolumeRenderer_slang_UBO_Data> ubo;

float4 RaymarchGrid_Bresenham(float4 rayPos, float4 rayDir) ;

void initEyeRay(uint x, uint y, uint s_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;

float4 Sample(float3 pos) ;

float4 SampleDC(float3 p, uint a_frame_id) ;

float SampleD(float3 p, uint a_frame_id) ;



float4 RaymarchGrid_Bresenham(float4 rayPos, float4 rayDir) {
  // Normalize rayDir
  const float3 dir = to_float3(rayDir);
  const float3 dir_inv = SafeInverse(dir);

  float step = 1.0f / ubo[0].m_sz; // half voxel size for smooth interpolation

  float2 tNearFar = RayBoxIntersection2(to_float3(rayPos), SafeInverse(dir), float3(-1.0f,-1.0f,-1.0f), float3(1.0f,1.0f,1.0f));
  tNearFar.x = max(tNearFar.x, rayPos.w);
  tNearFar.y = min(tNearFar.y, rayDir.w);
  float alpha = 1.0f;
  float t = tNearFar.x;
  float3 pos = to_float3(rayPos) + t * dir;
  float3 accumulatedColor = float3(0.0f);

  while (t < tNearFar.y && alpha > ubo[0].m_preset.alpha_thr)
  {
    float4 sample = Sample(pos);
    float density = sample.w;

    // Standard volume raymarching blend (front-to-back compositing)
    // Here, we interpret density as alpha per step
    float localAlpha = 1.0f - exp(-density * step);
    accumulatedColor += alpha * localAlpha * to_float3(sample);
    alpha *= 1.0f - localAlpha; // composite

    t += step;
    pos = to_float3(rayPos) + t * dir;
  }

  return to_float4(accumulatedColor + alpha*to_float3(ubo[0].m_backgroundColor), 1);
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
    float4 res = RaymarchGrid_Bresenham(rayPos, rayDir);
    m_colorBuffer[y*ubo[0].m_width + x] = kgenArgs.sample_id == 0 ? res : m_colorBuffer[y*ubo[0].m_width + x] + res;
  
  }
}

