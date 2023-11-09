#include "BufferUtils.h"
#include <stdexcept>


void BufferUtils::CreateBuffer(VK_Renderer::VK_Instance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // Create buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(instance->m_LogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer");
    }

    // Query buffer's memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(instance->m_LogicalDevice, buffer, &memRequirements);

    // Allocate memory in device
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = instance->GetMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(instance->m_LogicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate vertex buffer");
    }

    // Associate allocated memory with vertex buffer
    vkBindBufferMemory(instance->m_LogicalDevice, buffer, bufferMemory, 0);
}

void BufferUtils::CopyBuffer(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(instance->m_LogicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(instance->m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(instance->m_GraphicsQueue);
    vkFreeCommandBuffers(instance->m_LogicalDevice, commandPool, 1, &commandBuffer);
}

void BufferUtils::CreateBufferFromData(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, void* bufferData, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // Create the staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferUsageFlags stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags stagingProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    BufferUtils::CreateBuffer(instance, bufferSize, stagingUsage, stagingProperties, stagingBuffer, stagingBufferMemory);

    // Fill the staging buffer
    void *data;
    vkMapMemory(instance->m_LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, bufferData, static_cast<size_t>(bufferSize));
    vkUnmapMemory(instance->m_LogicalDevice, stagingBufferMemory);

    // Create the buffer
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage;
    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    BufferUtils::CreateBuffer( instance, bufferSize, usage, flags, buffer, bufferMemory);

    // Copy data from staging to buffer
    BufferUtils::CopyBuffer(instance, commandPool, stagingBuffer, buffer, bufferSize);

    // No need for the staging buffer anymore
    vkDestroyBuffer(instance->m_LogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(instance->m_LogicalDevice, stagingBufferMemory, nullptr);
}
