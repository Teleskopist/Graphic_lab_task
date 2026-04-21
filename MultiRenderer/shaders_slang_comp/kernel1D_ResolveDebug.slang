//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_comp_slang.h"

RWStructuredBuffer<float4> out_color; // 
RWStructuredBuffer<CRT_Hit> m_gBuffer; // 
RWStructuredBuffer<uint2> m_geomOffsets; // 
RWStructuredBuffer<uint> m_indices; // 
RWStructuredBuffer<float4x4> m_instanceTransformInvTransposed; // 
RWStructuredBuffer<uint2> m_matIdOffsets; // 
RWStructuredBuffer<uint> m_matIdbyPrimId; // 
RWStructuredBuffer<float4> m_normals; // 
RWStructuredBuffer<float4> m_vertices; // 
RWStructuredBuffer<MultiRenderer_slang_comp_UBO_Data> ubo;

float4 applyHighlight(CRT_Hit hit) ;

void loadNormalTcFromGbuffer(CRT_Hit hit, float3 ray_dir, inout float3 norm, inout float2 tc) ;

void initEyeRay(uint x, uint y, uint a_sample_id, inout float4 rayPosAndNear, inout float4 rayDirAndFar) ;



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
  
    CRT_Hit hit = m_gBuffer[tidX];
    float4 res_color = float4(1,0,1,1); //if pixel is purple at the end, then something gone wrong!
  
    if (hit.primId == 0xFFFFFFFF) //no hit
      res_color = float4(ubo[0].m_backgroundColor.x,ubo[0].m_backgroundColor.y,ubo[0].m_backgroundColor.z,ubo[0].m_AAFrameNum == 0 ? ubo[0].m_backgroundColor.w : -1.0f);
    else
    {
      uint type = hit.geomId >> SH_TYPE;
      uint geomId = hit.geomId & GEOM_ID_MASK;
      float4 rayPos, rayDir;
      initEyeRay(tidX % ubo[0].m_packedXY_width, tidX / ubo[0].m_packedXY_width, 0, rayPos, rayDir);
    
      float z = hit.t;
      float z_near = 0.1f;
      float z_far = 10.0f;
     
      float2 tc = float2(0,0);
      float3 norm = float3(1,0,0);
      loadNormalTcFromGbuffer(hit, to_float3(rayDir), norm, tc);
    
      switch (kgenArgs.render_mode)
      {
        case MULTI_RENDER_MODE_MASK:
          res_color = float4(1.0f,1.0f,1.0f,1.0f);
          break;
      
        case MULTI_RENDER_MODE_DEPTH:
        {
          float d = (1.0f / z - 1.0f / z_near) / (1.0f / z_far - 1.0f / z_near);
          res_color = float4(d,d,d,1);
        }
        break;
      
        case MULTI_RENDER_MODE_LINEAR_DEPTH:
        {
          float d = ((z - z_near) / (z_far - z_near));
          res_color = float4(d,d,d,1.0f);
        }
        break;
      
        case MULTI_RENDER_MODE_HSV_DEPTH:
        {
          float3 isect_pt = to_float3(rayPos + z * rayDir);
          float z_transformed = (isect_pt.z + 1.f) * 2.f;
          z_transformed = z_transformed - int(z_transformed);
          if (z_transformed < 0.f)
            z_transformed = 1.f + z_transformed;
          float3 hsv_col = float3(z_transformed,1.f,1.f);
      
          // HSV -> RGB
          res_color = float4(hsv_col.z,hsv_col.z,hsv_col.z,1.f);
          float Vmin = (1.f - hsv_col.y) * hsv_col.z;
          float H_i = 0.f;
          float a = (hsv_col.z - Vmin) * modf(hsv_col.x * 6.f, H_i);
          uint H_i_int = uint32_t(H_i);
          H_i_int = H_i_int <= 5u ? H_i_int : 5u;
      
          res_color[(2u + (H_i_int >> 1)) % 3u] = Vmin;
          if ((H_i_int & 1u) != 0u)
            res_color[H_i_int >> 1] = hsv_col.z - a;
          else
            res_color[(1u + (H_i_int >> 1)) % 3u] = Vmin + a;
          res_color.w = 1.f;
        }
        break;
      
        case MULTI_RENDER_MODE_INVERSE_LINEAR_DEPTH:
        {
          float d = 1 - ((z - z_near) / (z_far - z_near));
          res_color = float4(d,d,d,1);
        }
        break;
      
        case MULTI_RENDER_MODE_PRIMITIVE:
        {
          res_color = decode_RGBA8(m_palette[(hit.primId) % palette_size]);
        }
        break;
      
        case MULTI_RENDER_MODE_TYPE:
        {
          res_color = decode_RGBA8(m_palette[type % palette_size]);
        }
        break;
      
        case MULTI_RENDER_MODE_MATERIAL:
        {
          uint matId = m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];
          res_color = decode_RGBA8(m_palette[matId % palette_size]);
        }
        break;
      
        case MULTI_RENDER_MODE_NORMAL:
        {
          float3 v = abs(norm);
          res_color = float4(0.25f,0.25f,0.25f,0.5f) + 0.5f*float4(pow(v.x, 1.0f/1.5f),pow(v.y, 1.0f/1.5f),pow(v.z, 1.0f/1.5f),1.0f);
        }
        break;
      
        case MULTI_RENDER_MODE_BARYCENTRIC:
        {
          res_color = float4(hit.coords[0],hit.coords[1],1 - hit.coords[0] - hit.coords[1],1);
        }
        break;
      
        case MULTI_RENDER_MODE_ST_ITERATIONS:
        {
          if (hit.primId == 0)
          {
            res_color = float4(0.5f,0.5f,0.5f,1.0f);
          }
          else if (hit.primId <= 64)
          {
            float q = clamp(float(hit.primId) / 64, 0.0f, 1.0f);
            res_color = q * float4(0,0,1,1) + (1.0f - q) * float4(0,1,0,1);
          }
          else
          {
            float q = clamp(float(hit.primId - 64) / (256 - 64), 0.0f, 1.0f);
            res_color = q * float4(1,0,0,1) + (1.0f - q) * float4(0,0,1,1);
          }
        }
        break;
      
        case MULTI_RENDER_MODE_TEX_COORDS:
        {
          res_color = float4(tc.x,tc.y,0,1);
        }
        break;
      
        case MULTI_RENDER_MODE_MO_VOXEL_COUNT:
        {
          if (hit.primId == 0)
            res_color = float4(0.5,0.5,0.5,1);
          else if (hit.primId == 1)
            res_color = float4(0.5,1,0.5,1);
          else if (hit.primId == 2) 
            res_color = float4(0.5,0.5,1,1);
          else if (hit.primId <= 64)
          {
            float q = clamp(float(hit.primId) / 64, 0.0f, 1.0f);
            res_color = q * float4(1,0,0,1) + (1 - q) * float4(0.5,0.5,1,1);
          }
          else
          {
            res_color = float4(1,0,0,1);
          }
        }
        break;
        case MULTI_RENDER_MODE_MO_FLAGS:
        {
          res_color = decode_RGBA8(m_palette[hit.primId % palette_size]);
        }
        break;
        case MULTI_RENDER_MODE_INTERSECTION_COUNT:
        {
          if (hit.instId == 0)
            res_color = float4(0.3,0.3,0.3,1);
          else if (hit.instId == 1)
            res_color = float4(0,100,0,255)/255.0f;
          else if (hit.instId == 2)
            res_color = float4(0,200,0,255)/255.0f;
          else if (hit.instId == 3)
            res_color = float4(50,250,50,255)/255.0f;
          else if (hit.instId == 4)
            res_color = float4(150,250,150,255)/255.0f;
          else if (hit.instId == 5)
            res_color = float4(250,250,250,255)/255.0f;
          else if (hit.instId <= 20)
            res_color = float4(250,(1000-50*hit.instId)*0.333f,(1000-50*hit.instId)*0.333f,255)/255.0f;
          else 
            res_color = float4(1,0,min(1.0f, 0.025f*(hit.instId-20)),1);
          }
        break;
        default:
        break;

      } // end if switch
  
      res_color += applyHighlight(hit);

      // if we use AA, we should put depth into alpha channel
      float alpha_depth = ubo[0].m_AAFrameNum == 0 ? 1.0f : hit.t;
      res_color.w = alpha_depth;
  
    } // end of else


    if (kgenArgs.sample_id == 0)
      out_color[tidX] = res_color;
    else
      out_color[tidX] += res_color;

  
  }
}

