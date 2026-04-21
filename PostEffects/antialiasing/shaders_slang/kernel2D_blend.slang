//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_slang.h"

RWStructuredBuffer<float4> in_data; // 
RWStructuredBuffer<uint> out_data; // 
RWStructuredBuffer<float4> m_AABuffer; // 
RWStructuredBuffer<float4> m_AABuffer1; // 
RWStructuredBuffer<Antialiaser_slang_UBO_Data> ubo;

void initEyeRay(uint x, uint y, int img_width, int img_height, inout float4 rayPos, inout float4 rayDir) ;



void initEyeRay(uint x, uint y, int img_width, int img_height, inout float4 rayPos, inout float4 rayDir) {
  float x_new = (float)x + 0.5f;
  x_new = x_new / (float)img_width;
  float y_new = (float)y + 0.5f;
  y_new = y_new / (float)img_height;

  float2 d = float2(x_new,y_new);

  float4 pos = float4(2.0f*d.x - 1.0f,-2.0f*d.y + 1.0f,1.0f,1.0f);
  pos = ubo[0].m_invProj * pos;
  pos /= pos.w;
  float3 dir = normalize(to_float3(pos));
  float3 rpos = float3(0,0,0);

  transform_ray3f(ubo[0].m_invWorldView, rpos, dir);
  
  rayPos = to_float4(rpos, 1.0f);
  rayDir  = to_float4(dir, 0.0f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct KernelArgs
{
  int height; 
  int width; 
  uint iNumElementsZ; 
  uint tFlagsMask;    
};



[shader("compute")]
[numthreads(32, 8, 1)]
void main(uint3 a_globalTID: SV_DispatchThreadID, uint3 a_localTID : SV_GroupThreadID, uniform KernelArgs kgenArgs)
{
  bool runThisThread = true;
  const int y = int(a_globalTID[0]); 
  const int x = int(a_globalTID[1]); 
  if(y >= kgenArgs.height + 0 || x >= kgenArgs.width + 0)
    runThisThread = false;
    // KERNEL BODY:
  //
  if(runThisThread)
  {
  
            float4 color = in_data[y * kgenArgs.width + x];

            if (color.w < 0.0f) {
                color.w = 10000.0f;
            }

            float4 rayPos;
            float4 rayDir;

            initEyeRay(x, y, kgenArgs.width, kgenArgs.height, rayPos, rayDir);

            float4 pos = rayPos + rayDir * color.w;

            float4 previous = ubo[0].m_previousMatrix * pos;
            previous /= previous.w;

            float2 uv = float2(previous.x,previous.y);

            float screen_x = (uv.x + 1.0f) / 2.0f * kgenArgs.width - 0.5f;
            float screen_y = (1.0f - uv.y) / 2.0f * kgenArgs.height - 0.5f;

            int min_x = int(screen_x);
            int min_y = int(screen_y);

            float dx = screen_x - min_x;
            float dy = screen_y - min_y;

            float4 boxMin = in_data[y * kgenArgs.width + x];
            float4 boxMax = in_data[y * kgenArgs.width + x];

            if (x > 0) {
                boxMin = min(boxMin, in_data[y * kgenArgs.width + x - 1]);
                boxMax = max(boxMax, in_data[y * kgenArgs.width + x - 1]);
            }
            if (x < kgenArgs.width - 1) {
                boxMin = min(boxMin, in_data[y * kgenArgs.width + x + 1]);
                boxMax = max(boxMax, in_data[y * kgenArgs.width + x + 1]);
            }
            if (y > 0) {
                boxMin = min(boxMin, in_data[y * kgenArgs.width + x - kgenArgs.width]);
                boxMax = max(boxMax, in_data[y * kgenArgs.width + x - kgenArgs.width]);
            }
            if (y < kgenArgs.height - 1) {
                boxMin = min(boxMin, in_data[y * kgenArgs.width + x + kgenArgs.width]);
                boxMax = max(boxMax, in_data[y * kgenArgs.width + x + kgenArgs.width]);
            }

            // == Moore neighborhood ==
            
            // if (x > 0 && y > 0) {
            //     boxMin = min(boxMin, in_data[y * width + x - 1 - width]);
            //     boxMax = max(boxMax, in_data[y * width + x - 1 - width]);
            // }
            // if (x < width - 1 && y > 0) {
            //     boxMin = min(boxMin, in_data[y * width + x + 1 - width]);
            //     boxMax = max(boxMax, in_data[y * width + x + 1 - width]);
            // }
            // if (x > 0 && y < height - 1) {
            //     boxMin = min(boxMin, in_data[y * width + x - 1 + width]);
            //     boxMax = max(boxMax, in_data[y * width + x - 1 + width]);
            // }
            // if (x < width - 1 && y < height - 1) {
            //     boxMin = min(boxMin, in_data[y * width + x + 1 + width]);
            //     boxMax = max(boxMax, in_data[y * width + x + 1 + width]);
            // }

            if (min_x >= 0 && min_y >= 0 && min_x < kgenArgs.width - 1 && min_y < kgenArgs.height - 1) {
                if (ubo[0].m_AAFrameNum % 2 == 0) {
                    color = ubo[0].m_AAFactor * in_data[y * kgenArgs.width + x] + (1.0f - ubo[0].m_AAFactor) * (m_AABuffer[min_y * kgenArgs.width + min_x] * (1.0f - dx) * (1.0f - dy) + m_AABuffer[min_y * kgenArgs.width + min_x + 1] * (1.0f - dy) * dx + m_AABuffer[(min_y + 1) * kgenArgs.width + min_x] * (1.0f - dx) * dy + m_AABuffer[(min_y + 1) * kgenArgs.width + min_x + 1] * dx * dy);
                } else {
                    color = ubo[0].m_AAFactor * in_data[y * kgenArgs.width + x] + (1.0f - ubo[0].m_AAFactor) * (m_AABuffer1[min_y * kgenArgs.width + min_x] * (1.0f - dx) * (1.0f - dy) + m_AABuffer1[min_y * kgenArgs.width + min_x + 1] * (1.0f - dy) * dx + m_AABuffer1[(min_y + 1) * kgenArgs.width + min_x] * (1.0f - dx) * dy + m_AABuffer1[(min_y + 1) * kgenArgs.width + min_x + 1] * dx * dy);
                }
            } else {
                color = in_data[y * kgenArgs.width + x];
            }

            color = clamp(color, boxMin, boxMax);
        
            color.w = in_data[y * kgenArgs.width + x].w;

            if (ubo[0].m_AAFrameNum % 2 == 0) {
                m_AABuffer1[y * kgenArgs.width + x] = color;
            } else {
                m_AABuffer[y * kgenArgs.width + x] = color;
            }

            color.w = 1.0f;

            uint4 col = uint4(255 * clamp(color, float4(0,0,0,0), float4(1,1,1,1)));
            out_data[y * kgenArgs.width + x] = (col.w << 24) | (col.z << 16) | (col.y << 8) | col.x;
        
  }
}

