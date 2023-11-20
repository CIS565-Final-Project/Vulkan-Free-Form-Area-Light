#pragma once

#include "core/layer.h"
#include <vulkan/vulkan.hpp>

using namespace MyCore;

struct MeshletInfo;

#include "renderEngine/renderEngine.h"
#include "renderEngine/instance.h"
#include "renderEngine/graphicsPipeline.h"
#include "renderEngine/pipelineInput.h"
#include "renderEngine/commandPool.h"
#include "renderEngine/commandbuffer.h"
#include "renderEngine/device.h"
#include "renderEngine/swapchain.h"
#include "renderEngine/buffer.h"
#include "renderEngine/texture.h"
#include "renderEngine/descriptor.h"

#include "scene/mesh.h"
#include "scene/image.h"
#include "scene/scene.h"
#include "scene/perspectiveCamera.h"

class RenderLayer : public Layer
{
public:
	RenderLayer(std::string const& name, 
				std::string const&& meshFile, 
				std::string const&& textureFile);

public:
	virtual void OnAttach() override;
	virtual void OnDetech() override;

	virtual void OnUpdate(double const& deltaTime);
	virtual void OnRender(double const& deltaTime);
	virtual void OnImGui(double const& deltaTime);

	virtual void OnEvent(SDL_Event const&);

	void RecordCmd();

protected:
	VK_Renderer::VK_RenderEngine* m_Engine;
	VK_Renderer::VK_Device const* m_Device;
	VK_Renderer::VK_Swapchain const* m_Swapchain;

	uPtr<VK_Renderer::PerspectiveCamera> m_Camera;
	uPtr<VK_Renderer::VK_CommandBuffer> m_Cmd;
	
	uPtr<VK_Renderer::VK_Texture2D> m_Texture;

	uPtr<VK_Renderer::VK_PipelineInput> m_PipelineInput;
	uPtr<VK_Renderer::VK_GraphicsPipeline> m_MeshShaderPipeline;

	uPtr<VK_Renderer::VK_StagingBuffer> m_CamBuffer;
	uPtr<VK_Renderer::VK_DeviceBuffer> m_MeshletInfoBuffer;
	uPtr<VK_Renderer::VK_DeviceBuffer> m_TriangleBuffer;
	uPtr<VK_Renderer::VK_DeviceBuffer> m_PositionBuffer;
	uPtr<VK_Renderer::VK_DeviceBuffer> m_NormalBuffer;
	uPtr<VK_Renderer::VK_DeviceBuffer> m_UVBuffer;

	uPtr<VK_Renderer::VK_Descriptor> m_CamDescriptor;
	uPtr<VK_Renderer::VK_Descriptor> m_MeshShaderInputDescriptor;

	uPtr<MeshletInfo> m_MeshletInfo;

	std::string m_MeshFile;
	std::string m_TextureFile;
};