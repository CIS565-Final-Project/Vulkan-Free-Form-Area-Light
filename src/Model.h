#pragma once

#include <vulkan/vulkan.h>
#include <glm.hpp>
#include <vector>
#include "instance.h"


struct ModelBufferObject {
    glm::mat4 modelMatrix;
};


struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;

    // Get the binding description, which describes the rate to load data from memory
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    // Get the attribute descriptions, which describe how to handle vertex input
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

        // Position
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        // Color
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        // Texture coordinate
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        // Normal
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, normal);

        return attributeDescriptions;
    }
};


class Model {
protected:

    VK_Renderer::VK_Instance* m_instance;

    std::vector<Vertex> vertices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    std::vector<uint32_t> indices;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkBuffer modelBuffer;
    VkDeviceMemory modelBufferMemory;

    ModelBufferObject modelBufferObject;

    VkImage texture = VK_NULL_HANDLE;
    VkImageView textureView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;

public:
    Model() = delete;
    Model(VK_Renderer::VK_Instance* instance, VkCommandPool commandPool, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
    virtual ~Model();

    void SetTexture(VkImage texture);

    const std::vector<Vertex>& getVertices() const;

    VkBuffer getVertexBuffer() const;

    const std::vector<uint32_t>& getIndices() const;

    VkBuffer getIndexBuffer() const;

    const ModelBufferObject& getModelBufferObject() const;

    VkBuffer GetModelBuffer() const;
    VkImageView GetTextureView() const;
    VkSampler GetTextureSampler() const;
};
