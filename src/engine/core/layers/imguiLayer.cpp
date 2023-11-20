#include "imguiLayer.h"

#include "renderEngine/renderEngine.h"
#include "renderEngine/commandbuffer.h"

using namespace VK_Renderer;

namespace MyCore
{
	ImGuiLayer::ImGuiLayer(std::string const& name, 
							SDL_Window* window,
							VK_Renderer::VK_RenderEngine& renderEngine)
		:Layer(name), 
		 sdl_Window(window), 
		 m_RenderEngine(renderEngine)
	{
	}
	ImGuiLayer::~ImGuiLayer()
	{
	}
	void ImGuiLayer::OnAttach()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		// Do not enable Multi-Viewport!
		
		imgui_IO = &io;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		ImGui_ImplSDL2_InitForVulkan(sdl_Window);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = m_RenderEngine.GetInstance()->vk_Instance;
		init_info.PhysicalDevice = m_RenderEngine.GetDevice()->GetPhysicalDevice();
		init_info.Device = m_RenderEngine.GetDevice()->GetDevice();
		init_info.QueueFamily = m_RenderEngine.GetInstance()->m_QueueFamilyIndices.GraphicsIdx();
		init_info.Queue = m_RenderEngine.GetDevice()->GetGraphicsQueue();
		init_info.DescriptorPool = m_RenderEngine.GetDevice()->vk_DescriptorPool;
		init_info.Subpass = 0;
		init_info.MinImageCount = 2;
		init_info.ImageCount = m_RenderEngine.GetSwapchain()->vk_Framebuffers.size();
		init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(m_RenderEngine.GetDevice()->GetDeviceProperties().maxSampleCount);

		ImGui_ImplVulkan_Init(&init_info, m_RenderEngine.GetRenderPass()->GetRenderPass());

		m_Cmd = mkU<VK_CommandBuffer>(m_RenderEngine.GetDevice()->GetGraphicsCommandPool()->AllocateCommandBuffers(
			{
				.count = static_cast<uint32_t>(m_RenderEngine.GetSwapchain()->vk_Framebuffers.size()),
				.level = vk::CommandBufferLevel::eSecondary
			}
		));
	}

	void ImGuiLayer::OnDetech()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}
	void ImGuiLayer::OnUpdate(double const& deltaTime)
	{
		static bool pushed = false;
		if (pushed) return;
		pushed = true;
		for (int idx = 0; idx < m_Cmd->Size(); ++idx)
		{
			m_RenderEngine.PushSecondaryCommand((*m_Cmd)[idx], idx);
		}
	}
	void ImGuiLayer::OnRender(double const& deltaTime)
	{
		uint32_t const& image_idx = m_RenderEngine.GetSwapchain()->GetImageIdx();
		
		// Rendering
		ImDrawData* main_draw_data = ImGui::GetDrawData();
		const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);

		VK_CommandBuffer& cmd = *m_Cmd;
		cmd.Reset(image_idx);
		{
			cmd.Begin({ .usage = vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
				.inheritInfo = {
					.renderPass = m_RenderEngine.GetRenderPass()->GetRenderPass(),
					.subpass = 0}
				}, image_idx);
		
			ImGui_ImplVulkan_RenderDrawData(main_draw_data, (*m_Cmd)[image_idx]);
		
			cmd.End(image_idx);
		}
	}
	void ImGuiLayer::OnImGui(double const& deltaTime)
	{
		ImGui::ShowDemoWindow();
	}
	bool ImGuiLayer::HandleEvents(SDL_Event const& e)
	{
		ImGui_ImplSDL2_ProcessEvent(&e);
		return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered();
	}
	void ImGuiLayer::BeginFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}
	void ImGuiLayer::EndFrame()
	{
		ImGui::Render();

		if (imgui_IO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}
}