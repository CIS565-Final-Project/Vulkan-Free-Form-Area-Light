#include "Model.h"
#include "BufferUtils.h"
#include "Image.h"

Model::Model(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
  : m_instance(instance), vertices(vertices), indices(indices) {

    if (vertices.size() > 0) {
        BufferUtils::CreateBufferFromData(instance, commandPool, this->vertices.data(), vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, vertexBufferMemory);
    }

    if (indices.size() > 0) {
        BufferUtils::CreateBufferFromData(instance, commandPool, this->indices.data(), indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer, indexBufferMemory);
    }

    modelBufferObject.modelMatrix = glm::mat4(1.0f);
    BufferUtils::CreateBufferFromData(instance, commandPool, &modelBufferObject, sizeof(ModelBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, modelBuffer, modelBufferMemory);
}

Model::~Model() {
    if (indices.size() > 0) {
        vkDestroyBuffer(m_instance->m_LogicalDevice, indexBuffer, nullptr);
        vkFreeMemory(m_instance->m_LogicalDevice, indexBufferMemory, nullptr);
    }

    if (vertices.size() > 0) {
        vkDestroyBuffer(m_instance->m_LogicalDevice, vertexBuffer, nullptr);
        vkFreeMemory(m_instance->m_LogicalDevice, vertexBufferMemory, nullptr);
    }

    vkDestroyBuffer(m_instance->m_LogicalDevice, modelBuffer, nullptr);
    vkFreeMemory(m_instance->m_LogicalDevice, modelBufferMemory, nullptr);

    if (textureView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_instance->m_LogicalDevice, textureView, nullptr);
    }

    if (textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_instance->m_LogicalDevice, textureSampler, nullptr);
    }
}

void Model::SetTexture(VkImage texture) {

    this->texture = texture;
    this->textureView = Image::CreateView(m_instance, texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    //this->textureView = Image::CreateView(m_instance, texture, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

    // --- Specify all filters and transformations ---
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // Interpolation of texels that are magnified or minified
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    // Addressing mode
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // Anisotropic filtering
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;

    // Border color
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    // Choose coordinate system for addressing texels --> [0, 1) here
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    // Comparison function used for filtering operations
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    // Mipmapping
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_instance->m_LogicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler");
    }
    
}

const std::vector<Vertex>& Model::getVertices() const {
    return vertices;
}

VkBuffer Model::getVertexBuffer() const {
    return vertexBuffer;
}

const std::vector<uint32_t>& Model::getIndices() const {
    return indices;
}

VkBuffer Model::getIndexBuffer() const {
    return indexBuffer;
}

const ModelBufferObject& Model::getModelBufferObject() const {
    return modelBufferObject;
}

VkBuffer Model::GetModelBuffer() const {
    return modelBuffer;
}

VkImageView Model::GetTextureView() const {
    return textureView;
}

VkSampler Model::GetTextureSampler() const {
    return textureSampler;
}
