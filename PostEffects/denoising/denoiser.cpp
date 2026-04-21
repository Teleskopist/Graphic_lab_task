#include "denoiser.h"

void Denoiser::Denoise(int width, int height, const float4* in_data, uint32_t* out_data)
{
        kernel2D_filter(width, height, in_data, out_data);
}

void Denoiser::DenoiseFloat(int width, int height, const float4* in_data, float4* out_data)
{
        kernel2D_filterFloat(width, height, in_data, out_data);
}

void Denoiser::kernel2D_filter(int width, int height, const float4* in_data, uint32_t* out_data)
{
#pragma omp parallel for
    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            float Wp = 0.0f;
            float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

            for (int j = max((-(m_KernelSize - 1) / 2) + y, 0); j < min((m_KernelSize / 2 + 1) + y, height); j++) {
                for (int i = max((-(m_KernelSize - 1) / 2) + x, 0); i < min((m_KernelSize / 2 + 1) + x, width); i++) {
                    float v = Kr(length(in_data[y * width + x] - in_data[j * width + i])) * Ks(length(float2(x, y) - float2(i, j)));
                    color += in_data[j * width + i] * v;
                    Wp += v;
                }
            }

            color /= Wp;

            uint4 col = uint4(255 * clamp(color, float4(0, 0, 0, 0), float4(1, 1, 1, 1)));
            out_data[y * width + x] = (col.w << 24) | (col.z << 16) | (col.y << 8) | col.x;
        }
    }
}

void Denoiser::kernel2D_filterFloat(int width, int height, const float4* in_data, float4* out_data)
{
#pragma omp parallel for
    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            float Wp = 0.0f;
            float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

            for (int j = max((-(m_KernelSize - 1) / 2) + y, 0); j < min((m_KernelSize / 2 + 1) + y, height); j++) {
                for (int i = max((-(m_KernelSize - 1) / 2) + x, 0); i < min((m_KernelSize / 2 + 1) + x, width); i++) {
                    float v = Kr(length(in_data[y * width + x] - in_data[j * width + i])) * Ks(length(float2(x, y) - float2(i, j)));
                    color += in_data[j * width + i] * v;
                    Wp += v;
                }
            }

            color /= Wp;

            out_data[y * width + x] = color;
        }
    }
}

float Denoiser::Kr(float p) {
    return exp(-p*p/(2.0f * m_SigmaR * m_SigmaR));
}

float Denoiser::Ks(float p) {
    return exp(-p*p/(2.0f * m_SigmaS * m_SigmaS));
}

void Denoiser::kernel2D_fastFilter(int width, int height, const float4* in_data, uint32_t* out_data)
{
#pragma omp parallel for
    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
            if (x==0 || y == 0 || x == width-1 || y == height-1)
            {
                color = in_data[y * width + x];
            }
            else
            {
                float3 c0 = to_float3(in_data[y * width + x]);
                float t0 = in_data[y * width + x].w;
                float3 c_av = float3(0,0,0);
                float c_sum = 0.0f;
                for (int j=y-1;j<=y+1;j++)
                {
                    for (int i=x-1;i<=x+1;i++)
                    {
                        float3 c = to_float3(in_data[j * width + i]);
                        float t = in_data[j * width + i].w;
                        if (std::abs(t-t0) < m_SigmaR)
                        {
                            c_av += c;
                            c_sum += 1.0f;   
                        } 
                    }
                }

                c_av /= c_sum;
                float diff = length(c_av - c0);
                color = LiteMath::to_float4(diff > m_SigmaS ? c_av : c0, t0);
            }
            uint4 col = uint4(255 * clamp(color, float4(0, 0, 0, 0), float4(1, 1, 1, 1)));
            out_data[y * width + x] = (col.w << 24) | (col.z << 16) | (col.y << 8) | col.x;
        }
    }
}

void Denoiser::kernel2D_fastFilterFloat(int width, int height, const float4* in_data, float4* out_data)
{
#pragma omp parallel for
    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
            if (x==0 || y == 0 || x == width-1 || y == height-1)
            {
                color = in_data[y * width + x];
            }
            else
            {
                float3 c0 = to_float3(in_data[y * width + x]);
                float t0 = in_data[y * width + x].w;
                float3 c_av = float3(0,0,0);
                float c_sum = 0.0f;
                for (int j=y-1;j<=y+1;j++)
                {
                    for (int i=x-1;i<=x+1;i++)
                    {
                        float3 c = to_float3(in_data[j * width + i]);
                        float t = in_data[j * width + i].w;
                        if (std::abs(t-t0) < m_SigmaR)
                        {
                            c_av += c;
                            c_sum += 1.0f;   
                        } 
                    }
                }

                c_av /= c_sum;
                float diff = length(c_av - c0);
                color = LiteMath::to_float4(diff > m_SigmaS ? c_av : c0, t0);
            }
            out_data[y * width + x] = color;
        }
    }
}