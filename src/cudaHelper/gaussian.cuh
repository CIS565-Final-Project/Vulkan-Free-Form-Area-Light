#pragma once

namespace CUDA_Helper
{
	void GaussianBlur(void* const out_image, void const* const input_image, int const& width, int const& height, int const& kernelHalfRadius, float* kernel);
}