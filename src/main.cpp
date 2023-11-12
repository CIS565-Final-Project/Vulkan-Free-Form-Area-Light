#include <iostream>

#include <string>

#include <SDL.h>
#include <SDL_vulkan.h>

#include "Camera.h"
#include "Image.h"
#include "graphicsPipeline.h"
#include "commandPool.h"
#include "commandbuffer.h"

#include <glm.hpp>
#include "ltc_fit.h"


using namespace VK_Renderer;

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);


	SDL_Window* window = SDL_CreateWindow(
		"Vulkan Hello Triangle",
		200, 200, 680, 680, window_flags
	);

#ifndef NDEBUG
	VK_Instance::CheckAvailableExtensions();
#endif

	uPtr<VK_Instance> instance = mkU<VK_Instance>(window, "Vulkan Hello Triangle");

	if (SDL_Vulkan_CreateSurface(window, instance->m_Instance, &instance->m_Surface) != SDL_TRUE)
	{
		throw std::runtime_error("SDL_Vulkan_CreateSurface Failed!");
	}
	
	instance->PickPhysicalDeivce();
	instance->CreateLogicDevice();

	int width, height;
	SDL_GetWindowSize(window, &width, &height);

	instance->CreateSwapchain(width, height);
	instance->CreateImageViews();

	uPtr<VK_CommandPool> command_pool = mkU<VK_CommandPool>(instance->m_LogicalDevice, instance->m_QueueFamilyIndices.GraphicsValue());
	uPtr<VK_CommandBuffer> command_buffer = mkU<VK_CommandBuffer>(instance->m_LogicalDevice, command_pool->m_CommandPool);


	VkImage texImage;
	VkDeviceMemory texImageMemory;
	Image::FromFile(instance.get(),
		command_pool->m_CommandPool,
		// "images/wall.jpg",
		"images/ltc.dds",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		texImage,
		texImageMemory
	);

	std::vector<Model*> lightModels;
	const float halfWidth = 2.5f;
	lightModels.emplace_back(new Model(instance.get(), command_pool.get()->m_CommandPool,
		{
			{ { -halfWidth, halfWidth + 2.0f, -5.0f }, { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { halfWidth, halfWidth + 2.0f, -5.0f }, { 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { halfWidth, -halfWidth + 2.0f, -5.0f }, { 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { -halfWidth, -halfWidth + 2.0f, -5.0f }, { 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
		},
		{ 0, 1, 2, 2, 3, 0 }
	));
	
	std::vector<Model*> shadingModels;
	const float halfWidth_1 = 6.0f;
	const float quadHeight = -2.0f;
	shadingModels.emplace_back(new Model(instance.get(), command_pool.get()->m_CommandPool,
		{
			{ { -halfWidth_1, quadHeight, halfWidth_1 - 5.0f}, { 1.0f, 1.0f, 0.0f },{ 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ { halfWidth_1, quadHeight, halfWidth_1 - 5.0f}, { 0.0f, 1.0f, 1.0f },{ 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ { halfWidth_1, quadHeight, -halfWidth_1 - 5.0f}, { 1.0f, 0.0f, 1.0f },{ 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
			{ { -halfWidth_1, quadHeight,  -halfWidth_1 - 5.0f}, { 0.0f, 1.0f, 1.0f },{ 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }
		},
		// { 0, 1, 2, 2, 3, 0 }
		{ 0, 2, 1, 3, 2, 0 }
	));

	Camera* camera = new Camera(instance.get(), width / height);

	
	lightModels[0]->SetTexture(texImage);
	shadingModels[0]->SetTexture(texImage);

	uPtr<VK_GraphicsPipeline> graphics_pipeline = mkU<VK_GraphicsPipeline>(instance->m_LogicalDevice, 
																	instance->m_SwapchainExtent, 
																	instance->m_SwapchainImageFormat,
																	shadingModels, lightModels, camera);

	instance->CreateFrameBuffers(graphics_pipeline->m_RenderPass);
	
	uint32_t image_index;
	VkClearValue clear_color = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

	// Create synchronization objects
	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
	VkFence fence;

	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(instance->m_LogicalDevice, &semaphore_create_info, nullptr, &image_available_semaphore) != VK_SUCCESS ||
		vkCreateSemaphore(instance->m_LogicalDevice, &semaphore_create_info, nullptr, &render_finished_semaphore) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to Create Semaphore!");
	}

	VkFenceCreateInfo fence_create_info = {};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	if (vkCreateFence(instance->m_LogicalDevice, &fence_create_info, nullptr, &fence) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to Create Fence!");
	}
	
	// Main Loop
	SDL_Event e;
	bool bQuit = false;

	VkDevice device = instance->m_LogicalDevice;

	while (!bQuit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			//close the window when user alt-f4s or clicks the X button			
			if (e.type == SDL_QUIT) bQuit = true;

			// DrawFrame
			// Wait for previous frame
			vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
			vkResetFences(device, 1, &fence);

			vkAcquireNextImageKHR(device, instance->m_Swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);
			vkResetCommandBuffer(command_buffer->m_CommandBuffer, 0);

			// Record Command Buffer
			{
				command_buffer->BeginRecordBuffer();

				VkRenderPassBeginInfo render_pass_begin_info = {};
				render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				render_pass_begin_info.renderPass = graphics_pipeline->m_RenderPass;
				render_pass_begin_info.framebuffer = instance->m_SwapchainFramebuffers[image_index];
				render_pass_begin_info.clearValueCount = 1;
				render_pass_begin_info.pClearValues = &clear_color;
				render_pass_begin_info.renderArea.offset = { 0, 0 };
				render_pass_begin_info.renderArea.extent = instance->m_SwapchainExtent;

				vkCmdBeginRenderPass(command_buffer->m_CommandBuffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

				// Viewport and scissors
				VkViewport viewport = {};
				viewport.x = 0.0f;
				viewport.y = 0.0f;

				viewport.width = static_cast<float>(instance->m_SwapchainExtent.width);
				viewport.height = static_cast<float>(instance->m_SwapchainExtent.height);
				viewport.minDepth = 0.f;
				viewport.maxDepth = 1.f;

				vkCmdSetViewport(command_buffer->m_CommandBuffer, 0, 1, &viewport);

				VkRect2D scissor{};
				scissor.offset = { 0, 0 };
				scissor.extent = instance->m_SwapchainExtent;

				vkCmdSetScissor(command_buffer->m_CommandBuffer, 0, 1, &scissor);

				// Draw call
				// vkCmdDraw(command_buffer->m_CommandBuffer, 3, 1, 0, 0);

				vkCmdBindPipeline(command_buffer->m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->m_shadingPipelineInfo.pipeline);
				
				vkCmdBindDescriptorSets(command_buffer->m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->m_shadingPipelineInfo.pipelineLayout, 0, 1, &graphics_pipeline->cameraDescriptorSet, 0, nullptr);

				for (uint32_t j = 0; j < shadingModels.size(); ++j) {
					// Bind the vertex and index buffers
					VkBuffer vertexBuffers[] = { shadingModels[j]->getVertexBuffer() };
					VkDeviceSize offsets[] = { 0 };
					vkCmdBindVertexBuffers(command_buffer->m_CommandBuffer, 0, 1, vertexBuffers, offsets);

					vkCmdBindIndexBuffer(command_buffer->m_CommandBuffer, shadingModels[j]->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

					// Bind the descriptor set for each model
					vkCmdBindDescriptorSets(command_buffer->m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->m_shadingPipelineInfo.pipelineLayout, 1, 1, &graphics_pipeline->shadingModelDescriptorSets[j], 0, nullptr);

					// Draw
					std::vector<uint32_t> indices = shadingModels[j]->getIndices();
					vkCmdDrawIndexed(command_buffer->m_CommandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
				}

				vkCmdBindPipeline(command_buffer->m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->m_lightPipelineInfo.pipeline);

				vkCmdBindDescriptorSets(command_buffer->m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->m_lightPipelineInfo.pipelineLayout, 0, 1, &graphics_pipeline->cameraDescriptorSet, 0, nullptr);

				for (uint32_t j = 0; j < lightModels.size(); ++j) {
					// Bind the vertex and index buffers
					VkBuffer vertexBuffers[] = { lightModels[j]->getVertexBuffer() };
					VkDeviceSize offsets[] = { 0 };
					vkCmdBindVertexBuffers(command_buffer->m_CommandBuffer, 0, 1, vertexBuffers, offsets);

					vkCmdBindIndexBuffer(command_buffer->m_CommandBuffer, lightModels[j]->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

					// Bind the descriptor set for each model
					vkCmdBindDescriptorSets(command_buffer->m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->m_lightPipelineInfo.pipelineLayout, 1, 1, &graphics_pipeline->lightModelDescriptorSets[j], 0, nullptr);

					// Draw
					std::vector<uint32_t> indices = lightModels[j]->getIndices();
					vkCmdDrawIndexed(command_buffer->m_CommandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
				}

				vkCmdEndRenderPass(command_buffer->m_CommandBuffer);

				command_buffer->EndRecordBuffer();
			}

			// Submit Command Buffer
			VkSubmitInfo submit_info = {};
			submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			std::array<VkSemaphore, 1> wait_semaphore{ image_available_semaphore };
			std::array<VkSemaphore, 1> signal_semaphores{ render_finished_semaphore };

			std::array<VkPipelineStageFlags, 1> wait_stages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size());
			submit_info.pWaitSemaphores = wait_semaphore.data();
			submit_info.pWaitDstStageMask = wait_stages.data();

			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &(command_buffer->m_CommandBuffer);

			submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
			submit_info.pSignalSemaphores = signal_semaphores.data();

			if (vkQueueSubmit(instance->m_GraphicsQueue, 1, &submit_info, fence) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to submit command!");
			}

			VkPresentInfoKHR present_info = {};
			present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			present_info.waitSemaphoreCount = 1;
			present_info.pWaitSemaphores = signal_semaphores.data();

			std::array<VkSwapchainKHR, 1> swapchains{instance->m_Swapchain};
			present_info.swapchainCount = static_cast<uint32_t>(swapchains.size());
			present_info.pSwapchains = swapchains.data();
			present_info.pImageIndices = &image_index;

			vkQueuePresentKHR(instance->m_PresentQueue, &present_info);
		}
	}
	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &fence);

	vkDestroyImage(instance->m_LogicalDevice, texImage, nullptr);
	vkFreeMemory(instance->m_LogicalDevice, texImageMemory, nullptr);

	vkDestroyFence(instance->m_LogicalDevice, fence, nullptr);
	vkDestroySemaphore(instance->m_LogicalDevice, image_available_semaphore, nullptr);
	vkDestroySemaphore(instance->m_LogicalDevice, render_finished_semaphore, nullptr);

	for (int i = 0; i < shadingModels.size(); i++) {
		delete shadingModels[i];
	}
	for (int i = 0; i < lightModels.size(); i++) {
		delete lightModels[i];
	}
	delete camera;

	command_pool.reset();
	graphics_pipeline.reset();
	instance.reset();

	SDL_DestroyWindowSurface(window);
	SDL_DestroyWindow(window);
	
	//LTCFit::GenerateTexture("images/ltc");
	system("pause");
	return 0;
}