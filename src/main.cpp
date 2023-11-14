#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <SDL.h>
#include <SDL_vulkan.h>

#include "Camera.h"
#include "Image.h"
#include "graphicsPipeline.h"
#include "commandPool.h"
#include "commandbuffer.h"

#include "device.h"
#include "swapchain.h"
#include "buffer.h"

#include "scene/mesh.h"
#include "scene/scene.h"

#include "Model.h"
#include "Camera.h"

#include <glm.hpp>

using namespace VK_Renderer;

void CreateGraphicsPipeline(vk::Device vk_device, VK_GraphicsPipeline* pipeline)
{
	// Create required shader stages
	auto vert_shader = ReadFile("shaders/flat.vert.spv");
	auto frag_shader = ReadFile("shaders/flat.frag.spv");

	vk::ShaderModule vert_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = vert_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(vert_shader.data())
		});
	vk::ShaderModule frag_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = frag_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(frag_shader.data())
		});

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages
	{
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eVertex,
			.module = vert_module,
			.pName = "main"
		},
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = frag_module,
			.pName = "main"
		},
	};
	
	pipeline->CreatePipeline(shader_stages);

	vk_device.destroyShaderModule(vert_module);
	vk_device.destroyShaderModule(frag_module);
}

void CreateMeshPipeline(vk::Device vk_device, VK_GraphicsPipeline* pipeline)
{
	// Create required shader stages
	auto task_shader = ReadFile("shaders/mesh_flat.task.spv");
	auto mesh_shader = ReadFile("shaders/mesh_flat.mesh.spv");
	auto frag_shader = ReadFile("shaders/mesh_flat.frag.spv");

	vk::ShaderModule task_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = task_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(task_shader.data())
		});
	vk::ShaderModule mesh_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = mesh_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(mesh_shader.data())
		});
	vk::ShaderModule frag_module = vk_device.createShaderModule(vk::ShaderModuleCreateInfo{
		.codeSize = frag_shader.size(),
		.pCode = reinterpret_cast<const uint32_t*>(frag_shader.data())
		});

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages
	{
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eTaskEXT,
			.module = task_module,
			.pName = "main"
		},
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eMeshEXT,
			.module = mesh_module,
			.pName = "main"
		},
		vk::PipelineShaderStageCreateInfo{
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = frag_module,
			.pName = "main"
		},
	};

	pipeline->CreatePipeline(shader_stages);

	vk_device.destroyShaderModule(task_module);
	vk_device.destroyShaderModule(mesh_module);
	vk_device.destroyShaderModule(frag_module);
}

uPtr<VK_Device> CreateLogicalDevice(const VK_Instance* instance)
{
	vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2;
	
	vk::PhysicalDeviceMeshShaderFeaturesEXT meshShaderFeature;

	physicalDeviceFeatures2.pNext = &meshShaderFeature;

	std::vector<const char*> extensions{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_MESH_SHADER_EXTENSION_NAME
	};

	instance->vk_PhysicalDevice.getFeatures2(&physicalDeviceFeatures2);

	return mkU<VK_Device>(instance->vk_PhysicalDevice,
							extensions,
							physicalDeviceFeatures2, 
							instance->m_QueueFamilyIndices);
}

int main(int argc, char* argv[])
{
	// Load necessary functions before any function call
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
	
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);


	SDL_Window* window = SDL_CreateWindow(
		"Vulkan Hello Triangle",
		200, 200, 680, 680, window_flags
	);

#ifndef NDEBUG
	VK_Instance::CheckAvailableExtensions();
#endif

	// Create Vulkan Instance
	uPtr<VK_Instance> instance = mkU<VK_Instance>(window, "Vulkan Hello Triangle");

	// Create Vulkan Surface
	VkSurfaceKHR surface;
	if (SDL_Vulkan_CreateSurface(window, instance->vk_Instance, &surface) != SDL_TRUE)
	{
		throw std::runtime_error("SDL_Vulkan_CreateSurface Failed!");
	}
	instance->vk_Surface = surface;

	// Select a suitable Physical Device from avaiable physical devices (graphics cards)
	instance->PickPhysicalDeivce();

	// Create Vulkan Logical devices with desired extensions
	uPtr<VK_Device> device = CreateLogicalDevice(instance.get());

	// Create Vulkan Swapchain
	int width, height;
	SDL_GetWindowSize(window, &width, &height);

	uPtr<VK_Swapchain> swapchain = mkU<VK_Swapchain>(*device,
													SwapchainSupportDetails{
														.cpabilities = instance->vk_PhysicalDevice.getSurfaceCapabilitiesKHR(instance->vk_Surface),
														.surfaceFormats = instance->vk_PhysicalDevice.getSurfaceFormatsKHR(instance->vk_Surface),
														.presentModes = instance->vk_PhysicalDevice.getSurfacePresentModesKHR(instance->vk_Surface)
													},
													instance->vk_Surface, 
													instance->m_QueueFamilyIndices, 
													width, height);

	VK_CommandBuffer command_buffers = device->GetGraphicsCommandPool()->AllocateCommandBuffers();

	//VkImage texImage;
	//VkDeviceMemory texImageMemory;
	//Image::FromFile(instance.get(),
	//	command_pool->m_CommandPool,
	//	"images/wall.jpg",
	//	VK_FORMAT_R8G8B8A8_UNORM,
	//	VK_IMAGE_TILING_OPTIMAL,
	//	VK_IMAGE_USAGE_SAMPLED_BIT,
	//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//	texImage,
	//	texImageMemory
	//);

	//std::vector<Model*> models;
	//const float halfWidth = 2.5f;
	//models.emplace_back(new Model(instance.get(), command_pool.get()->m_CommandPool,
	//	{
	//		{ { -halfWidth, halfWidth + 2.0f, -5.0f }, { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f } },
	//		{ { halfWidth, halfWidth + 2.0f, -5.0f }, { 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f } },
	//		{ { halfWidth, -halfWidth + 2.0f, -5.0f }, { 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f } },
	//		{ { -halfWidth, -halfWidth + 2.0f, -5.0f }, { 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f } }
	//	},
	//	{ 0, 1, 2, 2, 3, 0 }
	//));
	//
	//const float halfWidth_1 = 6.0f;
	//const float quadHeight = -2.0f;
	//models.emplace_back(new Model(instance.get(), command_pool.get()->m_CommandPool,
	//	{
	//		{ { -halfWidth_1, quadHeight, halfWidth_1 - 5.0f}, { 1.0f, 1.0f, 0.0f },{ 1.0f, 0.0f } },
	//		{ { halfWidth_1, quadHeight, halfWidth_1 - 5.0f}, { 0.0f, 1.0f, 1.0f },{ 0.0f, 0.0f } },
	//		{ { halfWidth_1, quadHeight, -halfWidth_1 - 5.0f}, { 1.0f, 0.0f, 1.0f },{ 0.0f, 1.0f } },
	//		{ { -halfWidth_1, quadHeight,  -halfWidth_1 - 5.0f}, { 0.0f, 1.0f, 1.0f },{ 1.0f, 1.0f } }
	//	},
	//	// { 0, 1, 2, 2, 3, 0 }
	//	{ 0, 2, 1, 3, 2, 0 }
	//));

	//Camera* camera = new Camera(instance.get(), width / height);

	
	//models[0]->SetTexture(texImage);
	//models[1]->SetTexture(texImage);

	vk::Device vk_device = device->GetDevice();

	VK_Buffer buffer(*device);
	std::vector<float> test_data(10);

	buffer.CreateFromData(test_data.data(), sizeof(float) * test_data.size(), vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive);
	buffer.Free();

	std::vector<Model*> m;

	//uPtr<VK_GraphicsPipeline> graphics_pipeline = mkU<VK_GraphicsPipeline>(logicalDevice, 
	//																		extent,
	//																		static_cast<VkFormat>(swapchain->vk_ImageFormat),
	//																		m, nullptr);
	//CreateGraphicsPipeline(vk_device, graphics_pipeline.get());
	

	// mesh pipeline
	
	uPtr<VK_GraphicsPipeline> mesh_pipeline = mkU<VK_GraphicsPipeline>(*device,
																	  swapchain->vk_ImageExtent,
																	  swapchain->vk_ImageFormat);

	CreateMeshPipeline(vk_device, mesh_pipeline.get());

	swapchain->CreateFramebuffers(mesh_pipeline->vk_RenderPass);
	
	uint32_t image_index;
	vk::ClearValue clear_color{};
	clear_color.setColor({ {{0.0f, 0.0f, 0.0f, 1.0f}} });

	// Create synchronization objects
	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	vk::Semaphore image_available_semaphore = device->GetDevice().createSemaphore(vk::SemaphoreCreateInfo{});
	vk::Semaphore render_finished_semaphore = device->GetDevice().createSemaphore(vk::SemaphoreCreateInfo{});

	vk::Fence fence = device->GetDevice().createFence(vk::FenceCreateInfo{
		.flags = vk::FenceCreateFlagBits::eSignaled
	});

	// Main Loop
	SDL_Event e;
	bool bQuit = false;

	while (!bQuit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			//close the window when user alt-f4s or clicks the X button			
			if (e.type == SDL_QUIT) bQuit = true;

			// DrawFrame
			// Wait for previous frame
			device->GetDevice().waitForFences(fence, vk::True, UINT64_MAX);
			device->GetDevice().resetFences(fence);

			vk::ResultValue result = device->GetDevice().acquireNextImageKHR(swapchain->vk_Swapchain, UINT64_MAX, image_available_semaphore, nullptr);
			if (result.result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Fail to acquire next image KHR");
			}
			image_index = result.value;
			command_buffers.Reset();
			
			// Record Command Buffer
			{
				command_buffers.Begin(vk::CommandBufferUsageFlagBits::eRenderPassContinue);

				command_buffers[0].beginRenderPass(vk::RenderPassBeginInfo{
					.renderPass = mesh_pipeline->vk_RenderPass,
					.framebuffer = swapchain->vk_Framebuffers[image_index],
					.renderArea = vk::Rect2D{
						.offset = {0, 0},
						.extent = swapchain->vk_ImageExtent
					},
					.clearValueCount = 1,
					.pClearValues = &clear_color,
				}, vk::SubpassContents::eInline);

				command_buffers[0].bindPipeline(vk::PipelineBindPoint::eGraphics, mesh_pipeline->vk_Pipeline);

				// Viewport and scissors
				command_buffers[0].setViewport(0, vk::Viewport{
					.x = 0.f,
					.y = 0.f,
					.width = static_cast<float>(swapchain->vk_ImageExtent.width),
					.height = static_cast<float>(swapchain->vk_ImageExtent.height),
					.minDepth = 0.f,
					.maxDepth = 1.f
				});

				command_buffers[0].setScissor(0, vk::Rect2D{
					.offset = { 0, 0 },
					.extent = swapchain->vk_ImageExtent
				});

				// Draw call
				uint32_t num_workgroups_x = 2;
				uint32_t num_workgroups_y = 1;
				uint32_t num_workgroups_z = 1;

				command_buffers[0].drawMeshTasksEXT(num_workgroups_x, num_workgroups_y, num_workgroups_z);

				//vkCmdDraw(command_buffer->m_CommandBuffer, 3, 1, 0, 0);
				
				//vkCmdBindDescriptorSets(command_buffer->m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->m_PipelineLayout, 0, 1, &graphics_pipeline->cameraDescriptorSet, 0, nullptr);

				//for (uint32_t j = 0; j < models.size(); ++j) {
				//	// Bind the vertex and index buffers
				//	VkBuffer vertexBuffers[] = { models[j]->getVertexBuffer() };
				//	VkDeviceSize offsets[] = { 0 };
				//	vkCmdBindVertexBuffers(command_buffer->m_CommandBuffer, 0, 1, vertexBuffers, offsets);
				//
				//	vkCmdBindIndexBuffer(command_buffer->m_CommandBuffer, models[j]->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
				//
				//	// Bind the descriptor set for each model
				//	vkCmdBindDescriptorSets(command_buffer->m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->m_PipelineLayout, 1, 1, &graphics_pipeline->modelDescriptorSets[j], 0, nullptr);
				//
				//	// Draw
				//	std::vector<uint32_t> indices = models[j]->getIndices();
				//	vk::CommandBuffer commandBuffer = command_buffer->m_CommandBuffer;
				//	commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
				//	//vkCmdDrawIndexed(command_buffer->m_CommandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
				//}

				command_buffers[0].endRenderPass();
				command_buffers.End();
			}

			// Submit Command Buffer
			std::array<vk::Semaphore, 1> wait_semaphore{ image_available_semaphore };
			std::array<vk::Semaphore, 1> signal_semaphores{ render_finished_semaphore };

			std::array<vk::PipelineStageFlags, 1> wait_stages{ vk::PipelineStageFlagBits::eColorAttachmentOutput };

			device->GetGraphicsQueue().submit(vk::SubmitInfo{
				.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size()),
				.pWaitSemaphores = wait_semaphore.data(),
				.pWaitDstStageMask = wait_stages.data(),
				.commandBufferCount = 1,
				.pCommandBuffers = &command_buffers[0],
				.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size()),
				.pSignalSemaphores = signal_semaphores.data()
			}, fence);

			std::array<vk::SwapchainKHR, 1> swapchains{swapchain->vk_Swapchain};
			device->GetPresentQueue().presentKHR(vk::PresentInfoKHR{
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = signal_semaphores.data(),
				.swapchainCount = static_cast<uint32_t>(swapchains.size()),
				.pSwapchains = swapchains.data(),
				.pImageIndices = &image_index
			});
		}
	}

	device->GetDevice().waitForFences(fence, vk::True, UINT64_MAX);
	device->GetDevice().resetFences(fence);

	//vk_device.destroyShaderModule(vert_module);
	//vk_device.destroyShaderModule(frag_module);

	//vkDestroyImage(logicalDevice, texImage, nullptr);
	//vkFreeMemory(logicalDevice, texImageMemory, nullptr);
	device->GetDevice().destroyFence(fence);
	device->GetDevice().destroySemaphore(image_available_semaphore);
	device->GetDevice().destroySemaphore(render_finished_semaphore);

	//for (int i = 0; i < models.size(); i++) {
	//	delete models[i];
	//}
	//delete camera;

	device->GetGraphicsCommandPool()->FreeCommandBuffer(command_buffers);

	//graphics_pipeline.reset();
	mesh_pipeline.reset();
	swapchain.reset();
	device.reset();
	instance.reset();

	SDL_DestroyWindowSurface(window);
	SDL_DestroyWindow(window);
	
	//system("pause");

	return 0;
}