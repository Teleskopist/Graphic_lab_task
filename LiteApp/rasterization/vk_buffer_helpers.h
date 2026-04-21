#pragma once
#include "vkutils/vk_buffers.h"

struct BufferWrapper
{
    BufferWrapper() = default;
    BufferWrapper(VkDevice device, VkPhysicalDevice physicalDevice, size_t size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memoryProperties);
    BufferWrapper(const BufferWrapper &) = delete;
    BufferWrapper(BufferWrapper &&other)
    {
        swap(other);
    }
    BufferWrapper &operator=(const BufferWrapper &) = delete;
    ~BufferWrapper();

    void free();

    void swap(BufferWrapper &other)
    {
        std::swap(device, other.device);
        std::swap(buffer, other.buffer);
        std::swap(bufferMemory, other.bufferMemory);
        std::swap(size, other.size);
    }

    BufferWrapper &operator=(BufferWrapper &&other)
    {
        BufferWrapper tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    void *map();
    void unmap();
    VkDevice device = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    size_t size = 0;
};

template <typename T>
struct UniformBufferWrapper : BufferWrapper
{
    UniformBufferWrapper() = default;
    UniformBufferWrapper(
        VkDevice _device,
        VkPhysicalDevice _physicalDevice,
        VkMemoryPropertyFlagBits memoryProperties = VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        : BufferWrapper(_device, _physicalDevice, sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memoryProperties)
    {
    }
    T *map()
    {
        return static_cast<T *>(BufferWrapper::map());
    }
    void write(const T &value)
    {
        T *ptr = map();
        *ptr = value;
        unmap();
    }
};
