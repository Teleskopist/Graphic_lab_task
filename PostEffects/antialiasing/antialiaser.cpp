#include "antialiaser.h"

#include "utils/raytracing/render_common.h"

using LiteMath::float2;
using LiteMath::float3;

void Antialiaser::Antialias(int width, int height, const float4* in_data, uint32_t* out_data)
{
        kernel2D_blend(width, height, in_data, out_data);
}

void Antialiaser::AntialiasAfterDenoiser(int width, int height, const float4* in_data, uint32_t* out_data)
{
        kernel2D_blend(width, height, in_data, out_data);
}

void Antialiaser::kernel2D_blend(int width, int height, const float4* in_data, uint32_t* out_data)
{
#pragma omp parallel for
    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            float4 color = in_data[y * width + x];

            if (color.w < 0.0f) {
                color.w = 10000.0f;
            }

            float4 rayPos;
            float4 rayDir;

            initEyeRay(x, y, width, height, &rayPos, &rayDir);

            float4 pos = rayPos + rayDir * color.w;

            float4 previous = m_previousMatrix * pos;
            previous /= previous.w;

            float2 uv = float2(previous.x, previous.y);

            float screen_x = (uv.x + 1.0f) / 2.0f * width - 0.5f;
            float screen_y = (1.0f - uv.y) / 2.0f * height - 0.5f;

            int min_x = int(screen_x);
            int min_y = int(screen_y);

            float dx = screen_x - min_x;
            float dy = screen_y - min_y;

            float4 boxMin = in_data[y * width + x];
            float4 boxMax = in_data[y * width + x];

            if (x > 0) {
                boxMin = min(boxMin, in_data[y * width + x - 1]);
                boxMax = max(boxMax, in_data[y * width + x - 1]);
            }
            if (x < width - 1) {
                boxMin = min(boxMin, in_data[y * width + x + 1]);
                boxMax = max(boxMax, in_data[y * width + x + 1]);
            }
            if (y > 0) {
                boxMin = min(boxMin, in_data[y * width + x - width]);
                boxMax = max(boxMax, in_data[y * width + x - width]);
            }
            if (y < height - 1) {
                boxMin = min(boxMin, in_data[y * width + x + width]);
                boxMax = max(boxMax, in_data[y * width + x + width]);
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

            if (min_x >= 0 && min_y >= 0 && min_x < width - 1 && min_y < height - 1) {
                if (m_AAFrameNum % 2 == 0) {
                    color = m_AAFactor * in_data[y * width + x] + (1.0f - m_AAFactor) * (m_AABuffer[min_y * width + min_x] * (1.0f - dx) * (1.0f - dy) + m_AABuffer[min_y * width + min_x + 1] * (1.0f - dy) * dx + m_AABuffer[(min_y + 1) * width + min_x] * (1.0f - dx) * dy + m_AABuffer[(min_y + 1) * width + min_x + 1] * dx * dy);
                } else {
                    color = m_AAFactor * in_data[y * width + x] + (1.0f - m_AAFactor) * (m_AABuffer1[min_y * width + min_x] * (1.0f - dx) * (1.0f - dy) + m_AABuffer1[min_y * width + min_x + 1] * (1.0f - dy) * dx + m_AABuffer1[(min_y + 1) * width + min_x] * (1.0f - dx) * dy + m_AABuffer1[(min_y + 1) * width + min_x + 1] * dx * dy);
                }
            } else {
                color = in_data[y * width + x];
            }

            color = clamp(color, boxMin, boxMax);
        
            color.w = in_data[y * width + x].w;

            if (m_AAFrameNum % 2 == 0) {
                m_AABuffer1[y * width + x] = color;
            } else {
                m_AABuffer[y * width + x] = color;
            }

            color.w = 1.0f;

            uint4 col = uint4(255 * clamp(color, float4(0, 0, 0, 0), float4(1, 1, 1, 1)));
            out_data[y * width + x] = (col.w << 24) | (col.z << 16) | (col.y << 8) | col.x;
        }
    }
}

void Antialiaser::initEyeRay(uint32_t x, uint32_t y, int img_width, int img_height, LiteMath::float4* rayPos, LiteMath::float4* rayDir)
{
  float x_new = (float)x + 0.5f;
  x_new = x_new / (float)img_width;
  float y_new = (float)y + 0.5f;
  y_new = y_new / (float)img_height;

  float2 d = float2(x_new, y_new);

  float4 pos = float4(2.0f*d.x - 1.0f, -2.0f*d.y + 1.0f, 1.0f, 1.0f);
  pos = m_invProj * pos;
  pos /= pos.w;
  float3 dir = normalize(to_float3(pos));
  float3 rpos = float3(0,0,0);

  transform_ray3f(m_invWorldView, &rpos, &dir);
  
  *rayPos = to_float4(rpos, 1.0f);
  *rayDir  = to_float4(dir, 0.0f);
}