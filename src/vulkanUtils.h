#pragma once

#include "common.h"

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
}