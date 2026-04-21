//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_comp_rq_slang.h"

RWStructuredBuffer<float4> out_color; // 
RWStructuredBuffer<CRT_Hit> m_gBuffer; // 
RWStructuredBuffer<uint2> m_geomOffsets; // 
RWStructuredBuffer<uint> m_indices; // 
RWStructuredBuffer<float4x4> m_instanceTransformInvTransposed; // 
RWStructuredBuffer<Light> m_lights; // 
RWStructuredBuffer<float4> m_normals; // 
RWStructuredBuffer<float4> m_vertices; // 
RWStructuredBuffer<MultiRenderer_slang_comp_rq_UBO_Data> ubo;

float4 ResolveOutline(uint tidX, uint mode) ;

void loadNormalTcFromGbuffer(CRT_Hit hit, float3 ray_dir, inout float3 norm, inout float2 tc) ;

void initEyeRay(uint x, uint y, uint a_sample_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;



float4 ResolveOutline(uint tidX, uint mode) {
  CRT_Hit hit = m_gBuffer[tidX];

  if (hit.primId == 0xFFFFFFFF) // no hit
    return ubo[0].m_backgroundColor;

  uint type = hit.geomId >> SH_TYPE;
  uint geomId = hit.geomId & GEOM_ID_MASK;
  float4 rayPos, rayDir;
  initEyeRay(tidX % ubo[0].m_packedXY_width, tidX / ubo[0].m_packedXY_width, 0, rayPos, rayDir);

  float2 tc = float2(0,0);
  float3 norm = float3(1,0,0);
  loadNormalTcFromGbuffer(hit, to_float3(rayDir), norm, tc);

  float3 color = float3(1,1,1);

  float3 final_color = float3(0,0,0);
  for (int i = 0; i < ubo[0].m_lights_size; i++)
  {
    if (m_lights[i].type == LIGHT_TYPE_DIRECT)
    {
      float q = max(0.0f, dot(norm, m_lights[i].space));
      final_color += m_lights[i].color * color * q;
    }
    else if (m_lights[i].type == LIGHT_TYPE_POINT)
    {
      float3 surf_pos = to_float3(rayPos) + hit.t * to_float3(rayDir);
      float3 dir = m_lights[i].space - surf_pos;
      float l = length(dir);
      dir /= l;
      float q = max(0.0f, dot(norm, dir));
      final_color += m_lights[i].color * color * q / (l * l);
    }
    else
      final_color += m_lights[i].color * color;
  }

  bool outline = false;
  const uint x = tidX%ubo[0].m_packedXY_width;
  const uint y = tidX/ubo[0].m_packedXY_width;
  int4 offsets = int4(x == 0 ? 0 : -1,y == 0 ? 0 : -int(ubo[0].m_packedXY_width),x == ubo[0].m_packedXY_width - 1 ? 0 : 1,y == ubo[0].m_packedXY_height - 1 ? 0 : int(ubo[0].m_packedXY_width));
  offsets += int(tidX);

  if (mode == MULTI_RENDER_MODE_OUTLINE_BORDER)
  {
    const float tolerance = 10.0f*hit.t/float(ubo[0].m_packedXY_width);
    const float4 nearDist = float4(m_gBuffer[offsets.x].t,m_gBuffer[offsets.y].t,m_gBuffer[offsets.z].t,m_gBuffer[offsets.w].t);
    const float4 diff = abs(nearDist - hit.t);
    outline = diff.x > tolerance || diff.y > tolerance || diff.z > tolerance || diff.w > tolerance;
  }

  return outline ? float4(0.2,0.2,0.2,1) : to_float4(final_color, 1);
}

void loadNormalTcFromGbuffer(CRT_Hit hit, float3 ray_dir, inout float3 norm, inout float2 tc) {
  uint type = hit.geomId >> SH_TYPE;
  uint geomId = hit.geomId & GEOM_ID_MASK;

  if (type == TYPE_MESH_TRIANGLE)
  {
#ifndef DISABLE_MESH
    const uint2 a_geomOffsets = m_geomOffsets[geomId];
    const uint A = m_indices[a_geomOffsets.x + hit.primId * 3 + 0];
    const uint B = m_indices[a_geomOffsets.x + hit.primId * 3 + 1];
    const uint C = m_indices[a_geomOffsets.x + hit.primId * 3 + 2];

    const float2 A_tc = float2(m_vertices[a_geomOffsets.y + A].w,m_normals[a_geomOffsets.y + A].w);
    const float2 B_tc = float2(m_vertices[a_geomOffsets.y + B].w,m_normals[a_geomOffsets.y + B].w);
    const float2 C_tc = float2(m_vertices[a_geomOffsets.y + C].w,m_normals[a_geomOffsets.y + C].w);

    float3 raw_norm = float3(1,0,0);
    if (ubo[0].m_preset.normal_mode == NORMAL_MODE_GEOMETRY)
    {
      const float3 posA = to_float3(m_vertices[a_geomOffsets.y + A]);
      const float3 posB = to_float3(m_vertices[a_geomOffsets.y + B]);
      const float3 posC = to_float3(m_vertices[a_geomOffsets.y + C]);
      const float3 edge1 = posB - posA;
      const float3 edge2 = posC - posA;
      raw_norm = cross(edge1, edge2);
    }
    else if (ubo[0].m_preset.normal_mode == NORMAL_MODE_VERTEX)
    {
      const float4 normA = m_normals[a_geomOffsets.y + A];
      const float4 normB = m_normals[a_geomOffsets.y + B];
      const float4 normC = m_normals[a_geomOffsets.y + C];
      const float2 uv = float2(hit.coords[0],hit.coords[1]);
      const float4 norm4 = (1.0f - uv.x - uv.y) * normA + uv.y * normB + uv.x * normC;
      raw_norm = to_float3(norm4);
    }

    tc = (1.0f - hit.coords[0] - hit.coords[1]) * A_tc + hit.coords[1] * B_tc + hit.coords[0] * C_tc;
    norm = normalize(matmul4x3(m_instanceTransformInvTransposed[hit.instId], raw_norm));
    float norm_sign = sign(dot(-1.0f * ray_dir, norm));
    norm = (norm) * norm_sign;
#endif
  }
  else
  {
    tc = float2(hit.coords[0],hit.coords[1]);
    norm = decode_normal(float2(hit.coords[2],hit.coords[3]));
  }
}

void initEyeRay(uint x, uint y, uint a_sample_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) {
  float2 HamiltonSequence[16] = {
    float2(0.500000,0.333333),
    float2(0.250000,0.666667),
    float2(0.750000,0.111111),
    float2(0.125000,0.444444),
    float2(0.625000,0.777778),
    float2(0.375000,0.222222),
    float2(0.875000,0.555556),
    float2(0.062500,0.888889),
    float2(0.562500,0.037037),
    float2(0.312500,0.370370),
    float2(0.812500,0.703704),
    float2(0.187500,0.148148),
    float2(0.687500,0.481481),
    float2(0.437500,0.814815),
    float2(0.937500,0.259259),
    float2(0.031250,0.592593)
  };

  uint spp_sqrt = uint32_t(sqrt(ubo[0].m_preset.spp));
  float i_spp_sqrt = 1.0f/spp_sqrt;
  float2 d = (ubo[0].m_AAFrameNum >= 2 || ubo[0].m_preset.ray_gen_mode == RAY_GEN_MODE_RANDOM) ? 
              /*rand2(m_seed, x, y, a_sample_id + m_AAFrameNum + m_seed % 2)*/HamiltonSequence[ubo[0].m_AAFrameNum % 16] : i_spp_sqrt*float2(a_sample_id/spp_sqrt+0.5,a_sample_id%spp_sqrt+0.5);


  float3 rayDir = EyeRayDirNormalized((float(x)+d.x)/float(ubo[0].m_packedXY_width), (float(y)+d.y)/float(ubo[0].m_packedXY_height), ubo[0].m_projInv);
  float3 rayPos = float3(0,0,0);

  transform_ray3f(ubo[0].m_worldViewInv, 
                  rayPos, rayDir);
  
  float rayNear = 1e-9f;
  float rayFar  =  1e9f;
  float sgn = sign(dot(rayPos - ubo[0].m_cuttingPlane.pos, ubo[0].m_cuttingPlane.normal));
  float denom = ubo[0].m_cuttingPlane.is_active > 0 ? dot(ubo[0].m_cuttingPlane.normal, rayDir) : 0.0f;
  rayNear = sgn*abs(denom) < -1e-6f ? dot(ubo[0].m_cuttingPlane.pos - rayPos, ubo[0].m_cuttingPlane.normal) / denom : rayNear;
  rayFar  = sgn*abs(denom) >  1e-6f ? dot(ubo[0].m_cuttingPlane.pos - rayPos, ubo[0].m_cuttingPlane.normal) / denom : rayFar;
  rayPosAndNear = to_float4(rayPos, rayNear < 0 ? rayFar : rayNear);
  rayDirAndFar  = to_float4(rayDir, rayFar < 0 ?    1e9f :  rayFar);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct KernelArgs
{
  uint sample_id;
  uint render_mode;
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
  
    if (kgenArgs.sample_id == 0)
      out_color[tidX] = ResolveOutline(tidX, kgenArgs.render_mode);
    else
      out_color[tidX] += ResolveOutline(tidX, kgenArgs.render_mode);
  
  }
}

