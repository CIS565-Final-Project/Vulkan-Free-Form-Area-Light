#pragma once
#include <vulkan/vulkan.h>
#include <span>
#include <dds.hpp>
unsigned const DDS_FORMAT_R32G32B32A32_FLOAT = 2;
unsigned const DDS_FORMAT_R32G32_FLOAT       = 16;
unsigned const DDS_FORMAT_R16G16_FLOAT       = 34;
unsigned const DDS_FORMAT_R32_FLOAT          = 41;

struct DDSImage {
    uint32_t numMips;
    uint32_t arraySize = 1;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    dds::ResourceDimension dimension;
    bool supportsAlpha = false;
    std::vector<uint8_t> data = {};
    std::vector<dds::span<uint8_t>> mipmaps;
    VkFormat format;
    VkImageCreateInfo getVulkanImageCreateInfo(VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT);
    VkImageViewCreateInfo getVulkanImageViewCreateInfo();
};

bool SaveDDS( char const* path, unsigned format, unsigned texelSizeInBytes, unsigned width, unsigned height, void const* data );
DDSImage LoadDDS(char const* path);
