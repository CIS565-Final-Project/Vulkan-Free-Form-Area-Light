#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <SDL.h>
#include <SDL_vulkan.h>

#include "Camera.h"
#include "Image.h"
#include "graphicsPipeline.h"
#include "pipelineInput.h"
#include "commandPool.h"
#include "commandbuffer.h"

#include "device.h"
#include "swapchain.h"
#include "buffer.h"
#include "uniform.h"

#include "scene/mesh.h"
#include "scene/scene.h"
#include "scene/perspectiveCamera.h"

#include "Model.h"
#include "Camera.h"

#include <glm.hpp>

using namespace VK_Renderer;

struct CameraUBO 
{
	glm::mat4 viewProjMat;
};

struct MeshletInfo
{
	uint32_t Meshlet_Size;
	uint32_t Triangle_Count;
};

void CreateGraphicsPipeline(vk::Device vk_device, 
							VK_GraphicsPipeline* pipeline, 
							std::vector<vk::DescriptorSetLayout> const& descripotrSetLayouts)
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
	
	VK_GeneralPipeInput input;
	input.SetupPipelineVertexInputCreateInfo();

	pipeline->CreatePipeline(shader_stages, input, descripotrSetLayouts);

	vk_device.destroyShaderModule(vert_module);
	vk_device.destroyShaderModule(frag_module);
}

void CreateMeshPipeline(vk::Device vk_device, 
						VK_GraphicsPipeline* pipeline,
						std::vector<vk::DescriptorSetLayout> const& descripotrSetLayouts)
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

	VK_PipelineInput input;
	input.SetupPipelineVertexInputCreateInfo();

	pipeline->CreatePipeline(shader_stages, input, descripotrSetLayouts);

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

PerspectiveCamera camera = PerspectiveCamera{
	.m_Transform = Transformation{
		.position = {0, 0, -10}
	},
	.resolution = {680, 680}
};

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

	VK_DeviceBuffer buffer(*device);
	std::vector<glm::vec4> test_data={
		glm::vec4(1., -1., 0, 1),
		glm::vec4(1.,  1., 0, 1),
		glm::vec4(-1., -1., 0, 1),
		glm::vec4(-1., -1., 0, 1),
		glm::vec4(1.,  1., 0, 1),
		glm::vec4(-1.,  1., 0, 1)
	};

	buffer.CreateFromData(test_data.data(), 
		sizeof(glm::vec4) * test_data.size(), 
		vk::BufferUsageFlagBits::eStorageBuffer, 
		vk::SharingMode::eExclusive);
	

	std::vector<Model*> m;

	device->CreateDescriptiorPool(1, 10);

	VK_BufferUniform cam_uniform(*device);

	cam_uniform.Create(vk::ShaderStageFlagBits::eMeshEXT, { sizeof(CameraUBO) }, 3);

	VK_StorageBufferUniform storaging_buffer_unifom(*device);
	
	Mesh mesh;
	//mesh.LoadMeshFromFile("meshes/sphere.obj");
	mesh.LoadMeshFromFile("meshes/cube.obj");

	MeshletInfo meshlet_info{
		.Meshlet_Size = 32,
		.Triangle_Count = static_cast<uint32_t>(mesh.m_Triangles.size())
	};

	VK_BufferUniform meshlet_uniform(*device);

	meshlet_uniform.Create(vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT, { sizeof(MeshletInfo) }, 1);
	meshlet_uniform.m_MappedBuffers[0].Update(&meshlet_info, 0, sizeof(MeshletInfo));

	std::vector<glm::ivec4> triangles;

	for (Triangle const& tri : mesh.m_Triangles)
	{
		triangles.emplace_back(tri.pId, tri.materialId);
	}

	storaging_buffer_unifom.Create(vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
		{ 
			sizeof(glm::ivec4)* triangles.size(),
			sizeof(Float) * mesh.m_Positions.size(),
		}, 
	1);

	storaging_buffer_unifom.m_Buffers[0].Update(triangles.data(), 0, sizeof(glm::ivec4)* triangles.size());
	storaging_buffer_unifom.m_Buffers[1].Update(mesh.m_Positions.data(), 0, sizeof(Float)* mesh.m_Positions.size());

	// mesh pipeline	
	uPtr<VK_GraphicsPipeline> mesh_pipeline = mkU<VK_GraphicsPipeline>(*device,
																	  swapchain->vk_ImageExtent,
																	  swapchain->vk_ImageFormat);

	std::vector<vk::DescriptorSetLayout> descriptor_set_layouts{ 
		cam_uniform.vk_DescriptorSetLayout,  
		storaging_buffer_unifom.vk_DescriptorSetLayout,
		meshlet_uniform.vk_DescriptorSetLayout
	};
	CreateMeshPipeline(vk_device, mesh_pipeline.get(), descriptor_set_layouts);

	swapchain->CreateFramebuffers(mesh_pipeline->vk_RenderPass);

	uint32_t image_index;
	vk::ClearValue clear_color{};
	clear_color.setColor({ {{0.0f, 0.0f, 0.0f, 1.0f}} });

	// Create synchronization objects
	vk::Semaphore image_available_semaphore = device->GetDevice().createSemaphore(vk::SemaphoreCreateInfo{});
	vk::Semaphore render_finished_semaphore = device->GetDevice().createSemaphore(vk::SemaphoreCreateInfo{});

	vk::Fence fence = device->GetDevice().createFence(vk::FenceCreateInfo{
		.flags = vk::FenceCreateFlagBits::eSignaled
	});

	// Main Loop
	SDL_Event e;
	bool bQuit = false;
	
	glm::ivec2 mouse_pre;

	while (!bQuit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			//close the window when user alt-f4s or clicks the X button			
			if (e.type == SDL_QUIT) bQuit = true;
			if (e.type == SDL_MOUSEMOTION)
			{
				const Uint8* state = SDL_GetKeyboardState(nullptr);
				glm::ivec2 mouse_cur;
				SDL_GetMouseState(&mouse_cur.x, &mouse_cur.y);
				Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);
				if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
				{
					glm::vec2 offset = 0.001f * glm::vec2(mouse_cur - mouse_pre);
					camera.m_Transform.Translate({ -offset.x, offset.y, 0});
					camera.RecomputeProjView();
				}
				if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
				{
					glm::vec2 offset = 0.01f * glm::vec2(mouse_cur - mouse_pre);
					camera.m_Transform.RotateAround(glm::vec3(0.f), { 0.1f * offset.y, -offset.x, 0 });
					camera.RecomputeProjView();
				}
				mouse_pre = mouse_cur;
			}

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
				uint32_t num_workgroups_x = (triangles.size() + meshlet_info.Meshlet_Size - 1) / meshlet_info.Meshlet_Size;
				uint32_t num_workgroups_y = 1;
				uint32_t num_workgroups_z = 1;
				camera.RecomputeProjView();
				glm::mat4 view_proj_mat = camera.GetProjViewMatrix();
				cam_uniform.m_MappedBuffers[image_index].Update(&view_proj_mat, 0, sizeof(glm::mat4));
				
				glm::vec4 v0 = view_proj_mat * glm::vec4( 1., -1., 0, 1.f);
				glm::vec4 v1 = view_proj_mat * glm::vec4( 1.,  1., 0, 1.f);
				glm::vec4 v2 = view_proj_mat * glm::vec4(-1., -1., 0, 1.f);

				vk::ArrayProxy<vk::DescriptorSet> arr{
					cam_uniform.vk_DescriptorSets[image_index],
					storaging_buffer_unifom.vk_DescriptorSets[0],
					meshlet_uniform.vk_DescriptorSets[0]
				};

				command_buffers[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
														mesh_pipeline->vk_PipelineLayout,
														uint32_t(0),
														arr, nullptr);

				command_buffers[0].drawMeshTasksEXT(num_workgroups_x, num_workgroups_y, num_workgroups_z);
				
				//command_buffers[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphics_pipeline->vk_PipelineLayout, 0, 1, );
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
	buffer.Free();
	cam_uniform.Free();
	meshlet_uniform.Free();
	storaging_buffer_unifom.Free();
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