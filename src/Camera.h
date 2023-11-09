
#pragma once

#include "instance.h"

#include <glm.hpp>
#include <vulkan/vulkan.hpp>

struct CameraBufferObject {
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;
};

class Camera {
private:

    VK_Renderer::VK_Instance* m_instance;
    
    CameraBufferObject cameraBufferObject;
    
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;

    void* mappedData;

    float r, theta, phi;

public:
    Camera(VK_Renderer::VK_Instance* instance, float aspectRatio);
    ~Camera();

    VkBuffer GetBuffer() const;
    
    void UpdateOrbit(float deltaX, float deltaY, float deltaZ);
};
