#pragma once

#include "instance.h"
#include <vulkan/vulkan.h>

namespace Image {

    void Create(VK_Renderer::VK_Instance* instance, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void TransitionLayout(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    VkImageView CreateView(VK_Renderer::VK_Instance* instance, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void CopyFromBuffer(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, VkBuffer buffer, VkImage& image, uint32_t width, uint32_t height);
    void FromFile(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, const char* path, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageLayout layout, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
}
