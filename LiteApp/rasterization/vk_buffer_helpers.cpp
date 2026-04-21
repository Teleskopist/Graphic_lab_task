#include "vk_buffer_helpers.h"
#include "vkutils/vk_utils.h"

BufferWrapper::~BufferWrapper()
{
    free();
}

BufferWrapper::BufferWrapper(VkDevice _device, VkPhysicalDevice _physicalDevice, size_t _size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits memoryProperties)
    : device(_device), size(_size)
{
    VkMemoryRequirements memReq;
    buffer = vk_utils::createBuffer(device, size, usage, &memReq);
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.allocationSize = memReq.size;
    allocateInfo.memoryTypeIndex = vk_utils::findMemoryType(memReq.memoryTypeBits, memoryProperties, _physicalDevice);
    VK_CHECK_RESULT(vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory));

    VK_CHECK_RESULT(vkBindBufferMemory(device, buffer, bufferMemory, 0));
}

void BufferWrapper::free()
{
    if (buffer)
    {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, bufferMemory, nullptr);
    }
    device = nullptr;
    buffer = nullptr;
    bufferMemory = nullptr;
    size = 0;
}

void* BufferWrapper::map()
{   
    void * ptr= nullptr;
    vkMapMemory(device, bufferMemory, 0,  size, 0, &ptr);
    return ptr;
}

void BufferWrapper::unmap()
{
    vkUnmapMemory(device, bufferMemory);
}
