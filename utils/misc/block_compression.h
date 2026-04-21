#pragma once

#ifndef USE_KTX
#error "This file should be included only when USE_KTX is defined (<ktx.h> header is available)"
#endif

#ifndef USE_GPU
#error "This file requires vulkan"
#endif

#define VK_NO_PROTOTYPES
#include <ktx.h>
#include <vulkan/vulkan.h>
#include <cassert>
#include <vector>

#include "RowCopyHelper.h"

#define KTX_CHECK(e)                                             \
    do                                                           \
    {                                                            \
        KTX_error_code e__ = (e);                                \
        if (e__ != KTX_SUCCESS)                                  \
        {                                                        \
            printf(#e "\nKTX error: %s\n", ktxErrorString(e__)); \
            abort();                                             \
        }                                                        \
    } while (0)

namespace block_compression
{

    [[nodiscard]] inline VkFormat compress_image(std::vector<uint8_t> &compressed_data, const uint8_t *data, uint32_t width, uint32_t height, uint32_t channels)
    {
        // For now
        assert(channels == 1);

        uint32_t uncompressed_size = (int)(width * height * channels * sizeof(uint8_t));

        ktxTextureCreateInfo createInfo{};
        createInfo.vkFormat = VK_FORMAT_R8_UNORM;
        createInfo.baseWidth = width;
        createInfo.baseHeight = height;
        createInfo.baseDepth = 1;
        createInfo.numDimensions = 2;
        createInfo.numLevels = 1;
        createInfo.numLayers = 1;
        createInfo.numFaces = 1;
        createInfo.isArray = KTX_FALSE;
        createInfo.generateMipmaps = KTX_FALSE;

        ktxTexture2 *texture = nullptr;
        KTX_CHECK(ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture));
        KTX_CHECK(ktxTexture_SetImageFromMemory(ktxTexture(texture), 0, 0, 0, data, uncompressed_size));

        // Compress to BasisU inside KTX2 (UASTC or ETC1S; choose quality via options)
        ktxBasisParams params{};
        params.structSize = sizeof(params);

        params.uastc = true;
        params.compressionLevel = 2;
        params.qualityLevel = 255;
        params.verbose = true;

        KTX_CHECK(ktxTexture2_CompressBasisEx(texture, &params));
        KTX_CHECK(ktxTexture2_TranscodeBasis(texture, KTX_TTF_BC4_R, KTX_TF_HIGH_QUALITY));

        const uint8_t *ptr = ktxTexture_GetData(ktxTexture(texture));
        size_t size = ktxTexture_GetDataSize(ktxTexture(texture));

        compressed_data.resize(size);
        std::copy(ptr, ptr + size, compressed_data.data());

        assert(texture->vkFormat == VK_FORMAT_BC4_UNORM_BLOCK);

        printf("Compressed image %dx%dx%d from %d bytes to %d bytes (%f%%)\n", (int)width, (int)height, (int)channels, (int)uncompressed_size, (int)size, (float)size / uncompressed_size * 100);

        VkFormat format = (VkFormat)texture->vkFormat;

        ktxTexture_Destroy(ktxTexture(texture));

        return format;
    }

    inline void copy_compressed_image(
        VkImage image,
        VkImageLayout finalLayout,
        const void *data,
        uint32_t width,
        uint32_t height,
        uint32_t channels,
        uint32_t compressed_size,
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkQueue transferQueue,
        uint32_t transferQueueIDX)
    {
        uint32_t staging_size = compressed_size;
        auto copy = std::make_unique<RowCopyHelper>(physicalDevice, device, transferQueue, transferQueueIDX, compressed_size);
        copy->UpdateImageRow(image, data, width, height, compressed_size, finalLayout);
    }

}
