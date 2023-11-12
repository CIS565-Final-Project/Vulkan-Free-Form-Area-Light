#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Image.h"
#include "BufferUtils.h"
#include <dds.hpp>
#include <string>

void Image::Create(VK_Renderer::VK_Instance* instance, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    // Create Vulkan image
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(instance->m_LogicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image");
    }

    // Allocate memory for the image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(instance->m_LogicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = instance->GetMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(instance->m_LogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory");
    }

    // Bind the image
    vkBindImageMemory(instance->m_LogicalDevice, image, imageMemory, 0);
}

void Image::Create(VK_Renderer::VK_Instance* instance, const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    if (vkCreateImage(instance->m_LogicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image");
    }

    // Allocate memory for the image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(instance->m_LogicalDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = instance->GetMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(instance->m_LogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory");
    }

    // Bind the image
    vkBindImageMemory(instance->m_LogicalDevice, image, imageMemory, 0);
}

void Image::TransitionLayout(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    auto hasStencilComponent = [](VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
  };

    // Use an image memory barrier (type of pipeline barrier) to transition image layout
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
  
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
  
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
  
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
  
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw std::invalid_argument("Unsupported layout transition");
    }

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
  
    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(instance->m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(instance->m_GraphicsQueue);
    vkFreeCommandBuffers(instance->m_LogicalDevice, commandPool, 1, &commandBuffer);
}

VkImageView Image::CreateView(VK_Renderer::VK_Instance* instance, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    // Describe the image's purpose and which part of the image should be accessed
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(instance->m_LogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
      throw std::runtime_error("Failed to texture image view");
    }

    return imageView;
}

VkImageView Image::CreateView(VK_Renderer::VK_Instance* instance, const VkImageViewCreateInfo& viewInfo)
{
    VkImageView imageView;
    if (vkCreateImageView(instance->m_LogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to texture image view");
    }
    return imageView;
}

void Image::CopyFromBuffer(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, VkBuffer buffer, VkImage& image, uint32_t width, uint32_t height) {
    // Specify which part of the buffer is going to be copied to which part of the image
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

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

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(instance->m_PresentQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(instance->m_PresentQueue);
    vkFreeCommandBuffers(instance->m_LogicalDevice, commandPool, 1, &commandBuffer);
}

void Image::FromFile(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, const char* path, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImageLayout layout, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    int texWidth, texHeight, texChannels;
    std::string file_name = path;
    size_t postfix_start = file_name.find_last_of(".");
    std::string file_postfix = file_name.substr(postfix_start + 1, file_name.size() - postfix_start);
    std::cout << "load a " << file_postfix << " image" << std::endl;
    //stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (file_postfix == "dds") {
        dds::Image dds_image;
        dds::readFile(file_name, &dds_image);
        //format = dds::getVulkanFormat(dds_image.format, dds_image.supportsAlpha);
        texWidth = dds_image.width;
        texHeight = dds_image.height;
        texChannels = 4;
        stbi_uc* pixels = dds_image.data.data() + 148;
        VkDeviceSize imageSize = dds_image.data.size() - 148;//data format
        //format = dds::getVulkanFormat(dds_image.format, dds_image.supportsAlpha);
        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        VkBufferUsageFlags stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkMemoryPropertyFlags stagingProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        BufferUtils::CreateBuffer(instance, imageSize, stagingUsage, stagingProperties, stagingBuffer, stagingBufferMemory);

        // Copy pixel values to the buffer
        void* data;
        vkMapMemory(instance->m_LogicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(instance->m_LogicalDevice, stagingBufferMemory);


        // Create Vulkan image
        //Image::Create(instance, texWidth, texHeight, format, tiling, VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage, properties, image, imageMemory);
        auto info = dds::getVulkanImageCreateInfo(&dds_image);
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | usage;
        auto viewInfo = dds::getVulkanImageViewCreateInfo(&dds_image);
        Image::Create(instance, info, properties, image, imageMemory);

        // Copy the staging buffer to the texture image
        // --> First need to transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        Image::TransitionLayout(instance, commandPool, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        Image::CopyFromBuffer(instance, commandPool, stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        // Transition texture image for shader access
        Image::TransitionLayout(instance, commandPool, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout);

        // No need for staging buffer anymore
        vkDestroyBuffer(instance->m_LogicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(instance->m_LogicalDevice, stagingBufferMemory, nullptr);
    }
    else {
        stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("Failed to load texture image");
        }

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        VkBufferUsageFlags stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkMemoryPropertyFlags stagingProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        BufferUtils::CreateBuffer(instance, imageSize, stagingUsage, stagingProperties, stagingBuffer, stagingBufferMemory);

        // Copy pixel values to the buffer
        void* data;
        vkMapMemory(instance->m_LogicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(instance->m_LogicalDevice, stagingBufferMemory);

        // Free pixel array
        stbi_image_free(pixels);

        // Create Vulkan image
        Image::Create(instance, texWidth, texHeight, format, tiling, VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage, properties, image, imageMemory);

        // Copy the staging buffer to the texture image
        // --> First need to transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        Image::TransitionLayout(instance, commandPool, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        Image::CopyFromBuffer(instance, commandPool, stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        // Transition texture image for shader access
        Image::TransitionLayout(instance, commandPool, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout);

        // No need for staging buffer anymore
        vkDestroyBuffer(instance->m_LogicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(instance->m_LogicalDevice, stagingBufferMemory, nullptr);
    }
    



}