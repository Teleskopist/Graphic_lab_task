//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_comp_rq_slang.h"

[[vk::binding(0, 0)]] RWStructuredBuffer<float4> out_color; // 
[[vk::binding(1, 0)]] RWStructuredBuffer<CRT_Hit> m_gBuffer; // 
[[vk::binding(2, 0)]] RWStructuredBuffer<uint2> m_geomOffsets; // 
[[vk::binding(3, 0)]] RWStructuredBuffer<uint> m_indices; // 
[[vk::binding(4, 0)]] RWStructuredBuffer<float4x4> m_instanceTransformInvTransposed; // 
[[vk::binding(5, 0)]] RWStructuredBuffer<Light> m_lights; // 
[[vk::binding(6, 0)]] RWStructuredBuffer<uint2> m_matIdOffsets; // 
[[vk::binding(7, 0)]] RWStructuredBuffer<uint> m_matIdbyPrimId; // 
[[vk::binding(8, 0)]] RWStructuredBuffer<MultiRendererMaterial> m_materials; // 
[[vk::binding(9, 0)]] RWStructuredBuffer<float4> m_normals; // 
[[vk::binding(10, 0)]] Texture2D<float4> m_textures[]; //
[[vk::binding(10, 0)]] SamplerState m_textures_sam[];
[[vk::binding(11, 0)]] RWStructuredBuffer<float4> m_transparencyBuffer; // 
[[vk::binding(12, 0)]] RWStructuredBuffer<float4> m_vertices; // 
[[vk::binding(13, 0)]] RWStructuredBuffer<MultiRenderer_slang_comp_rq_UBO_Data> ubo;

float4 ResolveTransparency(uint tidX, float4 color) ;

float4 ResolveCommon(uint tidX, uint a_render_mode) ;

float4 applyHighlight(CRT_Hit hit) ;

void loadNormalTcFromGbuffer(CRT_Hit hit, float3 ray_dir, inout float3 norm, inout float2 tc) ;

void initEyeRay(uint x, uint y, uint a_sample_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;



float4 ResolveTransparency(uint tidX, float4 color) {
  float4 blended_color = to_float4(to_float3(m_transparencyBuffer[tidX]) + m_transparencyBuffer[tidX].w * to_float3(color), color.w);
  return blended_color;
}

float4 ResolveCommon(uint tidX, uint a_render_mode) {
  CRT_Hit hit = m_gBuffer[tidX];

  if (hit.primId == 0xFFFFFFFF) //no hit
    return float4(ubo[0].m_backgroundColor.x,ubo[0].m_backgroundColor.y,ubo[0].m_backgroundColor.z,ubo[0].m_AAFrameNum == 0 ? ubo[0].m_backgroundColor.w : -1.0f);

  uint type = hit.geomId >> SH_TYPE;
  uint geomId = hit.geomId & GEOM_ID_MASK;
  float4 rayPos, rayDir;
  initEyeRay(tidX % ubo[0].m_packedXY_width, tidX / ubo[0].m_packedXY_width, 0, rayPos, rayDir);

  float4 res_color = float4(1,0,1,1); //if pixel is purple at the end, then something gone wrong!
  float2 tc = float2(0,0);
  float3 norm = float3(1,0,0);
  loadNormalTcFromGbuffer(hit, to_float3(rayDir), norm, tc);

  switch (a_render_mode)
  {
  case MULTI_RENDER_MODE_DIFFUSE:
  {
    float4 color = float4(0,0,1,1);
    if (type == TYPE_SDF_SBS_COL)
    {
      color.x = round(hit.coords[0])/255.0f;
      color.y = fract(hit.coords[0]);
      color.z = round(hit.coords[1])/255.0f;
    }
    else
    {
      uint matId = m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];
      color = m_materials[matId].type == MULTI_RENDER_MATERIAL_TYPE_COLORED ? m_materials[matId].base_color : m_textures[NonUniformResourceIndex(m_materials[matId].texId)].SampleLevel(m_textures_sam[NonUniformResourceIndex(m_materials[matId].texId)], tc, 0);
    }
    res_color = to_float4(to_float3(color), 1);
  }
  break;

  case MULTI_RENDER_MODE_LAMBERT:
  case MULTI_RENDER_MODE_LAMBERT_NO_TEX:
  case MULTI_RENDER_MODE_GEOM:
  case MULTI_RENDER_MODE_LOD:
  {
    float3 color = float3(0,0,1);
    if (a_render_mode == MULTI_RENDER_MODE_LAMBERT_NO_TEX)
      color = float3(1,1,1);
    else if (a_render_mode == MULTI_RENDER_MODE_LOD)
      color = to_float3(decode_RGBA8(m_lod_palette[hit.primId % palette_size]));
    else if (a_render_mode == MULTI_RENDER_MODE_GEOM)
      color = to_float3(decode_RGBA8(m_palette[geomId % palette_size]));
    else if (type == TYPE_SDF_SBS_COL || type == TYPE_GRAPHICS_PRIM)
    {
      color.x = round(hit.coords[0])/255.0f;
      color.y = fract(hit.coords[0]);
      color.z = round(hit.coords[1])/255.0f;
    }
    else
    {
      uint matId = m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];
      //printf("%d %d %d %d -- %d\n", geomId, hit.primId, m_matIdOffsets[geomId].x, m_matIdOffsets[geomId].y, matId);
      color = to_float3(m_materials[matId].type == MULTI_RENDER_MATERIAL_TYPE_COLORED ? 
                        m_materials[matId].base_color : m_textures[NonUniformResourceIndex(m_materials[matId].texId)].SampleLevel(m_textures_sam[NonUniformResourceIndex(m_materials[matId].texId)], tc, 0));
    }

    float3 final_color = float3(0,0,0);
    for (int i=0;i<ubo[0].m_lights_size;i++)
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
        final_color += m_lights[i].color * color * q / (l*l);
      }
      else
        final_color += m_lights[i].color * color;
    }

    res_color = to_float4(final_color, 1);
  }
  break;

  case MULTI_RENDER_MODE_PHONG:
  case MULTI_RENDER_MODE_PHONG_NO_TEX:
  {
    const float Kd = 0.25f;
    const float Ks = 0.25f;
    const int spec_pow = 32;
    const float BIAS = 1e-6f;

    float3 color = float3(0,0,1);
    if (a_render_mode == MULTI_RENDER_MODE_PHONG_NO_TEX)
      color = float3(1,1,1);
    else if (type == TYPE_SDF_SBS_COL || type == TYPE_GRAPHICS_PRIM)
    {
      color.x = round(hit.coords[0])/255.0f;
      color.y = fract(hit.coords[0]);
      color.z = round(hit.coords[1])/255.0f;
    }
    else
    {
      uint matId = m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];
      color = to_float3(m_materials[matId].type == MULTI_RENDER_MATERIAL_TYPE_COLORED ? 
                        m_materials[matId].base_color : m_textures[NonUniformResourceIndex(m_materials[matId].texId)].SampleLevel(m_textures_sam[NonUniformResourceIndex(m_materials[matId].texId)], tc, 0));
    }

    float3 final_color = float3(0,0,0);
    for (int i=0;i<ubo[0].m_lights_size;i++)
    {
      if (m_lights[i].type == LIGHT_TYPE_AMBIENT)
      {
        final_color += m_lights[i].color * color;
      }
      else
      {
        float3 surf_pos = to_float3(rayPos) + (hit.t - BIAS) * to_float3(rayDir);
        float3 light_dir = float3(1,1,1);
        float light_dist = 1.0f;
        if (m_lights[i].type == LIGHT_TYPE_DIRECT)
        {
          light_dir = m_lights[i].space;
          light_dist = 1.0f;
          float q = max(0.0f, dot(norm, m_lights[i].space));
          final_color += m_lights[i].color * color * q;
        }
        else if (m_lights[i].type == LIGHT_TYPE_POINT)
        {
          float3 dir = m_lights[i].space - surf_pos;
          float l = length(dir);
          
          light_dir  = dir / l;
          light_dist = l;
        }

        //CRT_Hit shadowHit = m_pAccelStruct->RayQuery_NearestHit(to_float4(surf_pos, rayPos.w), to_float4(light_dir, rayDir.w));
        //float shade = (shadowHit.primId == 0xFFFFFFFF) ? 1 : 0;
        float shade = 1;
        float3 view_dir = to_float3(rayDir);
        float3 reflect = -1.0f*light_dir + 2.0f * dot(norm, light_dir) * norm;
        float3 f_col = (shade * m_lights[i].color * (Kd * max(0.0f, dot(norm, light_dir)) + Ks * pow(max(0.0f, dot(norm, reflect)), spec_pow))) * color;
        final_color += f_col;
      }
    }

    res_color = to_float4(final_color, 1);    
  }
  break;

  case MULTI_RENDER_MODE_PBR:
  case MULTI_RENDER_MODE_PBR_NO_TEX:
  {
    // default material always exists
    uint matId = a_render_mode == MULTI_RENDER_MODE_PBR_NO_TEX ? DEFAULT_MATERIAL :
                     m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];

    float3 baseColor = to_float3(m_materials[matId].type == MULTI_RENDER_MATERIAL_TYPE_COLORED ? 
                        m_materials[matId].base_color : m_textures[NonUniformResourceIndex(m_materials[matId].texId)].SampleLevel(m_textures_sam[NonUniformResourceIndex(m_materials[matId].texId)], tc, 0));
    float metallic = m_materials[matId].metallic;
    float roughness = m_materials[matId].roughness;

    float alpha = roughness * roughness;
    float f0 = 0.04f;

    float3 surf_pos = to_float3(rayPos) + hit.t * to_float3(rayDir);
    float3 eye_dir = -1 * to_float3(rayDir);
    float NV = dot(norm, eye_dir);

    float3 final_color = float3(0,0,0);
    for (int i = 0; i < ubo[0].m_lights_size; i++)
    {
      float3 light_dir = float3(1,1,1);
      if (m_lights[i].type == LIGHT_TYPE_DIRECT)
      {
        light_dir = m_lights[i].space;
      }
      else if (m_lights[i].type == LIGHT_TYPE_POINT)
      {
        light_dir = m_lights[i].space - surf_pos;
        float l = length(light_dir);
        light_dir = light_dir / l;
      }
      float3 half_vector = eye_dir + light_dir;
      float h = length(half_vector);
      half_vector = half_vector / h;

      float NH = dot(norm, half_vector);
      float NL = dot(norm, light_dir);
      float HL = dot(half_vector, light_dir);
      float HV = dot(half_vector, eye_dir);

      float D = alpha * max(0.0f, NH) / max(M_PI * float(pow(NH * NH * (alpha - 1.0f) + 1.0f, 2.0f)), 0.00001f);
      float V = max(0.0f, HL) / max(abs(NL) + sqrt(alpha + (1 - alpha) * NL * NL), 0.00001f);
      V *= max(0.0f, HV) / max(abs(NV) + sqrt(alpha + (1 - alpha) * NV * NV), 0.00001f);

      float specular_brdf = V * D;
      float3 diffuse_brdf = 1.0f / M_PI * baseColor;

      float3 metal_brdf = specular_brdf * (baseColor + (1.0f - baseColor) * pow(1.0f - abs(HV), 5.0f));
      float F = f0 + (1.0f - f0) * pow(1.0f - abs(HV), 5.0f);
      float3 dielectric_brdf = (1.0f - F) * diffuse_brdf + F * specular_brdf;

      float3 brdf = (1.0f - metallic) * dielectric_brdf + metallic * metal_brdf;

      final_color += brdf * max(NL, 0.0f);
    }

    res_color = to_float4(final_color, 1);    
  }
  break;
  default:
  break;
  }

  res_color += applyHighlight(hit);

  // if we use AA, we should put depth into alpha channel
  float alpha_depth = ubo[0].m_AAFrameNum == 0 ? 1.0f : hit.t;
  res_color.w = alpha_depth;

  return res_color;
}

float4 applyHighlight(CRT_Hit hit) {
  bool highlighted = false;
  if (ubo[0].m_highlight.mode == HIGHLIGHT_MODE_PRIMITIVE)
  {
    if (ubo[0].m_highlight.prim_id == hit.primId &&
        ubo[0].m_highlight.geom_id == hit.geomId &&
        ubo[0].m_highlight.inst_id == hit.instId)
    {
      highlighted = true;
    }
  }

  float4 color = highlighted ? float4(0.5,0,0,0) : float4(0,0,0,0);
  return color;
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
    {
      out_color[tidX] = ResolveCommon(tidX, kgenArgs.render_mode);
      out_color[tidX] = ResolveTransparency(tidX, out_color[tidX]);
    }
    else
    {
      float4 color = ResolveCommon(tidX, kgenArgs.render_mode);
      out_color[tidX] += ResolveTransparency(tidX, color);
    }
  
  }
}

