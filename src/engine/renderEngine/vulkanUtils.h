#pragma once

#include <vulkan/vulkan.hpp>

#ifdef GLOBAL_USE_DOUBLE
#define vk_FormatFloat	vk::Format::eR64Sfloat
#define vk_FormatFloat2	vk::Format::eR64G64Sfloat
#define vk_FormatFloat3	vk::Format::eR64G64B64Sfloat
#define vk_FormatFloat4	vk::Format::eR64G64B64A64Sfloat
#else
#define vk_FormatFloat	vk::Format::eR32Sfloat				
#define vk_FormatFloat2	vk::Format::eR32G32Sfloat	
#define vk_FormatFloat3	vk::Format::eR32G32B32Sfloat
#define vk_FormatFloat4	vk::Format::eR32G32B32A32Sfloat
#endif

namespace VK_Renderer
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> computeFamily;
		std::optional<uint32_t> memTransferFamily;

		bool Valid() const {
			return graphicsFamily.has_value() &&
				presentFamily.has_value() &&
				computeFamily.has_value() &&
				memTransferFamily.has_value();
		}

		uint32_t GraphicsIdx() const { return graphicsFamily.value(); }
		uint32_t PresentIdx() const { return presentFamily.value(); }
		uint32_t ComputeIdx() const { return computeFamily.value(); }
		uint32_t MemTransferIdx() const { return memTransferFamily.value(); }
	};

	struct SwapchainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR cpabilities;
		std::vector<vk::SurfaceFormatKHR> surfaceFormats;
		std::vector<vk::PresentModeKHR> presentModes;
	};
}