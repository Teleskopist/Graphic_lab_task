#include "VoxelRenderer.h"
#include "utils/raytracing/render_common.h"

void VoxelRenderer::Clear(uint32_t width, uint32_t height)
{
  kernel2D_PackXY(width, height);
}

bool VoxelRenderer::RayQuery_AnyHit(float4 posAndNear, float4 dirAndFar)
{
  return RayQuery_NearestHit(posAndNear, dirAndFar).primId != INVALID_IDX;
}

CRT_Hit VoxelRenderer::RayQuery_NearestHit(float4 posAndNear, float4 dirAndFar)
{
  CRT_Hit hit = CRT_Hit();
  switch (m_data_type)
  {
  case TYPE_SDF_DAG:
    hit = IntersectDAG(posAndNear, dirAndFar);
    break;
  case TYPE_SPARSE_VOXEL_OCTREE:
    hit = IntersectSVO(posAndNear, dirAndFar);
    break;
  }
  return hit;
}

void VoxelRenderer::Render(uint32_t *imageData, uint32_t width, uint32_t height, int passNum)
{
  assert(m_width == width);
  assert(m_height == height);

  uint32_t samples = passNum*m_preset.spp;
  for (uint32_t sample = 0; sample < samples; sample++)
  {
    if (m_data_type == TYPE_SDF_DAG)
    {
      kernel1D_IntersectDAG(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
    }
    else if (m_data_type == TYPE_SCOM2)
    {
      kernel1D_IntersectSCom(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
    }
    else if (m_data_type == TYPE_SPARSE_VOXEL_OCTREE)
    {
      if (m_preset.octree_traversal_mode == OCTREE_TRAVERSAL_MODE_DEFAULT)
      {
        kernel1D_IntersectSVO(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
      else if (m_preset.octree_traversal_mode == OCTREE_TRAVERSAL_MODE_FAST)
      {
        kernel1D_IntersectSVOFast(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
    }
  }
  kernel1D_Tonemap(m_width*m_height, samples, imageData);
}

void VoxelRenderer::RenderFloat(float4 *imageData, uint32_t width, uint32_t height, int passNum)
{
  assert(m_width == width);
  assert(m_height == height);

  uint32_t samples = passNum*m_preset.spp;
  for (uint32_t sample = 0; sample < samples; sample++)
  {
    if (m_data_type == TYPE_SDF_DAG)
    {
      kernel1D_IntersectDAG(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
    }
    else if (m_data_type == TYPE_SCOM2)
    {
      kernel1D_IntersectSCom(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
    }
    else if (m_data_type == TYPE_SPARSE_VOXEL_OCTREE)
    {
      if (m_preset.octree_traversal_mode == OCTREE_TRAVERSAL_MODE_DEFAULT)
      {
        kernel1D_IntersectSVO(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
      else if (m_preset.octree_traversal_mode == OCTREE_TRAVERSAL_MODE_FAST)
      {
        kernel1D_IntersectSVOFast(m_packedXY_width * m_packedXY_height, sample % m_preset.spp, m_colorBuffer.data());
      }
    }
  }
  kernel1D_AverageColor(m_width*m_height, samples, imageData);
}

void VoxelRenderer::kernel2D_PackXY(uint32_t w, uint32_t h)
{
  #pragma omp parallel for
  for(uint32_t y=0;y<h;y++)
  {
    for(uint32_t x=0;x<w;x++)
    {
      const uint offset   = SuperBlockIndex2DOpt(x, y, m_packedXY_width);
      m_packedXY[offset] = ((y << 16) & 0xFFFF0000) | (x & 0x0000FFFF);
    }
  }
}

void VoxelRenderer::kernel1D_Tonemap(uint32_t count, uint32_t sample_count, uint32_t* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    float4 color_f = m_colorBuffer[tidX] / float(sample_count);
    uint4 col = uint4(255 * clamp(color_f, float4(0,0,0,0), float4(1,1,1,1)));
    uint32_t color_rgba8 = (col.w<<24) | (col.z<<16) | (col.y<<8) | col.x;
    out_color[tidX] = color_rgba8;
  }
}

void VoxelRenderer::kernel1D_AverageColor(uint32_t count, uint32_t sample_count, LiteMath::float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
    out_color[tidX] = m_colorBuffer[tidX] / float(sample_count);
}

void VoxelRenderer::kernel1D_IntersectDAG(uint32_t count, uint32_t sample_id, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    CRT_Hit hit = IntersectDAG(rayPos, rayDir);
    float4 res = Resolve(hit, rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VoxelRenderer::kernel1D_IntersectSCom(uint32_t count, uint32_t sample_id, float4* out_color)
{
  //#pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    CRT_Hit hit = IntersectSCom(rayPos, rayDir);
    float4 res = Resolve(hit, rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VoxelRenderer::kernel1D_IntersectSVO(uint32_t count, uint32_t sample_id, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    CRT_Hit hit = IntersectSVO(rayPos, rayDir);
    float4 res = Resolve(hit, rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VoxelRenderer::kernel1D_IntersectSVOFast(uint32_t count, uint32_t sample_id, float4* out_color)
{
  #pragma omp parallel for
  for (uint32_t tidX = 0; tidX < count; tidX++)
  {
    const uint XY = m_packedXY[tidX];
    uint x  = (XY & 0x0000FFFF);
    uint y  = (XY & 0xFFFF0000) >> 16;
  
    float4 rayPos, rayDir;
    initEyeRay(x, y, sample_id, &rayPos, &rayDir);
    CRT_Hit hit = IntersectSVOFast(rayPos, rayDir);
    float4 res = Resolve(hit, rayPos, rayDir);
    m_colorBuffer[y*m_width + x] = sample_id == 0 ? res : m_colorBuffer[y*m_width + x] + res;
  }
}

void VoxelRenderer::initEyeRay(uint32_t x, uint32_t y, uint32_t s_id, 
                                LiteMath::float4* rayPosAndNear, LiteMath::float4* rayDirAndFar)
{
  uint32_t spp_sqrt = uint32_t(sqrt(m_preset.spp));
  float i_spp_sqrt = 1.0f/spp_sqrt;
  float2 d = (m_AAFrameNum >= 2) ? rand2(m_seed, x, y, s_id + m_AAFrameNum + m_seed % 2) : 
                                   i_spp_sqrt*float2(s_id/spp_sqrt+0.5, s_id%spp_sqrt+0.5);

  float3 rayDir = EyeRayDirNormalized((float(x)+d.x)/float(m_packedXY_width), (float(y)+d.y)/float(m_packedXY_height), m_projInv);
  float3 rayPos = float3(0,0,0);

  transform_ray3f(m_worldViewInv, 
                  &rayPos, &rayDir);
  
  float rayNear = 1e-5f;
  float rayFar  =  1e5f;
  float sgn = sign(dot(rayPos - m_cuttingPlane.pos, m_cuttingPlane.normal));
  float denom = m_cuttingPlane.is_active > 0 ? dot(m_cuttingPlane.normal, rayDir) : 0.0f;
  rayNear = sgn*std::abs(denom) < -1e-6f ? dot(m_cuttingPlane.pos - rayPos, m_cuttingPlane.normal) / denom : rayNear;
  rayFar  = sgn*std::abs(denom) >  1e-6f ? dot(m_cuttingPlane.pos - rayPos, m_cuttingPlane.normal) / denom : rayFar;
  *rayPosAndNear = to_float4(rayPos, rayNear < 0 ? rayFar : rayNear);
  *rayDirAndFar  = to_float4(rayDir, rayFar < 0 ?    1e9f :  rayFar);
}

float4 VoxelRenderer::Resolve(const CRT_Hit &hit, float4 posAndNear, float4 dirAndFar)
{
  if (hit.primId == INVALID_IDX)
  {
    float4 c = m_backgroundColor;
    c.w = m_AAFrameNum == 0 ? 1.0f : -1.0f;
    return c;
  }

  // if we use AA, we should put depth into alpha channel
  float alpha_depth = m_AAFrameNum == 0 ? 1.0f : hit.t;
  float3 q = float3(hit.coords[0], hit.coords[1], hit.coords[2]);
  float3 norm = float3(0,0,0);
  float3 v = float3(std::max(q.x, 1 - q.x),
                    std::max(q.y, 1 - q.y),
                    std::max(q.z, 1 - q.z));
  int dim = 0;
  if (v.y > v.x && v.y > v.z)
    dim = 1;
  else if (v.z > v.x && v.z > v.y)
    dim = 2;
  norm[dim] = q[dim] < 0.5f ? -1.0f : 1.0f;

  if (m_preset.render_mode == VOXEL_RENDER_MODE_MASK)
  {
    return float4(1,1,1,alpha_depth);
  }
  else if (m_preset.render_mode == VOXEL_RENDER_MODE_NORMAL)
  {
    v = abs(norm);
    return float4(0.25f, 0.25f, 0.25f, 0.5f) + 0.5f*float4(pow(v.x, 1.0f/1.5f), pow(v.y, 1.0f/1.5f), pow(v.z, 1.0f/1.5f), alpha_depth);
  }
  else if (m_preset.render_mode == VOXEL_RENDER_MODE_PRIMITIVE)
  {
    return decode_RGBA8(m_palette[(hit.primId) % palette_size]); 
  }
  else if (m_preset.render_mode == VOXEL_RENDER_MODE_LAMBERT ||
           m_preset.render_mode == VOXEL_RENDER_MODE_LAMBERT_NO_TEX)
  {
    uint32_t side = 2*dim + (q[dim] > 0.5f ? 0 : 1);
    float3 color = float3(1,0,1);
    if (m_preset.render_mode == MULTI_RENDER_MODE_LAMBERT_NO_TEX)
      color = float3(1,1,1);
    else
    {
      float tmp = dim == 2 ? q.x : q.z;
      float2 tc = float2(q[dim] > 0.5f ? tmp : 1.0f-tmp, 1.0f-q[(dim+1)%2]);
      uint32_t texId = m_blocks[hit.geomId].texIds[side];
      color = to_float3(m_textures[texId]->sample(tc));
    }

    float3 final_color = float3(0,0,0);
    for (int i=0;i<m_lights.size();i++)
    {
      if (m_lights[i].type == LIGHT_TYPE_DIRECT)
      {
        float light_q = max(0.0f, dot(norm, m_lights[i].space));
        final_color += m_lights[i].color * color * light_q;
      }
      else if (m_lights[i].type == LIGHT_TYPE_POINT)
      {
        float3 surf_pos = to_float3(posAndNear) + hit.t * to_float3(dirAndFar);
        float3 dir = m_lights[i].space - surf_pos;
        float l = length(dir);
        dir /= l;
        float light_q = max(0.0f, dot(norm, dir));
        final_color += m_lights[i].color * color * light_q / (l*l);
      }
      else
        final_color += m_lights[i].color * color;
    }

    return to_float4(final_color, alpha_depth);
  }

  return float4(1,0,1,alpha_depth);
}

CRT_Hit VoxelRenderer::IntersectDAG(float4 posAndNear, float4 dirAndFar)
{
  CRT_Hit hit;
  hit.geomId = INVALID_IDX;
  hit.instId = INVALID_IDX;
  hit.primId = INVALID_IDX;
  hit.t = dirAndFar.w;
  hit.coords[0] = 1;
  hit.coords[1] = 0;
  hit.coords[2] = 0;
  hit.coords[3] = 0;

#ifndef DISABLE_SDF_DAG
  //It is a naive version that iterates all child nodes, instead of traversing it in octree-like fashion
  //but DAG is a mostly for debugging similarity compression, so it does not really matter
  //Still, we should optimize it if performance testing of octree/64-tree is considered

  const float3 ray_pos = to_float3(posAndNear);
  const float3 ray_dir = to_float3(dirAndFar);
  const float tNear = posAndNear.w;
  const float tFar = dirAndFar.w;
  const float3 ray_dir_inv = SafeInverse(ray_dir);
  OTStackElement stack[32];

  int top = 0;
  int buf_top = 0;
  uint3 p;
  float3 p_f, t0, t1, tm;
  int currNode;
  uint32_t nodeId;
  uint32_t level_sz;
  float d, level_sz_f, sz_inv;
  float2 fNearFar;
  float values[8];
  SdfDAGHeader header = m_SdfDAGHeaders[0];//m_SdfDAGHeaders[m_geomData[geomId].offset.y];

  stack[top].nodeId = 0;//m_geomData[geomId].offset.x;
  stack[top].info = 0; //info here is cumulative rotation id of the node
  stack[top].p_size = uint2(0,1);

  while (top >= 0)
  {
    OTStackElement curElem = stack[top];
    top--;

    SdfDAGNode node = m_SdfDAGNodes[curElem.nodeId];
    level_sz = curElem.p_size.y & 0xFFFF;
    level_sz_f = float(level_sz);
    p = uint3(curElem.p_size.x >> 16, curElem.p_size.x & 0xFFFF, curElem.p_size.y >> 16);
    
    sz_inv = 2.0f/level_sz_f;
    float3 brick_min_pos = float3(-1,-1,-1) + sz_inv*float3(p);
    float3 brick_max_pos = brick_min_pos + sz_inv*float3(1,1,1);
    float3 brick_size = brick_max_pos - brick_min_pos;

    float2 brick_fNearFar = RayBoxIntersection2(ray_pos, ray_dir_inv, brick_min_pos, brick_max_pos);
    brick_fNearFar.x = std::max(tNear, brick_fNearFar.x);

#ifdef ON_CPU
    //printf("[%d %d %d %d] node %d rot %d\n", p.x, p.y, p.z, level_sz, curElem.nodeId, curElem.info);
#endif
    if (brick_fNearFar.x >= brick_fNearFar.y)
    {
      continue;
    }

    bool is_leaf = node.children_edges_offset == 0;
    // if (!is_leaf)
    // {
    //   float t = m_preset.fixed_lod > 0 ? 1.0f : brick_fNearFar.x;
    //   float target_lod_size = (std::pow(2.0f, m_preset.level_of_detail) - 0.01f) / t;
    //   is_leaf = float(level_sz) > target_lod_size;
    // }

    if (is_leaf)
    {
      //intersect with grid inside leaf
      if (DAG_node_is_full(node.voxel_count_flags) && node.data_edges_offset > 0)
      {
        //intersect leaf
        float sz = level_sz_f;
        d = 2.0f/(sz*header.brick_size);
        bool is_surface = DAG_extract_is_surface(node.voxel_count_flags);
        uint32_t surface_count = DAG_extract_count(node.voxel_count_flags);
        uint32_t primId = node.channels_edge.child_offset; // primId is used to access channel data in Resolve pass
                                                           // so we must put channels offset here to apply sim. comp. to channels

        while (brick_fNearFar.x < brick_fNearFar.y)
        {
          uint32_t v_size = header.brick_size + 2*header.brick_pad + 1;
          float3 hit_pos = ray_pos + brick_fNearFar.x*ray_dir;
          float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*sz*header.brick_size);
          float3 voxelPos = floor(clamp(local_pos, 1e-6f, header.brick_size-1e-6f));

          float3 min_pos = brick_min_pos + d*voxelPos;
          float3 max_pos = min_pos + d*float3(1,1,1);
          float3 size = max_pos - min_pos;

          uint32_t s_id = 0;
          while (s_id < surface_count)
          {
            uint32_t offset = m_SdfDAGDataEdges[node.data_edges_offset + s_id].data_offset;
            int4 voxelPosI = int4(int(voxelPos.x), int(voxelPos.y), int(voxelPos.z), 1);
            uint32_t rotIdx = m_RotAddTable[curElem.info*ROT_COUNT + m_SdfDAGDataEdges[node.data_edges_offset + s_id].rotation_id];
            float add = m_SdfDAGDataEdges[node.data_edges_offset + s_id].add;
            float vmin = 1.0f;
            float vmax = is_surface ? -1.0f : 1.0f;
            for (int i=0;i<8;i++)
            {
              int4 pI = voxelPosI + int4((i & 4) >> 2, (i & 2) >> 1, i & 1, 0);
              uint32_t vId0 = uint32_t(dot(m_SdfCompactOctreeRotModifiers[rotIdx], pI));
              float val = m_SdfDAGDistances[offset + vId0] + add;
              values[i] = val;
              //printf("vId0 = %d, offset = %d add = %f, val = %f\n", vId0, offset, add, values[i]);
              vmin = std::min(vmin, val);
              vmax = std::max(vmax, val);
            }

            fNearFar = RayBoxIntersection2(ray_pos, SafeInverse(ray_dir), min_pos, max_pos);
            if (fNearFar.x < fNearFar.y && vmin <= 0.0f && vmax >= 0.0f)    
            {
              float3 start_pos = ray_pos + fNearFar.x * ray_dir;
              float vox_sz = (0.5f * level_sz_f * header.brick_size);
              float3 start_q = vox_sz * (start_pos - min_pos);
              
              // if this node size is smaller than m_SVO_max_level_size, it should be textured 
              // as a grid of voxels, so we change start_q accordingly
              float q_mult = float(m_DAG_max_level_size / level_sz);
              start_q = clamp(start_q, float3(1e-6f), float3(1.0f - 1e-6f));
              start_q = fract(start_q * q_mult);

              hit.t = fNearFar.x;
              hit.primId = node.data_edges_offset;
              hit.geomId = m_SdfDAGBlockIds[node.channels_edge.child_offset];
              
              hit.coords[0] = start_q.x;
              hit.coords[1] = start_q.y;
              hit.coords[2] = start_q.z;
              return hit;
            }
            s_id++;
          }
          brick_fNearFar.x += std::max(0.0f, fNearFar.y-brick_fNearFar.x) + 1e-5f;
        }
      }
    }
    else if (brick_fNearFar.x < brick_fNearFar.y)
    {
      //traverse children and add to stack (in reverse order)
      int v_sz = header.node_grid_size;
      float t_neg = (brick_fNearFar.y + tNear + 1);
      float3 neg_pos = ray_pos + t_neg * ray_dir;
      float3 neg_dir = -ray_dir;
      float3 neg_dir_inv = -ray_dir_inv;
      float2 neg_brick_fNearFar = float2(t_neg - brick_fNearFar.y, t_neg - brick_fNearFar.x);

      while (neg_brick_fNearFar.x < neg_brick_fNearFar.y)
      {
        d = 2.0f/(level_sz_f*v_sz);
        float3 hit_pos = neg_pos + neg_brick_fNearFar.x*neg_dir;
        float3 local_pos = (hit_pos - brick_min_pos) * (0.5f*level_sz_f*v_sz);
        float3 voxelPos = floor(clamp(local_pos, 1e-6f, v_sz-1e-6f));
        uint32_t voxId = uint32_t(dot(m_SdfCompactOctreeRotModifiers[2*ROT_COUNT + curElem.info], 
                                      int4((int)voxelPos.x, (int)voxelPos.y, (int)voxelPos.z, 1)));

        float3 min_pos = brick_min_pos + d*voxelPos;
        float3 max_pos = min_pos + d*float3(1,1,1);
        float3 size = max_pos - min_pos;

        fNearFar = RayBoxIntersection2(neg_pos, neg_dir_inv, min_pos, max_pos);
        SdfDAGChildEdge edge = m_SdfDAGChildEdges[node.children_edges_offset + voxId];

        //if we itersected with this node bbox and this node is valid
        if (tNear < fNearFar.x && edge.child_offset > 0)    
        {
          SdfDAGNode child_node = m_SdfDAGNodes[edge.child_offset];
          top++;
          stack[top].nodeId = edge.child_offset;
          stack[top].info = m_RotAddTable[curElem.info*ROT_COUNT + edge.rotation_id];
          stack[top].p_size = (curElem.p_size*v_sz) + uint2((uint(voxelPos.x) << 16) | uint(voxelPos.y), (uint(voxelPos.z) << 16) | 0);
        }

        neg_brick_fNearFar.x += std::max(0.0f, fNearFar.y-neg_brick_fNearFar.x) + 0.01f*d;
      }
    }
  }
  return hit;
#endif
}

CRT_Hit VoxelRenderer::IntersectSVO(float4 posAndNear, float4 dirAndFar)
{
  CRT_Hit hit;
  hit.geomId = INVALID_IDX;
  hit.instId = INVALID_IDX;
  hit.primId = INVALID_IDX;
  hit.t = dirAndFar.w;
  hit.coords[0] = 1;
  hit.coords[1] = 0;
  hit.coords[2] = 0;
  hit.coords[3] = 0;

  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return hit;
  const float tNear = std::max(posAndNear.w, boxNearFar.x);
  const float tFar  = std::min(dirAndFar.w, boxNearFar.y);
  float3 pos_ray_pos = to_float3(posAndNear);
  float3 pos_ray_dir = to_float3(dirAndFar);

  //assume octree is box [-1,1]^3
  uint32_t a = 0;
  if (dirAndFar.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (dirAndFar.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (dirAndFar.z < 0)
  {
    pos_ray_pos.z *= -1;
    pos_ray_dir.z *= -1;
    a |= 1;
  }

  pos_ray_pos *= -1;
  const float3 pos_ray_dir_inv = SafeInverse(pos_ray_dir);
  const float3 _t0 = pos_ray_pos * pos_ray_dir_inv - pos_ray_dir_inv;
  const float3 _t1 = pos_ray_pos * pos_ray_dir_inv + pos_ray_dir_inv;
  const float3 _l = _t1 - _t0;

  const uint3 nn_indices[8] = {uint3(4, 2, 1), uint3(5, 3, 8), uint3(6, 8, 3), uint3(7, 8, 8),
                               uint3(8, 6, 5), uint3(8, 7, 8), uint3(8, 8, 7), uint3(8, 8, 8)};

  OTStackElement stack[32];
  OTStackElement tmp_buf[4];

  int top = 0;
  int buf_top = 0;
  uint3 p;
  float3 p_f, t0, t1, tm;
  int currNode;
  uint32_t nodeId;
  uint32_t level_sz;
  float d, qNear, qFar;
  float2 fNearFar;
  float3 start_q;
  float values[8];

  stack[top].nodeId = 0;
  stack[top].info = 0;
  stack[top].p_size = uint2(0,1);

  while (top >= 0)
  {
    level_sz = stack[top].p_size.y & 0xFFFF;
    p = uint3(stack[top].p_size.x >> 16, stack[top].p_size.x & 0xFFFF, stack[top].p_size.y >> 16);
    d = 1.0f / float(level_sz);
    p_f = float3(p);
    t0 = _t0 + d * p_f * _l;
    t1 = _t0 + d * (p_f + 1) * _l;

    float t_out = std::min(std::min(t1.x, t1.y), t1.z);
    if (t_out < tNear)
    {
      top--;
      continue;
    }

    if (stack[top].info > 0)
    {
      float tmin = std::max(t0.x, std::max(t0.y, t0.z));
      float tmax = std::min(t1.x, std::min(t1.y, t1.z));

      uint32_t p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,
                           ((a & 2) > 0) ? (~p.y & p_mask) : p.y,
                           ((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      nodeId = stack[top].nodeId;
      float sz = 0.5f * float(level_sz);
      d = 1.0f / sz;
      float3 min_pos = float3(-1, -1, -1) + d * float3(real_p.x, real_p.y, real_p.z);

      fNearFar = float2(tmin, tmax);
      float3 start_pos = to_float3(posAndNear) + fNearFar.x * to_float3(dirAndFar);
      start_q = sz * (start_pos - min_pos);

      // if this node size is smaller than m_SVO_max_level_size, it should be textured 
      // as a grid of voxels, so we change start_q accordingly
      float q_mult = float(m_SVO_max_level_size / level_sz);
      start_q = clamp(start_q, float3(1e-6f), float3(1.0f - 1e-6f));
      start_q = fract(start_q * q_mult);

      hit.t = fNearFar.x;
      hit.primId = stack[top].nodeId;
      hit.geomId = m_SVO[stack[top].nodeId];
      hit.instId = 0;
      //printf("leaf geomId %d\n", hit.geomId);

      hit.coords[0] = start_q.x;
      hit.coords[1] = start_q.y;
      hit.coords[2] = start_q.z;
      return hit;
    }
    else
    {
      uint32_t node = m_SVO[stack[top].nodeId];
      uint32_t shortPtr = stack[top].nodeId + (node & MAX_CHILD_POINTER);
      uint32_t childrenOff = (node & IS_FAR_BIT) > 0 ? m_SVO[shortPtr] : shortPtr;

      buf_top = 0;
      tm = 0.5f * (t0 + t1);
      currNode = first_node(t0, tm);
      do
      {
        uint32_t childN = currNode ^ a; // child node number, from 0 to 8
        uint32_t childHasData = node & (1u << (24+childN));
        //printf("intersect child %d\n", childN);

        // if child is active
        if (childHasData > 0)
        {
          uint32_t childPos = bitCount((node >> 24) & ((1u << childN) - 1));
          uint32_t childIsLeaf = (node & (1u << (16+childN)));
          //printf("child %d %d (%d)\n", childrenOff, childPos, childIsLeaf);
          tmp_buf[buf_top].nodeId = childrenOff + childPos;
          tmp_buf[buf_top].info = childIsLeaf; // > 0 is child is leaf
          tmp_buf[buf_top].p_size = (stack[top].p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1), (currNode & 1) << 16);
          buf_top++;
        }
        currNode = new_node(((currNode & 4) > 0) ? t1.x : tm.x, nn_indices[currNode].x,
                            ((currNode & 2) > 0) ? t1.y : tm.y, nn_indices[currNode].y,
                            ((currNode & 1) > 0) ? t1.z : tm.z, nn_indices[currNode].z);
      } while (currNode < 8);

      for (int i = 0; i < buf_top; i++)
      {
        stack[top + i] = tmp_buf[buf_top - i - 1];
      }
      top += buf_top - 1;
    }
  }

  return hit;
}

CRT_Hit VoxelRenderer::IntersectSVOFast(float4 posAndNear, float4 dirAndFar)
{
  CRT_Hit hit;
  hit.geomId = INVALID_IDX;
  hit.instId = INVALID_IDX;
  hit.primId = INVALID_IDX;
  hit.t = dirAndFar.w;
  hit.coords[0] = 1;
  hit.coords[1] = 0;
  hit.coords[2] = 0;
  hit.coords[3] = 0;

  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return hit;

  float3 pos_ray_pos = to_float3(posAndNear);
  float3 pos_ray_dir = to_float3(dirAndFar);

  //assume octree is box [-1,1]^3
  uint32_t a = 0;
  if (dirAndFar.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (dirAndFar.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (dirAndFar.z < 0)
  {
    pos_ray_pos.z *= -1;
    pos_ray_dir.z *= -1;
    a |= 1;
  }

  pos_ray_pos *= -1;
  const float3 pos_ray_dir_inv = SafeInverse(pos_ray_dir);
  const float3 _t0 = pos_ray_pos * pos_ray_dir_inv - pos_ray_dir_inv;
  const float3 _t1 = pos_ray_pos * pos_ray_dir_inv + pos_ray_dir_inv;
  const float3 _l = _t1 - _t0;

  OTStackElement stack[16];

  int top = 0;
  float d = 1.0f;
  float3 t0 = _t0;
  float3 tm = t0 + 0.5f*_l;

  OTStackElement cur;
  {
    uint32_t node = m_SVO[0];
    uint32_t shortPtr = 0 + (node & MAX_CHILD_POINTER);
    uint32_t childrenOff = (node & IS_FAR_BIT) > 0 ? m_SVO[shortPtr] : shortPtr;

    cur.nodeId = childrenOff;
    cur.info = first_node(t0, tm) | (node & 0xFFFF0000u);
    cur.p_size = uint2(0,1);
  }

  stack[0].nodeId = 0xFFFFFFFFu;
  stack[0].info = 0;
  stack[0].p_size = uint2(0,0);

  while (top >= 0)
  {
    uint32_t currNode = cur.info & 0x7;
    uint32_t childrenOff = cur.nodeId;
    uint32_t childN = currNode ^ a; // child node number, from 0 to 8
    uint32_t childHasData = cur.info & (1u << (24+childN));
    uint32_t childIsLeaf  = cur.info & (1u << (16+childN));

    d = 1.0f / float(cur.p_size.y & 0xFFFF);
    float3 pf2 = float3(float(cur.p_size.x >> 16) + (((currNode & 4) > 0) ? 1.0f : 0.5f),
                        float(cur.p_size.x & 0xFFFF) + (((currNode & 2) > 0) ? 1.0f : 0.5f),
                        float(cur.p_size.y >> 16) + (((currNode & 1) > 0) ? 1.0f : 0.5f));
    float3 tl = _t0 + d * pf2 * _l;

    //this child is empty, go to the next child of this node
    if (std::min(std::min(tl.x, tl.y), tl.z) < posAndNear.w || childHasData == 0)
    {
      uint32_t nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode | (cur.info & 0xFFFF0000u);
    }
    else if (childIsLeaf > 0) // we intersected with leaf, fill the hit data and return
    {
      uint32_t level_sz = 2*(cur.p_size.y & 0xFFFF);
      uint3 p = 2*uint3(cur.p_size.x >> 16, cur.p_size.x & 0xFFFF, cur.p_size.y >> 16) + 
            uint3((currNode & 4) >> 2, (currNode & 2) >> 1, currNode & 1);
      d = 1.0f / float(level_sz);
      float3 p_f = float3(p);
      t0 = _t0 + d * p_f * _l;

      uint32_t childPos = bitCount((cur.info >> 24) & ((1u << childN) - 1));
      uint32_t childNodeId = childrenOff + childPos;

      uint32_t p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,
                           ((a & 2) > 0) ? (~p.y & p_mask) : p.y,
                           ((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      float tmin = std::max(t0.x, std::max(t0.y, t0.z));
      float3 start_pos = to_float3(posAndNear) + tmin * to_float3(dirAndFar);
      float3 min_pos = float3(-1, -1, -1) + 2.0f * d * float3(real_p.x, real_p.y, real_p.z);
      float3 start_q = 0.5f * float(level_sz) * (start_pos - min_pos);

      // if this node size is smaller than m_SVO_max_level_size, it should be textured 
      // as a grid of voxels, so we change start_q accordingly
      float q_mult = float(m_SVO_max_level_size / level_sz);
      start_q = fract(q_mult * clamp(start_q, float3(1e-6f), float3(1.0f - 1e-6f)));

      hit.t = tmin;
      hit.primId = childNodeId;
      hit.geomId = m_SVO[childNodeId];
      hit.instId = 0;
      hit.coords[0] = start_q.x;
      hit.coords[1] = start_q.y;
      hit.coords[2] = start_q.z;

      return hit;
    }
    else // this child is a node, move to it
    {
      uint32_t nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);

      if (nextChildOfCurNode < 8)  // if it is not a last child, leave the current node and place the child in the stack
      {
        cur.info = nextChildOfCurNode | (cur.info & 0xFFFF0000u);
        stack[++top] = cur;
      }

      d = 0.5f / float(cur.p_size.y & 0xFFFF);
      float3 p_f = float3(2*(cur.p_size.x >> 16) + ((currNode & 4) >> 2),
                          2*(cur.p_size.x & 0xFFFF) + ((currNode & 2) >> 1),
                          2*(cur.p_size.y >> 16) + (currNode & 1));
      t0 = _t0 + d * p_f * _l;
      tm = t0 + 0.5f*d*_l;

      uint32_t childPos = bitCount((cur.info >> 24) & ((1u << childN) - 1));
      uint32_t childNodeId = childrenOff + childPos;
      uint32_t node = m_SVO[childNodeId];
      uint32_t shortPtr = childNodeId + (node & MAX_CHILD_POINTER);
      uint32_t nextChildrenOff = (node & IS_FAR_BIT) > 0 ? m_SVO[shortPtr] : shortPtr;

      cur.nodeId = nextChildrenOff;
      cur.info = first_node(t0, tm) | (node & 0xFFFF0000u);
      cur.p_size = (cur.p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1), (currNode & 1) << 16);
    }
  }

  return hit;
}

CRT_Hit VoxelRenderer::IntersectSCom(float4 posAndNear, float4 dirAndFar)
{
  CRT_Hit hit;
  hit.geomId = INVALID_IDX;
  hit.instId = INVALID_IDX;
  hit.primId = INVALID_IDX;
  hit.t = dirAndFar.w;
  hit.coords[0] = 1;
  hit.coords[1] = 0;
  hit.coords[2] = 0;
  hit.coords[3] = 0;

  float2 boxNearFar = RayBoxIntersection2(to_float3(posAndNear), SafeInverse(to_float3(dirAndFar)), float3(-1,-1,-1), float3(1,1,1));
  if (boxNearFar.x > boxNearFar.y) 
    return hit;

  float3 pos_ray_pos = to_float3(posAndNear);
  float3 pos_ray_dir = to_float3(dirAndFar);

  //assume octree is box [-1,1]^3
  uint32_t a = 0;
  if (dirAndFar.x < 0)
  {
    pos_ray_pos.x *= -1;
    pos_ray_dir.x *= -1;
    a |= 4;
  }

  if (dirAndFar.y < 0)
  {
    pos_ray_pos.y *= -1;
    pos_ray_dir.y *= -1;
    a |= 2;
  }

  if (dirAndFar.z < 0)
  {
    pos_ray_pos.z *= -1;
    pos_ray_dir.z *= -1;
    a |= 1;
  }

  pos_ray_pos *= -1;
  const float3 pos_ray_dir_inv = SafeInverse(pos_ray_dir);
  const float3 _t0 = pos_ray_pos * pos_ray_dir_inv - pos_ray_dir_inv;
  const float3 _t1 = pos_ray_pos * pos_ray_dir_inv + pos_ray_dir_inv;
  const float3 _l = _t1 - _t0;

  ExtStackElement stack[16];

  int top = 0;
  float d = 1.0f;
  float3 t0 = _t0;
  float3 tm = t0 + 0.5f*_l;

  ExtStackElement cur;
  {
    uint32_t node = m_SComNodes[m_header.root_node_off];
    uint32_t currNode = first_node(t0, tm);

    cur.linksOff = m_header.root_node_off + m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1);
    cur.info = currNode | (node & 0xFFFFFF00u);
    cur.transform = 0;
    cur.p_size = uint2(0,1);
  }

  stack[0].linksOff = 0xFFFFFFFFu;
  stack[0].info = 0;
  stack[0].transform = 0;
  stack[0].p_size = uint2(0,0);

  while (top >= 0)
  {
    uint32_t currNode = cur.info & 0x7;
    uint32_t rawChildNode = currNode ^ a; 
    uint3 voxelPos  = uint3((rawChildNode & 4) >> 2, (rawChildNode & 2) >> 1, rawChildNode & 1);
    uint32_t childN = uint32_t(dot(m_SdfCompactOctreeRotModifiers[2 * ROT_COUNT + cur.transform], // child node number, from 0 to 8
                                   int4((int)voxelPos.x, (int)voxelPos.y, (int)voxelPos.z, 1)));
    uint32_t childLink = cur.linksOff + bitCount((cur.info >> 24) & ((1u << childN) - 1));
    uint32_t childHasData = cur.info & (1u << (24+childN));
    uint32_t childIsLeaf  = cur.info & (1u << (8+2*childN));

    d = 1.0f / float(cur.p_size.y & 0xFFFF);
    float3 pf2 = float3(float(cur.p_size.x >> 16) + (((currNode & 4) > 0) ? 1.0f : 0.5f),
                        float(cur.p_size.x & 0xFFFF) + (((currNode & 2) > 0) ? 1.0f : 0.5f),
                        float(cur.p_size.y >> 16) + (((currNode & 1) > 0) ? 1.0f : 0.5f));
    float3 tl = _t0 + d * pf2 * _l;

    //this child is empty, go to the next child of this node
    if (std::min(std::min(tl.x, tl.y), tl.z) < posAndNear.w || childHasData == 0)
    {
      uint32_t nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);
      if (nextChildOfCurNode >= 8)
        cur = stack[top--];
      else
        cur.info = nextChildOfCurNode  | (cur.info & 0xFFFFFF00u);
    }
    else if (childIsLeaf > 0) // we intersected with leaf, fill the hit data and return
    {
      uint32_t level_sz = 2*(cur.p_size.y & 0xFFFF);
      uint3 p = 2*uint3(cur.p_size.x >> 16, cur.p_size.x & 0xFFFF, cur.p_size.y >> 16) + 
            uint3((currNode & 4) >> 2, (currNode & 2) >> 1, currNode & 1);
      d = 1.0f / float(level_sz);
      float3 p_f = float3(p);
      t0 = _t0 + d * p_f * _l;

      uint32_t p_mask = level_sz - 1;
      uint3 real_p = uint3(((a & 4) > 0) ? (~p.x & p_mask) : p.x,
                           ((a & 2) > 0) ? (~p.y & p_mask) : p.y,
                           ((a & 1) > 0) ? (~p.z & p_mask) : p.z);

      float tmin = std::max(t0.x, std::max(t0.y, t0.z));
      float3 start_pos = to_float3(posAndNear) + tmin * to_float3(dirAndFar);
      float3 min_pos = float3(-1, -1, -1) + 2.0f * d * float3(real_p.x, real_p.y, real_p.z);
      float3 start_q = 0.5f * float(level_sz) * (start_pos - min_pos);

      // if this node size is smaller than m_DAG_max_level_size, it should be textured 
      // as a grid of voxels, so we change start_q accordingly
      float q_mult = float(m_DAG_max_level_size / level_sz);
      start_q = fract(q_mult * clamp(start_q, float3(1e-6f), float3(1.0f - 1e-6f)));

      hit.t = tmin;
      hit.primId = childLink;
      hit.geomId = m_SComNodes[childLink] & m_header.node_offset_mask;
      hit.instId = 0;
      hit.coords[0] = start_q.x;
      hit.coords[1] = start_q.y;
      hit.coords[2] = start_q.z;

      return hit;
    }
    else // this child is a node, move to it
    {
      uint32_t nextChildOfCurNode = new_node(currNode, tl.x, tl.y, tl.z);

      if (nextChildOfCurNode < 8)  // if it is not a last child, leave the current node and place the child in the stack
      {
        cur.info = nextChildOfCurNode | (cur.info & 0xFFFFFF00u);
        stack[++top] = cur;
      }

      d = 0.5f / float(cur.p_size.y & 0xFFFF);
      float3 p_f = float3(2*(cur.p_size.x >> 16) + ((currNode & 4) >> 2),
                          2*(cur.p_size.x & 0xFFFF) + ((currNode & 2) >> 1),
                          2*(cur.p_size.y >> 16) + (currNode & 1));
      t0 = _t0 + d * p_f * _l;
      tm = t0 + 0.5f*d*_l;

      SdfDAGChildEdge ce = unpack_child_edge(m_header, m_SComNodes[childLink], m_SComNodes[childLink]);
      uint32_t node = m_SComNodes[ce.child_offset];
      uint32_t nextNode = first_node(t0, tm);

      cur.linksOff = ce.child_offset + m_header.links_offset + ((node & 0xFF) == SCOM2_CHILD_EMPTY ? 0 : 1);
      cur.transform = m_RotAddTable[cur.transform * ROT_COUNT + ce.rotation_id];
      cur.info = nextNode | (node & 0xFFFFFF00u);
      cur.p_size = (cur.p_size << 1) | uint2(((currNode & 4) << (16 - 2)) | ((currNode & 2) >> 1), (currNode & 1) << 16);
    }
  }

  return hit;
}