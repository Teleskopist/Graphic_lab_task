#include <cfloat>

#include "MultiRenderer.h"
#include "utils/raytracing/render_common.h"

using LiteMath::as_uint;
using LiteMath::abs;
using LiteMath::sqrt;

float4 MultiRenderer::applyHighlight(CRT_Hit hit)
{
  bool highlighted = false;
  if (m_highlight.mode == HIGHLIGHT_MODE_PRIMITIVE)
  {
    if (m_highlight.prim_id == hit.primId &&
        m_highlight.geom_id == hit.geomId &&
        m_highlight.inst_id == hit.instId)
    {
      highlighted = true;
    }
  }

  float4 color = highlighted ? float4(0.5,0,0,0) : float4(0,0,0,0);
  return color;
}

void MultiRenderer::loadNormalTcFromGbuffer(CRT_Hit hit, float3 ray_dir, float3 *norm, float2 *tc)
{
  unsigned type = hit.geomId >> SH_TYPE;
  unsigned geomId = hit.geomId & GEOM_ID_MASK;

  if (type == TYPE_MESH_TRIANGLE)
  {
#ifndef DISABLE_MESH
    const uint2 a_geomOffsets = m_geomOffsets[geomId];
    const uint32_t A = m_indices[a_geomOffsets.x + hit.primId * 3 + 0];
    const uint32_t B = m_indices[a_geomOffsets.x + hit.primId * 3 + 1];
    const uint32_t C = m_indices[a_geomOffsets.x + hit.primId * 3 + 2];

    const float2 A_tc = float2(m_vertices[a_geomOffsets.y + A].w, m_normals[a_geomOffsets.y + A].w);
    const float2 B_tc = float2(m_vertices[a_geomOffsets.y + B].w, m_normals[a_geomOffsets.y + B].w);
    const float2 C_tc = float2(m_vertices[a_geomOffsets.y + C].w, m_normals[a_geomOffsets.y + C].w);

    float3 raw_norm = float3(1, 0, 0);
    if (m_preset.normal_mode == NORMAL_MODE_GEOMETRY)
    {
      const float3 posA = to_float3(m_vertices[a_geomOffsets.y + A]);
      const float3 posB = to_float3(m_vertices[a_geomOffsets.y + B]);
      const float3 posC = to_float3(m_vertices[a_geomOffsets.y + C]);
      const float3 edge1 = posB - posA;
      const float3 edge2 = posC - posA;
      raw_norm = cross(edge1, edge2);
    }
    else if (m_preset.normal_mode == NORMAL_MODE_VERTEX)
    {
      const float4 normA = m_normals[a_geomOffsets.y + A];
      const float4 normB = m_normals[a_geomOffsets.y + B];
      const float4 normC = m_normals[a_geomOffsets.y + C];
      const float2 uv = float2(hit.coords[0], hit.coords[1]);
      const float4 norm4 = (1.0f - uv.x - uv.y) * normA + uv.y * normB + uv.x * normC;
      raw_norm = to_float3(norm4);
    }

    *tc = (1.0f - hit.coords[0] - hit.coords[1]) * A_tc + hit.coords[1] * B_tc + hit.coords[0] * C_tc;
    *norm = normalize(matmul4x3(m_instanceTransformInvTransposed[hit.instId], raw_norm));
    float norm_sign = sign(dot(-1.0f * ray_dir, *norm));
    *norm = (*norm) * norm_sign;
#endif
  }
  else
  {
    *tc = float2(hit.coords[0], hit.coords[1]);
    *norm = decode_normal(float2(hit.coords[2], hit.coords[3]));
  }
}

float MultiRenderer::loadFromFloatChannel(uint32_t offset)
{
  return m_allFloatChannels[offset];
}

float MultiRenderer::loadFromFP8Channel(uint32_t offset)
{
  return ((m_allCompressedChannels[offset/4] >> ((offset%4)*8)) & 0xFF) * (1.0f/255.0f);
}

float3 MultiRenderer::calculateChannelColor(CRT_Hit hit, uint32_t tidX)
{
#if !defined(DISABLE_AUGMENTED_GEOMETRY) && !defined(DISABLE_MESH)
  float3 norm;
  float2 tc;
  float4 rayPos, rayDir;
  initEyeRay(tidX % m_packedXY_width, tidX / m_packedXY_width, 0, &rayPos, &rayDir);
  loadNormalTcFromGbuffer(hit, to_float3(rayDir), &norm, &tc);
  uint32_t channel_id = m_preset.channel_id + m_allChannelOffsets[hit.geomId].w;
  int i_val = 0;
  float f_val = 0;
  if (m_allChannelRenderInfo[channel_id].data_type == CHANNEL_DATA_TYPE_INT)
  {
    if (m_allChannelRenderInfo[channel_id].type == CHANNEL_TYPE_FACE)
    {
      const uint32_t offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * m_allChannelRenderInfo[channel_id].components;
      i_val = m_allIntChannels[offset + m_preset.component_id];
    }
    //else
    //CHANNEL_TYPE_VERTEX should not be here
    f_val = (float)i_val;
  }
  else if (m_allChannelRenderInfo[channel_id].data_type == CHANNEL_DATA_TYPE_FLOAT)
  {
    if (m_allChannelRenderInfo[channel_id].type == CHANNEL_TYPE_FACE)
    {
      const uint32_t offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * m_allChannelRenderInfo[channel_id].components;
      f_val = loadFromFloatChannel(offset + m_preset.component_id);
    }
    else //if CHANNEL_TYPE_VERTEX
    {
      const uint2 a_geomOffsets = m_geomOffsets[hit.geomId];
      const uint32_t A = m_indices[a_geomOffsets.x + hit.primId * 3 + 0];
      const uint32_t B = m_indices[a_geomOffsets.x + hit.primId * 3 + 1];
      const uint32_t C = m_indices[a_geomOffsets.x + hit.primId * 3 + 2];

      const uint32_t off_A = m_allChannelRenderInfo[channel_id].offset + A * m_allChannelRenderInfo[channel_id].components;
      const uint32_t off_B = m_allChannelRenderInfo[channel_id].offset + B * m_allChannelRenderInfo[channel_id].components;
      const uint32_t off_C = m_allChannelRenderInfo[channel_id].offset + C * m_allChannelRenderInfo[channel_id].components;

      const float val_A = loadFromFloatChannel(off_A + m_preset.component_id);
      const float val_B = loadFromFloatChannel(off_B + m_preset.component_id);
      const float val_C = loadFromFloatChannel(off_C + m_preset.component_id);

      const float2 uv = float2(hit.coords[0], hit.coords[1]);
      f_val = (1.0f - uv.x - uv.y) * val_A + uv.y * val_B + uv.x * val_C;
      i_val = (int)f_val;
    }
  }
  else //if (m_allChannelRenderInfo[channel_id].data_type == CHANNEL_DATA_TYPE_FP8)
  {
    if (m_allChannelRenderInfo[channel_id].type == CHANNEL_TYPE_FACE)
    {
      const uint32_t b_offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * m_allChannelRenderInfo[channel_id].components;
      f_val = loadFromFP8Channel(b_offset + m_preset.component_id);
    }
    else //if CHANNEL_TYPE_VERTEX
    {
      const uint2 a_geomOffsets = m_geomOffsets[hit.geomId];
      const uint32_t A = m_indices[a_geomOffsets.x + hit.primId * 3 + 0];
      const uint32_t B = m_indices[a_geomOffsets.x + hit.primId * 3 + 1];
      const uint32_t C = m_indices[a_geomOffsets.x + hit.primId * 3 + 2];

      const uint32_t b_off_A = m_allChannelRenderInfo[channel_id].offset + A * m_allChannelRenderInfo[channel_id].components;
      const uint32_t b_off_B = m_allChannelRenderInfo[channel_id].offset + B * m_allChannelRenderInfo[channel_id].components;
      const uint32_t b_off_C = m_allChannelRenderInfo[channel_id].offset + C * m_allChannelRenderInfo[channel_id].components;

      const float val_A = loadFromFP8Channel(b_off_A + m_preset.component_id);
      const float val_B = loadFromFP8Channel(b_off_B + m_preset.component_id);
      const float val_C = loadFromFP8Channel(b_off_C + m_preset.component_id);

      const float2 uv = float2(hit.coords[0], hit.coords[1]);
      f_val = (1.0f - uv.x - uv.y) * val_A + uv.y * val_B + uv.x * val_C;
      i_val = (int)f_val;
    }
  }

  float3 color = float3(1,0,1);
  switch (m_preset.channel_render_mode)
  {
  case CHANNEL_RENDER_MODE_DIRECT:
    color = float3(f_val, 0, 0);
    break;
  case CHANNEL_RENDER_MODE_NORMALIZED:
  {
    float mn = m_allChannelRenderInfo[channel_id].min_val;
    float mx = m_allChannelRenderInfo[channel_id].max_val;
    color = float3((f_val - mn) / (mx - mn), 0, 0);
  }
    break;
  case CHANNEL_RENDER_MODE_PALETTE:
    color = to_float3(decode_RGBA8(m_palette[(i_val) % palette_size]));
    break;
  case CHANNEL_RENDER_MODE_CTF:
    if (m_preset.CTF_id < m_allCTFs.size())
    {
      float mn = m_allChannelRenderInfo[channel_id].min_val;
      float mx = m_allChannelRenderInfo[channel_id].max_val;
      float val = (f_val - mn) / (mx - mn);
      int count = m_allCTFs[m_preset.CTF_id].sample_points_count;
      int offset = m_allCTFs[m_preset.CTF_id].sample_points_offset;
      int end_point = 0;
      while (end_point < count && 
             m_allCTFPoints[offset + end_point].w < val)
        end_point++;
      int start_point = std::max(0, end_point-1);
      end_point = std::min(end_point, count-1);
      float v1 = m_allCTFPoints[offset + start_point].w;
      float v2 = m_allCTFPoints[offset +   end_point].w;
      float q = v1<v2 ? (val-v1)/(v2-v1) : 0;
      color = to_float3((1-q)*m_allCTFPoints[offset + start_point] + q*m_allCTFPoints[offset + end_point]);
    }
    break;
  default:
    break;
  }
  if (m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL)
    return color;
  float3 final_color = float3(0, 0, 0);
  for (int i = 0; i < m_lights.size(); i++)
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
  return final_color;
#else
  return float3(1,0,1);
#endif
}

float3 MultiRenderer::calculateChannelColorSDF(CRT_Hit hit, uint32_t tidX)
{
#ifndef DISABLE_AUGMENTED_GEOMETRY
  float3 norm = decode_normal(float2(hit.coords[2], hit.coords[3]));
  const uint32_t geomId = hit.geomId & GEOM_ID_MASK;
  const uint32_t channel_id = m_preset.channel_id + m_allChannelOffsets[geomId].w;
  const uint32_t num_comp = m_allChannelRenderInfo[channel_id].components;
  //printf("%d %d %d %d\n", channel_id, geomId, hit.primId, num_comp);
  int i_val = 0;
  float f_val = 0;
  if (m_allChannelRenderInfo[channel_id].data_type == CHANNEL_DATA_TYPE_INT)
  {
    if (m_allChannelRenderInfo[channel_id].type == CHANNEL_TYPE_FACE)
    {
      const uint32_t offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * num_comp;
      i_val = m_allIntChannels[offset + m_preset.component_id];
    }
    //else
    //CHANNEL_TYPE_VERTEX should not be here
    f_val = (float)i_val;
  }
  else if (m_allChannelRenderInfo[channel_id].data_type == CHANNEL_DATA_TYPE_FLOAT)
  {
    if (m_allChannelRenderInfo[channel_id].type == CHANNEL_TYPE_FACE)
    {
      const uint32_t offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * num_comp;
      f_val = loadFromFloatChannel(offset + m_preset.component_id);
    }
    else //if CHANNEL_TYPE_VERTEX
    {
      float values[8];
      const uint32_t base_offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * 8*num_comp;
      for (int i = 0; i < 8; i++)
        values[i] = loadFromFloatChannel(base_offset + i*num_comp + m_preset.component_id);

      uint32_t packed_xy = as_uint(hit.coords[0]);
      uint32_t packed_z  = as_uint(hit.coords[1]);
      float3 dp = float3(float(packed_xy & 0xFFFF) / float(0xFFFF), 
                         float(packed_xy >> 16) / float(0xFFFF), 
                         float(packed_z) / float(0xFFFF));

      f_val = (1-dp.x)*(1-dp.y)*(1-dp.z)*values[0] + 
              (1-dp.x)*(1-dp.y)*(  dp.z)*values[1] + 
              (1-dp.x)*(  dp.y)*(1-dp.z)*values[2] + 
              (1-dp.x)*(  dp.y)*(  dp.z)*values[3] + 
              (  dp.x)*(1-dp.y)*(1-dp.z)*values[4] + 
              (  dp.x)*(1-dp.y)*(  dp.z)*values[5] + 
              (  dp.x)*(  dp.y)*(1-dp.z)*values[6] + 
              (  dp.x)*(  dp.y)*(  dp.z)*values[7];
    }
    i_val = (int)f_val;
  }
  else if (m_allChannelRenderInfo[channel_id].data_type == CHANNEL_DATA_TYPE_FP8)
  {
    if (m_allChannelRenderInfo[channel_id].type == CHANNEL_TYPE_FACE)
    {
      const uint32_t offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * num_comp;
      f_val = loadFromFP8Channel(offset + m_preset.component_id);
    }
    else //if CHANNEL_TYPE_VERTEX
    {
      float values[8];
      const uint32_t base_offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * 8*num_comp;
      for (int i = 0; i < 8; i++)
        values[i] = loadFromFP8Channel(base_offset + i*num_comp + m_preset.component_id);

      uint32_t packed_xy = as_uint(hit.coords[0]);
      uint32_t packed_z  = as_uint(hit.coords[1]);
      float3 dp = float3(float(packed_xy & 0xFFFF) / float(0xFFFF), 
                         float(packed_xy >> 16) / float(0xFFFF), 
                         float(packed_z) / float(0xFFFF));

      f_val = (1-dp.x)*(1-dp.y)*(1-dp.z)*values[0] + 
              (1-dp.x)*(1-dp.y)*(  dp.z)*values[1] + 
              (1-dp.x)*(  dp.y)*(1-dp.z)*values[2] + 
              (1-dp.x)*(  dp.y)*(  dp.z)*values[3] + 
              (  dp.x)*(1-dp.y)*(1-dp.z)*values[4] + 
              (  dp.x)*(1-dp.y)*(  dp.z)*values[5] + 
              (  dp.x)*(  dp.y)*(1-dp.z)*values[6] + 
              (  dp.x)*(  dp.y)*(  dp.z)*values[7];
    }
    i_val = (int)f_val;
  }

  float3 color = float3(1,0,1);
  switch (m_preset.channel_render_mode)
  {
  case CHANNEL_RENDER_MODE_DIRECT:
    color = float3(f_val, 0, 0);
    break;
  case CHANNEL_RENDER_MODE_NORMALIZED:
  {
    float mn = m_allChannelRenderInfo[channel_id].min_val;
    float mx = m_allChannelRenderInfo[channel_id].max_val;
    color = float3((f_val - mn) / (mx - mn), 0, 0);
  }
    break;
  case CHANNEL_RENDER_MODE_PALETTE:
    color = to_float3(decode_RGBA8(m_palette[(i_val) % palette_size]));
    break;
  case CHANNEL_RENDER_MODE_CTF:
    if (m_preset.CTF_id < m_allCTFs.size())
    {
      float mn = m_allChannelRenderInfo[channel_id].min_val;
      float mx = m_allChannelRenderInfo[channel_id].max_val;
      float val = (f_val - mn) / (mx - mn);
      int count = m_allCTFs[m_preset.CTF_id].sample_points_count;
      int offset = m_allCTFs[m_preset.CTF_id].sample_points_offset;
      int end_point = 0;
      while (end_point < count && 
             m_allCTFPoints[offset + end_point].w < val)
        end_point++;
      int start_point = std::max(0, end_point-1);
      end_point = std::min(end_point, count-1);
      float v1 = m_allCTFPoints[offset + start_point].w;
      float v2 = m_allCTFPoints[offset +   end_point].w;
      float q = v1<v2 ? (val-v1)/(v2-v1) : 0;
      color = to_float3((1-q)*m_allCTFPoints[offset + start_point] + q*m_allCTFPoints[offset + end_point]);
    }
    break;
  default:
    break;
  }

  float4 rayPos, rayDir;
  initEyeRay(tidX % m_packedXY_width, tidX / m_packedXY_width, 0, &rayPos, &rayDir);

  if (m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL)
    return color;
  float3 final_color = float3(0, 0, 0);
  for (int i = 0; i < m_lights.size(); i++)
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
  return final_color;
#else
  return float3(1,0,1);
#endif
}

float3 MultiRenderer::calculateDirectChannelColorSDF(CRT_Hit hit, uint32_t tidX)
{
#ifndef DISABLE_AUGMENTED_GEOMETRY
  float3 norm = decode_normal(float2(hit.coords[2], hit.coords[3]));
  const uint32_t geomId = hit.geomId & GEOM_ID_MASK;
  const uint32_t channel_id = m_preset.channel_id + m_allChannelOffsets[geomId].w;
  const uint32_t num_comp = m_allChannelRenderInfo[channel_id].components;
  float3 color = float3(1,1,0);

  if (m_allChannelRenderInfo[channel_id].components < m_preset.component_id + 3)
    return float3(1,0,1);

  if (m_allChannelRenderInfo[channel_id].data_type == CHANNEL_DATA_TYPE_FLOAT)
  {
    if (m_allChannelRenderInfo[channel_id].type == CHANNEL_TYPE_FACE)
    {
      const uint32_t offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * num_comp;
      color.x = loadFromFloatChannel(offset + m_preset.component_id+0);
      color.y = loadFromFloatChannel(offset + m_preset.component_id+1);
      color.z = loadFromFloatChannel(offset + m_preset.component_id+2);
    }
    else //if CHANNEL_TYPE_VERTEX
    {
      float3 values[8];
      const uint32_t base_offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * 8*num_comp;
      for (int i = 0; i < 8; i++)
      {
        values[i].x = loadFromFloatChannel(base_offset + i*num_comp + m_preset.component_id+0);
        values[i].y = loadFromFloatChannel(base_offset + i*num_comp + m_preset.component_id+1);
        values[i].z = loadFromFloatChannel(base_offset + i*num_comp + m_preset.component_id+2);
      }

      uint32_t packed_xy = as_uint(hit.coords[0]);
      uint32_t packed_z  = as_uint(hit.coords[1]);
      float3 dp = float3(float(packed_xy & 0xFFFF) / float(0xFFFF), 
                         float(packed_xy >> 16) / float(0xFFFF), 
                         float(packed_z) / float(0xFFFF));

      color = (1-dp.x)*(1-dp.y)*(1-dp.z)*values[0] + 
              (1-dp.x)*(1-dp.y)*(  dp.z)*values[1] + 
              (1-dp.x)*(  dp.y)*(1-dp.z)*values[2] + 
              (1-dp.x)*(  dp.y)*(  dp.z)*values[3] + 
              (  dp.x)*(1-dp.y)*(1-dp.z)*values[4] + 
              (  dp.x)*(1-dp.y)*(  dp.z)*values[5] + 
              (  dp.x)*(  dp.y)*(1-dp.z)*values[6] + 
              (  dp.x)*(  dp.y)*(  dp.z)*values[7];
    }
  }
  else if (m_allChannelRenderInfo[channel_id].data_type == CHANNEL_DATA_TYPE_FP8)
  {
    if (m_allChannelRenderInfo[channel_id].type == CHANNEL_TYPE_FACE)
    {
      const uint32_t offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * num_comp;
      color.x = loadFromFP8Channel(offset + m_preset.component_id+0);
      color.y = loadFromFP8Channel(offset + m_preset.component_id+1);
      color.z = loadFromFP8Channel(offset + m_preset.component_id+2);
    }
    else //if CHANNEL_TYPE_VERTEX
    {
      float3 values[8];
      const uint32_t base_offset = m_allChannelRenderInfo[channel_id].offset + hit.primId * 8*num_comp;
      for (int i = 0; i < 8; i++)
      {
        values[i].x = loadFromFP8Channel(base_offset + i*num_comp + m_preset.component_id+0);
        values[i].y = loadFromFP8Channel(base_offset + i*num_comp + m_preset.component_id+1);
        values[i].z = loadFromFP8Channel(base_offset + i*num_comp + m_preset.component_id+2);
      }

      uint32_t packed_xy = as_uint(hit.coords[0]);
      uint32_t packed_z  = as_uint(hit.coords[1]);
      float3 dp = float3(float(packed_xy & 0xFFFF) / float(0xFFFF), 
                         float(packed_xy >> 16) / float(0xFFFF), 
                         float(packed_z) / float(0xFFFF));

      color = (1-dp.x)*(1-dp.y)*(1-dp.z)*values[0] + 
              (1-dp.x)*(1-dp.y)*(  dp.z)*values[1] + 
              (1-dp.x)*(  dp.y)*(1-dp.z)*values[2] + 
              (1-dp.x)*(  dp.y)*(  dp.z)*values[3] + 
              (  dp.x)*(1-dp.y)*(1-dp.z)*values[4] + 
              (  dp.x)*(1-dp.y)*(  dp.z)*values[5] + 
              (  dp.x)*(  dp.y)*(1-dp.z)*values[6] + 
              (  dp.x)*(  dp.y)*(  dp.z)*values[7];
    }
  }

  switch (m_preset.channel_render_mode)
  {
  case CHANNEL_RENDER_MODE_AS_RGB_COLOR:
    color = clamp(color, float3(0,0,0), float3(1,1,1));
    break;
  case CHANNEL_RENDER_MODE_AS_NORMAL:
  {
    float3 v = abs(normalize(color));
    color = float3(0.25f, 0.25f, 0.25f) + float3(pow(v.x, 1.0f/1.5f), pow(v.y, 1.0f/1.5f), pow(v.z, 1.0f/1.5f));
  }
    break;
  default:
    break;
  }

  float4 rayPos, rayDir;
  initEyeRay(tidX % m_packedXY_width, tidX / m_packedXY_width, 0, &rayPos, &rayDir);

  if (m_preset.render_mode == MULTI_RENDER_MODE_CHANNEL)
    return color;
  float3 final_color = float3(0, 0, 0);
  for (int i = 0; i < m_lights.size(); i++)
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
  return final_color;
#else
  return float3(1,0,1);
#endif
}

void MultiRenderer::kernel1D_AverageColor(uint32_t count, uint32_t sample_count, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
    out_color[tidX] = AverageColor(tidX, sample_count);
}

float4 MultiRenderer::AverageColor(uint32_t tidX, uint32_t a_sample_count)
{
  return m_colorBuffer[tidX] / float(a_sample_count);
}

void MultiRenderer::kernel1D_Tonemap(uint32_t count, uint32_t sample_count, uint32_t* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
    out_color[tidX] = Tonemap(tidX, sample_count);
}

uint32_t MultiRenderer::Tonemap(uint32_t tidX, uint32_t a_sample_count)
{
  float4 color_f = m_colorBuffer[tidX] / float(a_sample_count);
  uint4 col = uint4(255 * clamp(color_f, float4(0,0,0,0), float4(1,1,1,1)));
  uint32_t color_rgba8 = (col.w<<24) | (col.z<<16) | (col.y<<8) | col.x;
  return color_rgba8;
}

void MultiRenderer::kernel1D_ResolveCommon(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    if (sample_id == 0)
      out_color[tidX] = ResolveCommon(tidX, render_mode);
    else
      out_color[tidX] += ResolveCommon(tidX, render_mode);
  }
}

void MultiRenderer::kernel1D_ResolveCommonWithTransparency(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    if (sample_id == 0)
    {
      out_color[tidX] = ResolveCommon(tidX, render_mode);
      out_color[tidX] = ResolveTransparency(tidX, out_color[tidX]);
    }
    else
    {
      float4 color = ResolveCommon(tidX, render_mode);
      out_color[tidX] += ResolveTransparency(tidX, color);
    }
  }
}

float4 MultiRenderer::ResolveTransparency(uint32_t tidX, float4 color)
{
  float4 blended_color = to_float4(to_float3(m_transparencyBuffer[tidX]) + m_transparencyBuffer[tidX].w * to_float3(color), color.w);
  return blended_color;
}

float4 MultiRenderer::ResolveCommon(uint32_t tidX, uint32_t a_render_mode)
{
  CRT_Hit hit = m_gBuffer[tidX];

  if (hit.primId == 0xFFFFFFFF) //no hit
    return float4(m_backgroundColor.x, m_backgroundColor.y, m_backgroundColor.z, m_AAFrameNum == 0 ? m_backgroundColor.w : -1.0f);

  unsigned type = hit.geomId >> SH_TYPE;
  unsigned geomId = hit.geomId & GEOM_ID_MASK;
  float4 rayPos, rayDir;
  initEyeRay(tidX % m_packedXY_width, tidX / m_packedXY_width, 0, &rayPos, &rayDir);

  float4 res_color = float4(1,0,1,1); //if pixel is purple at the end, then something gone wrong!
  float2 tc = float2(0, 0);
  float3 norm = float3(1,0,0);
  loadNormalTcFromGbuffer(hit, to_float3(rayDir), &norm, &tc);

  switch (a_render_mode)
  {
  case MULTI_RENDER_MODE_DIFFUSE:
  {
    float4 color = float4(0,0,1,1);
    if (type == TYPE_SDF_SBS_COL)
    {
      color.x = std::round(hit.coords[0])/255.0f;
      color.y = fract(hit.coords[0]);
      color.z = std::round(hit.coords[1])/255.0f;
    }
    else
    {
      unsigned matId = m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];
      color = m_materials[matId].type == MULTI_RENDER_MATERIAL_TYPE_COLORED ? m_materials[matId].base_color : m_textures[m_materials[matId].texId]->sample(tc);
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
      color.x = std::round(hit.coords[0])/255.0f;
      color.y = fract(hit.coords[0]);
      color.z = std::round(hit.coords[1])/255.0f;
    }
    else
    {
      unsigned matId = m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];
      //printf("%d %d %d %d -- %d\n", geomId, hit.primId, m_matIdOffsets[geomId].x, m_matIdOffsets[geomId].y, matId);
      color = to_float3(m_materials[matId].type == MULTI_RENDER_MATERIAL_TYPE_COLORED ? 
                        m_materials[matId].base_color : m_textures[m_materials[matId].texId]->sample(tc));
    }

    float3 final_color = float3(0,0,0);
    for (int i=0;i<m_lights.size();i++)
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
      color.x = std::round(hit.coords[0])/255.0f;
      color.y = fract(hit.coords[0]);
      color.z = std::round(hit.coords[1])/255.0f;
    }
    else
    {
      unsigned matId = m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];
      color = to_float3(m_materials[matId].type == MULTI_RENDER_MATERIAL_TYPE_COLORED ? 
                        m_materials[matId].base_color : m_textures[m_materials[matId].texId]->sample(tc));
    }

    float3 final_color = float3(0,0,0);
    for (int i=0;i<m_lights.size();i++)
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
        float3 f_col = (shade * m_lights[i].color * (Kd * std::max(0.0f, dot(norm, light_dir)) + Ks * pow(std::max(0.0f, dot(norm, reflect)), spec_pow))) * color;
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
    unsigned matId = a_render_mode == MULTI_RENDER_MODE_PBR_NO_TEX ? DEFAULT_MATERIAL :
                     m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];

    float3 baseColor = to_float3(m_materials[matId].type == MULTI_RENDER_MATERIAL_TYPE_COLORED ? 
                        m_materials[matId].base_color : m_textures[m_materials[matId].texId]->sample(tc));
    float metallic = m_materials[matId].metallic;
    float roughness = m_materials[matId].roughness;

    float alpha = roughness * roughness;
    float f0 = 0.04f;

    float3 surf_pos = to_float3(rayPos) + hit.t * to_float3(rayDir);
    float3 eye_dir = -1 * to_float3(rayDir);
    float NV = dot(norm, eye_dir);

    float3 final_color = float3(0,0,0);
    for (int i = 0; i < m_lights.size(); i++)
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
  float alpha_depth = m_AAFrameNum == 0 ? 1.0f : hit.t;
  res_color.w = alpha_depth;

  return res_color;
}

void MultiRenderer::kernel1D_ResolveDebug(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    CRT_Hit hit = m_gBuffer[tidX];
    float4 res_color = float4(1,0,1,1); //if pixel is purple at the end, then something gone wrong!
  
    if (hit.primId == 0xFFFFFFFF) //no hit
      res_color = float4(m_backgroundColor.x, m_backgroundColor.y, m_backgroundColor.z, m_AAFrameNum == 0 ? m_backgroundColor.w : -1.0f);
    else
    {
      unsigned type = hit.geomId >> SH_TYPE;
      unsigned geomId = hit.geomId & GEOM_ID_MASK;
      float4 rayPos, rayDir;
      initEyeRay(tidX % m_packedXY_width, tidX / m_packedXY_width, 0, &rayPos, &rayDir);
    
      float z = hit.t;
      float z_near = 0.1f;
      float z_far = 10.0f;
     
      float2 tc = float2(0, 0);
      float3 norm = float3(1,0,0);
      loadNormalTcFromGbuffer(hit, to_float3(rayDir), &norm, &tc);
    
      switch (render_mode)
      {
        case MULTI_RENDER_MODE_MASK:
          res_color = float4(1.0f, 1.0f, 1.0f, 1.0f);
          break;
      
        case MULTI_RENDER_MODE_DEPTH:
        {
          float d = (1.0f / z - 1.0f / z_near) / (1.0f / z_far - 1.0f / z_near);
          res_color = float4(d, d, d, 1);
        }
        break;
      
        case MULTI_RENDER_MODE_LINEAR_DEPTH:
        {
          float d = ((z - z_near) / (z_far - z_near));
          res_color = float4(d, d, d, 1.0f);
        }
        break;
      
        case MULTI_RENDER_MODE_HSV_DEPTH:
        {
          float3 isect_pt = to_float3(rayPos + z * rayDir);
          float z_transformed = (isect_pt.z + 1.f) * 2.f;
          z_transformed = z_transformed - int(z_transformed);
          if (z_transformed < 0.f)
            z_transformed = 1.f + z_transformed;
          float3 hsv_col = float3(z_transformed, 1.f, 1.f);
      
          // HSV -> RGB
          res_color = float4(hsv_col.z, hsv_col.z, hsv_col.z, 1.f);
          float Vmin = (1.f - hsv_col.y) * hsv_col.z;
          float H_i = 0.f;
          float a = (hsv_col.z - Vmin) * std::modf(hsv_col.x * 6.f, &H_i);
          uint32_t H_i_int = uint32_t(H_i);
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
          res_color = float4(d, d, d, 1);
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
          unsigned matId = m_matIdbyPrimId[m_matIdOffsets[geomId].x + hit.primId % m_matIdOffsets[geomId].y];
          res_color = decode_RGBA8(m_palette[matId % palette_size]);
        }
        break;
      
        case MULTI_RENDER_MODE_NORMAL:
        {
          float3 v = abs(norm);
          res_color = float4(0.25f, 0.25f, 0.25f, 0.5f) + 0.5f*float4(pow(v.x, 1.0f/1.5f), pow(v.y, 1.0f/1.5f), pow(v.z, 1.0f/1.5f), 1.0f);
        }
        break;
      
        case MULTI_RENDER_MODE_BARYCENTRIC:
        {
          res_color = float4(hit.coords[0], hit.coords[1], 1 - hit.coords[0] - hit.coords[1], 1);
        }
        break;
      
        case MULTI_RENDER_MODE_ST_ITERATIONS:
        {
          if (hit.primId == 0)
          {
            res_color = float4(0.5f, 0.5f, 0.5f, 1.0f);
          }
          else if (hit.primId <= 64)
          {
            float q = clamp(float(hit.primId) / 64, 0.0f, 1.0f);
            res_color = q * float4(0, 0, 1, 1) + (1.0f - q) * float4(0, 1, 0, 1);
          }
          else
          {
            float q = clamp(float(hit.primId - 64) / (256 - 64), 0.0f, 1.0f);
            res_color = q * float4(1, 0, 0, 1) + (1.0f - q) * float4(0, 0, 1, 1);
          }
        }
        break;
      
        case MULTI_RENDER_MODE_TEX_COORDS:
        {
          res_color = float4(tc.x, tc.y, 0, 1);
        }
        break;
      
        case MULTI_RENDER_MODE_MO_VOXEL_COUNT:
        {
          if (hit.primId == 0)
            res_color = float4(0.5, 0.5, 0.5, 1);
          else if (hit.primId == 1)
            res_color = float4(0.5, 1, 0.5, 1);
          else if (hit.primId == 2) 
            res_color = float4(0.5, 0.5, 1, 1);
          else if (hit.primId <= 64)
          {
            float q = clamp(float(hit.primId) / 64, 0.0f, 1.0f);
            res_color = q * float4(1, 0, 0, 1) + (1 - q) * float4(0.5, 0.5, 1, 1);
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
            res_color = float4(0, 100, 0, 255)/255.0f;
          else if (hit.instId == 2)
            res_color = float4(0, 200, 0, 255)/255.0f;
          else if (hit.instId == 3)
            res_color = float4(50, 250, 50, 255)/255.0f;
          else if (hit.instId == 4)
            res_color = float4(150, 250, 150, 255)/255.0f;
          else if (hit.instId == 5)
            res_color = float4(250, 250, 250, 255)/255.0f;
          else if (hit.instId <= 20)
            res_color = float4(250,(1000-50*hit.instId)*0.333f, (1000-50*hit.instId)*0.333f, 255)/255.0f;
          else 
            res_color = float4(1, 0, std::min(1.0f, 0.025f*(hit.instId-20)), 1);
          }
        break;
        default:
        break;

      } // end if switch
  
      res_color += applyHighlight(hit);

      // if we use AA, we should put depth into alpha channel
      float alpha_depth = m_AAFrameNum == 0 ? 1.0f : hit.t;
      res_color.w = alpha_depth;
  
    } // end of else


    if (sample_id == 0)
      out_color[tidX] = res_color;
    else
      out_color[tidX] += res_color;

  } // end of for
} // end of kernel function


void MultiRenderer::kernel1D_ResolveCTF(uint32_t count, uint32_t sample_id, uint32_t render_mode, LiteMath::float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    if (sample_id == 0)
      out_color[tidX] = ResolveCTF(tidX, render_mode);
    else
      out_color[tidX] += ResolveCTF(tidX, render_mode);
  }
}

float4 MultiRenderer::ResolveCTF(uint32_t tidX, uint32_t a_render_mode)
{
  CRT_Hit hit = m_gBuffer[tidX];

  if (hit.primId == 0xFFFFFFFF) //no hit
    return float4(m_backgroundColor.x, m_backgroundColor.y, m_backgroundColor.z, m_AAFrameNum == 0 ? m_backgroundColor.w : -1.0f);

  unsigned type = hit.geomId >> SH_TYPE;
  unsigned geomId = hit.geomId & GEOM_ID_MASK;
  float4 res_color = float4(1,0,1,1);

  if (m_allChannelOffsets.size() <= geomId || m_allChannelOffsets[geomId].x == HAS_NO_CHANNELS)
    res_color = ResolveCommon(tidX, MULTI_RENDER_MODE_LAMBERT_NO_TEX); //if no channels, fallback to a safe default mode
  else if (type == TYPE_MESH_TRIANGLE)
    res_color = to_float4(calculateChannelColor(hit, tidX), 1);
  else if (type == TYPE_SDF_MULTI_OCTREE || type == TYPE_SDF_DAG || type == TYPE_SCOM2)
  {
    if (m_preset.channel_render_mode == CHANNEL_RENDER_MODE_AS_RGB_COLOR ||
        m_preset.channel_render_mode == CHANNEL_RENDER_MODE_AS_NORMAL)
    {
      res_color = to_float4(calculateDirectChannelColorSDF(hit, tidX), 1);
    }
    else
    {
      res_color = to_float4(calculateChannelColorSDF(hit, tidX), 1);  
    }
  }
  else
    res_color = float4(1,0,0,1);
  
  res_color += applyHighlight(hit);

  // if we use AA, we should put depth into alpha channel
  float alpha_depth = m_AAFrameNum == 0 ? 1.0f : hit.t;
  res_color.w = alpha_depth;

  return res_color;
}

void MultiRenderer::kernel1D_ResolveOutline(uint32_t count, uint32_t sample_id, uint32_t render_mode, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    if (sample_id == 0)
      out_color[tidX] = ResolveOutline(tidX, render_mode);
    else
      out_color[tidX] += ResolveOutline(tidX, render_mode);
  }
}

float4 MultiRenderer::ResolveOutline(uint32_t tidX, uint32_t mode)
{
  CRT_Hit hit = m_gBuffer[tidX];

  if (hit.primId == 0xFFFFFFFF) // no hit
    return m_backgroundColor;

  unsigned type = hit.geomId >> SH_TYPE;
  unsigned geomId = hit.geomId & GEOM_ID_MASK;
  float4 rayPos, rayDir;
  initEyeRay(tidX % m_packedXY_width, tidX / m_packedXY_width, 0, &rayPos, &rayDir);

  float2 tc = float2(0, 0);
  float3 norm = float3(1, 0, 0);
  loadNormalTcFromGbuffer(hit, to_float3(rayDir), &norm, &tc);

  float3 color = float3(1, 1, 1);

  float3 final_color = float3(0, 0, 0);
  for (int i = 0; i < m_lights.size(); i++)
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
  const uint32_t x = tidX % m_packedXY_width;
  const uint32_t y = tidX / m_packedXY_width;
  int4 offsets = int4(x == 0 ? 0 : -1, 
                      y == 0 ? 0 : -int(m_packedXY_width), 
                      x == m_packedXY_width - 1 ? 0 : 1,
                      y == m_packedXY_height - 1 ? 0 : int(m_packedXY_width));
  offsets += int(tidX);

  if (mode == MULTI_RENDER_MODE_OUTLINE_BORDER)
  {
    const float tolerance = 10.0f*hit.t/float(m_packedXY_width);
    const float4 nearDist = float4(m_gBuffer[offsets.x].t, m_gBuffer[offsets.y].t, m_gBuffer[offsets.z].t, m_gBuffer[offsets.w].t);
    const float4 diff = abs(nearDist - hit.t);
    outline = diff.x > tolerance || diff.y > tolerance || diff.z > tolerance || diff.w > tolerance;
  }

  return outline ? float4(0.2,0.2,0.2,1) : to_float4(final_color, 1);
}
