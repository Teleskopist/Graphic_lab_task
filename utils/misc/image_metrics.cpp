#include "image_metrics.h"
#include "omp.h"
#include "FLIP.h"

namespace image_metrics
{
  using LiteMath::uint4;
  uint32_t encode_RGBA8(float4 c)
  {
    uint4 col = uint4(255 * clamp(c, float4(0, 0, 0, 0), float4(1, 1, 1, 1)));
    return (col.w << 24) | (col.z << 16) | (col.y << 8) | col.x;
  }

  float4 decode_RGBA8(uint32_t c)
  {
    uint4 col = uint4(c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF, (c >> 24) & 0xFF);
    return float4(col.x * (1.0f / 255.0f), col.y * (1.0f / 255.0f), col.z * (1.0f / 255.0f), col.w * (1.0f / 255.0f));
  }

  LiteImage::Image2D<float> to_grayscale(const LiteImage::Image2D<float4> &image)
  {
    LiteImage::Image2D<float> res(image.width(), image.height());
    unsigned sz = image.vector().size();
    for (int i = 0; i < sz; i++)
      res.data()[i] = 0.2126f * image.vector()[i].x + 0.7152f * image.vector()[i].y + 0.0722f * image.vector()[i].z;
    return res;
  }

  float MSE(const LiteImage::Image2D<float> &image_1, const LiteImage::Image2D<float> &image_2)
  {
    if (image_1.width() == image_2.width() && image_1.height() == image_2.height())
    {
      unsigned sz = image_1.vector().size();
      double sum = 0.0;
      for (int i = 0; i < sz; i++)
      {
        float v1 = image_1.vector()[i];
        float v2 = image_2.vector()[i];
        sum += (v1-v2)*(v1-v2);
      }
      return sum / sz;
    }
    else
    {
      LiteImage::Sampler sampler;
      sampler.filter = LiteImage::Sampler::Filter::LINEAR;
      sampler.addressU = LiteImage::Sampler::AddressMode::CLAMP;
      sampler.addressV = LiteImage::Sampler::AddressMode::CLAMP;
      unsigned w = std::max(image_1.width(), image_2.width());
      unsigned h = std::max(image_1.height(), image_2.height());
      unsigned sz = w * h;
      double sum = 0.0;
      for (unsigned i = 0; i < h; i++)
      {
        for (unsigned j = 0; j < w; j++)
        {
          float v1 = image_1.sample(sampler, LiteMath::float2((float)j/w, (float)i/h)).x;
          float v2 = image_2.sample(sampler, LiteMath::float2((float)j/w, (float)i/h)).x;
          sum += (v1-v2)*(v1-v2);
        }
      }
      return sum / sz;
    }
  }

  float MSE(const LiteImage::Image2D<float4> &image_1, const LiteImage::Image2D<float4> &image_2)
  {
    if (image_1.width() == image_2.width() && image_1.height() == image_2.height())
    {
      unsigned sz = image_1.vector().size();
      double sum = 0.0;
      for (int i = 0; i < sz; i++)
      {
        float4 v1 = image_1.vector()[i];
        float4 v2 = image_2.vector()[i];
        sum += (v1.x-v2.x)*(v1.x-v2.x) + (v1.y-v2.y)*(v1.y-v2.y) + (v1.z-v2.z)*(v1.z-v2.z);
      }
      return sum / (3*sz);
    }
    else
    {
      LiteImage::Sampler sampler;
      sampler.filter = LiteImage::Sampler::Filter::LINEAR;
      sampler.addressU = LiteImage::Sampler::AddressMode::CLAMP;
      sampler.addressV = LiteImage::Sampler::AddressMode::CLAMP;
      unsigned w = std::max(image_1.width(), image_2.width());
      unsigned h = std::max(image_1.height(), image_2.height());
      unsigned sz = w * h;
      double sum = 0.0;
      for (unsigned i = 0; i < h; i++)
      {
        for (unsigned j = 0; j < w; j++)
        {
          float4 v1 = image_1.sample(sampler, LiteMath::float2((float)j/w, (float)i/h));
          float4 v2 = image_2.sample(sampler, LiteMath::float2((float)j/w, (float)i/h));
          sum += (v1.x-v2.x)*(v1.x-v2.x) + (v1.y-v2.y)*(v1.y-v2.y) + (v1.z-v2.z)*(v1.z-v2.z);
        }
      }
      return sum / (3*sz);
    }
  }

  float MSE(const LiteImage::Image2D<uint32_t> &image_1, const LiteImage::Image2D<uint32_t> &image_2)
  {
    if (image_1.width() == image_2.width() && image_1.height() == image_2.height())
    {
      unsigned sz = image_1.vector().size();
      double sum = 0.0;
      for (int i = 0; i < sz; i++)
      {
        float4 v1 = decode_RGBA8(image_1.vector()[i]);
        float4 v2 = decode_RGBA8(image_2.vector()[i]);
        sum += (v1.x-v2.x)*(v1.x-v2.x) + (v1.y-v2.y)*(v1.y-v2.y) + (v1.z-v2.z)*(v1.z-v2.z);
      }
      return sum / (3*sz);
    }
    else
    {
      LiteImage::Sampler sampler;
      sampler.filter = LiteImage::Sampler::Filter::LINEAR;
      sampler.addressU = LiteImage::Sampler::AddressMode::CLAMP;
      sampler.addressV = LiteImage::Sampler::AddressMode::CLAMP;
      unsigned w = std::max(image_1.width(), image_2.width());
      unsigned h = std::max(image_1.height(), image_2.height());
      unsigned sz = w * h;
      double sum = 0.0;
      for (unsigned i = 0; i < h; i++)
      {
        for (unsigned j = 0; j < w; j++)
        {
          float4 v1 = image_1.sample(sampler, LiteMath::float2((float)j/w, (float)i/h));
          float4 v2 = image_2.sample(sampler, LiteMath::float2((float)j/w, (float)i/h));
          sum += (v1.x-v2.x)*(v1.x-v2.x) + (v1.y-v2.y)*(v1.y-v2.y) + (v1.z-v2.z)*(v1.z-v2.z);
        }
      }
      return sum / (3*sz);
    }
  }

  float MAE(const LiteImage::Image2D<float> &image_1, const LiteImage::Image2D<float> &image_2)
  {
    if (image_1.width() == image_2.width() && image_1.height() == image_2.height())
    {
      unsigned sz = image_1.vector().size();
      double sum = 0.0;
      for (int i = 0; i < sz; i++)
      {
        float v1 = image_1.vector()[i];
        float v2 = image_2.vector()[i];
        sum += std::abs(v1-v2);
      }
      return sum / sz;
    }
    else
    {
      LiteImage::Sampler sampler;
      sampler.filter = LiteImage::Sampler::Filter::LINEAR;
      sampler.addressU = LiteImage::Sampler::AddressMode::CLAMP;
      sampler.addressV = LiteImage::Sampler::AddressMode::CLAMP;
      unsigned w = std::max(image_1.width(), image_2.width());
      unsigned h = std::max(image_1.height(), image_2.height());
      unsigned sz = w * h;
      double sum = 0.0;
      for (unsigned i = 0; i < h; i++)
      {
        for (unsigned j = 0; j < w; j++)
        {
          float v1 = image_1.sample(sampler, LiteMath::float2((float)j/w, (float)i/h)).x;
          float v2 = image_2.sample(sampler, LiteMath::float2((float)j/w, (float)i/h)).x;
          sum += std::abs(v1-v2);
        }
      }
      return sum / sz;
    }
  }

  float MAE(const LiteImage::Image2D<float4> &image_1, const LiteImage::Image2D<float4> &image_2)
  {
    if (image_1.width() == image_2.width() && image_1.height() == image_2.height())
    {
      unsigned sz = image_1.vector().size();
      double sum = 0.0;
      for (int i = 0; i < sz; i++)
      {
        float4 v1 = image_1.vector()[i];
        float4 v2 = image_2.vector()[i];
        sum += std::abs(v1.x-v2.x) + std::abs(v1.y-v2.y) + std::abs(v1.z-v2.z);
      }
      return sum / (3*sz);
    }
    else
    {
      LiteImage::Sampler sampler;
      sampler.filter = LiteImage::Sampler::Filter::LINEAR;
      sampler.addressU = LiteImage::Sampler::AddressMode::CLAMP;
      sampler.addressV = LiteImage::Sampler::AddressMode::CLAMP;
      unsigned w = std::max(image_1.width(), image_2.width());
      unsigned h = std::max(image_1.height(), image_2.height());
      unsigned sz = w * h;
      double sum = 0.0;
      for (unsigned i = 0; i < h; i++)
      {
        for (unsigned j = 0; j < w; j++)
        {
          float4 v1 = image_1.sample(sampler, LiteMath::float2((float)j/w, (float)i/h));
          float4 v2 = image_2.sample(sampler, LiteMath::float2((float)j/w, (float)i/h));
          sum += std::abs(v1.x-v2.x) + std::abs(v1.y-v2.y) + std::abs(v1.z-v2.z);
        }
      }
      return sum / (3*sz);
    }
  }

  float PSNR(const LiteImage::Image2D<float> &image_1, const LiteImage::Image2D<float> &image_2)
  {
    float mse = MSE(image_1, image_2);
    return -10 * log10(std::max<double>(1e-10, mse));
  }

  float PSNR(const LiteImage::Image2D<float4> &image_1, const LiteImage::Image2D<float4> &image_2)
  {
    float mse = MSE(image_1, image_2);
    return -10 * log10(std::max<double>(1e-10, mse));
  }

  float PSNR(const LiteImage::Image2D<uint32_t> &image_1, const LiteImage::Image2D<uint32_t> &image_2)
  {
    float mse = MSE(image_1, image_2);
    return -10 * log10(std::max<double>(1e-10, mse));
  }

  float SSIM(const LiteImage::Image2D<uint32_t> &image_1, const LiteImage::Image2D<uint32_t> &image_2, int win_size, float data_range)
  {
    assert(image_1.width() == image_2.width());
    assert(image_1.height() == image_2.height());
    unsigned sz = image_1.vector().size();

    LiteImage::Image2D<float4> image_1_f(image_1.width(), image_1.height(), float4(0, 0, 0, 0));
    LiteImage::Image2D<float4> image_2_f(image_1.width(), image_1.height(), float4(0, 0, 0, 0));

    for (int i = 0; i < sz; i++)
    {
      image_1_f.data()[i] = decode_RGBA8(image_1.vector()[i]);
      image_2_f.data()[i] = decode_RGBA8(image_2.vector()[i]);
    }

    return SSIM(image_1_f, image_2_f, win_size, data_range);
  }

  float SSIM(const LiteImage::Image2D<float4> &image_1, const LiteImage::Image2D<float4> &image_2, int win_size, float data_range)
  {
    return SSIM(to_grayscale(image_1), to_grayscale(image_2), win_size, data_range);
  }

  float SSIM(const LiteImage::Image2D<float> &image_1, const LiteImage::Image2D<float> &image_2, int win_size, float data_range)
  {
    assert(image_1.width() == image_2.width());
    assert(image_1.height() == image_2.height());
    assert(image_1.width() >= win_size);
    assert(image_1.height() >= win_size);
    assert(data_range > 1.0f);
    assert(win_size > 1);

    constexpr float k1 = 0.01f;
    constexpr float k2 = 0.03f;
    float c1 = (k1 * data_range) * (k1 * data_range);
    float c2 = (k2 * data_range) * (k2 * data_range);
    unsigned h = image_1.height();
    unsigned w = image_1.width();

    std::vector<float> similarities(omp_get_max_threads(), 0);
    std::vector<float> counts(omp_get_max_threads(), 0);

    #pragma omp parallel for
    for (int y = 0; y < h-win_size+1; y++)
    {
      for (int x = 0; x < w-win_size+1; x++)
      {
        double av_1 = 0;
        double av_2 = 0;

        for (int i = 0; i < win_size; i++)
        {
          for (int j = 0; j < win_size; j++)
          {
            av_1 += image_1.data()[(y + i)*w + x + j];
            av_2 += image_2.data()[(y + i)*w + x + j];
          }
        }

        av_1 /= win_size * win_size;
        av_2 /= win_size * win_size;
        
        double var_1 = 0;
        double var_2 = 0;
        double cov = 0;
        for (int i = 0; i < win_size; i++)
        {
          for (int j = 0; j < win_size; j++)
          {
            float a = (image_1.data()[(y + i)*w + x + j] - av_1);
            float b = (image_2.data()[(y + i)*w + x + j] - av_2);
            var_1 += a * a;
            var_2 += b * b;
            cov += a * b;
          }
        }

        var_1 /= win_size * win_size - 1;
        var_2 /= win_size * win_size - 1;
        cov /= win_size * win_size - 1;

        float ssim = (2 * av_1 * av_2 + c1) * (2 * cov + c2) / ((av_1 * av_1 + av_2 * av_2 + c1) * (var_1 + var_2 + c2));
        similarities[omp_get_thread_num()] += ssim;
        counts[omp_get_thread_num()]++;
      }
    }

    float ssim = 0;
    for (int i = 0; i < omp_get_max_threads(); i++)
      ssim += similarities[i] / counts[i];
    return ssim / omp_get_max_threads();
  }

  float FLIP(const LiteImage::Image2D<uint32_t> &image_1, const LiteImage::Image2D<uint32_t> &image_2)
  {
    assert(image_1.width() == image_2.width());
    assert(image_1.height() == image_2.height());

    FLIP::image<FLIP::color3> referenceImageInput(FLIP::int3{(int)image_1.width(), (int)image_1.height(), 1});
    FLIP::image<FLIP::color3> testImageInput(FLIP::int3{(int)image_1.width(), (int)image_1.height(), 1});
    FLIP::image<float> errorMapFLIPOutput(FLIP::int3{(int)image_1.width(), (int)image_1.height(), 1});

    for (int i = 0; i < image_1.width(); i++)
    {
      for (int j = 0; j < image_1.height(); j++)
      {
        float4 col1 = decode_RGBA8(image_1.data()[j*image_1.width() + i]);
        float4 col2 = decode_RGBA8(image_2.data()[j*image_1.width() + i]);
        referenceImageInput.set(i, j, FLIP::color3{col1.x, col1.y, col1.z});
        testImageInput.set(i, j, FLIP::color3{col2.x, col2.y, col2.z});
      }
    }

    FLIP::Parameters parameters;
    FLIP::evaluate(referenceImageInput, testImageInput, false, parameters, errorMapFLIPOutput);

    double average_flip = 0;

    for (int i = 0; i < image_1.width(); i++)
      for (int j = 0; j < image_1.height(); j++)
        average_flip += errorMapFLIPOutput.get(i, j);

    average_flip /= image_1.width() * image_1.height();
    return average_flip;
  }

  float FLIP(const LiteImage::Image2D<float4> &image_1, const LiteImage::Image2D<float4> &image_2)
  {
    assert(image_1.width() == image_2.width());
    assert(image_1.height() == image_2.height());

    FLIP::image<FLIP::color3> referenceImageInput(FLIP::int3{(int)image_1.width(), (int)image_1.height(), 1});
    FLIP::image<FLIP::color3> testImageInput(FLIP::int3{(int)image_1.width(), (int)image_1.height(), 1});
    FLIP::image<float> errorMapFLIPOutput(FLIP::int3{(int)image_1.width(), (int)image_1.height(), 1});

    for (int i = 0; i < image_1.width(); i++)
    {
      for (int j = 0; j < image_1.height(); j++)
      {
        float4 col1 = image_1.data()[j*image_1.width() + i];
        float4 col2 = image_2.data()[j*image_1.width() + i];
        referenceImageInput.set(i, j, FLIP::color3{col1.x, col1.y, col1.z});
        testImageInput.set(i, j, FLIP::color3{col2.x, col2.y, col2.z});
      }
    }

    FLIP::Parameters parameters;
    FLIP::evaluate(referenceImageInput, testImageInput, false, parameters, errorMapFLIPOutput);

    double average_flip = 0;
    
    for (int i = 0; i < image_1.width(); i++)
      for (int j = 0; j < image_1.height(); j++)
        average_flip += errorMapFLIPOutput.get(i, j);

    average_flip /= image_1.width() * image_1.height();
    return average_flip;
  }
}