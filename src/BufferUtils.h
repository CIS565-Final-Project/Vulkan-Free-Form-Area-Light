#pragma once


#include <vulkan/vulkan.h>
#include "instance.h"

namespace BufferUtils {
    void CreateBuffer(VK_Renderer::VK_Instance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CopyBuffer(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void CreateBufferFromData( VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, void* bufferData, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
}
