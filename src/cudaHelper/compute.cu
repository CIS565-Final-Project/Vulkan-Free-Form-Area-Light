#include "gaussian.cuh"

#include <iostream>
#include <cuda_runtime.h>
#include <thrust/device_vector.h>

#include <device_launch_parameters.h>

namespace CUDA_Helper
{
	__global__ void GaussianKernal(int maxCount, int width, int height, uchar4* out_image, uchar4* in_image, float* kernel, int const kernelHalfRadius)
	{
		int index = (blockIdx.x * blockDim.x) + threadIdx.x;
		if (index >= maxCount) return;

		int id_x = index % width;
		int id_y = (index - id_x) / width;

		int kernel_radius = kernelHalfRadius * 2 + 1;

		float4 pixel_sum = { 0.f, 0.f, 0.f, 0.f };
		float weight_sum = 0.f;

		for (int ky = -kernelHalfRadius; ky <= kernelHalfRadius; ky++) 
		{
			for (int kx = -kernelHalfRadius; kx <= kernelHalfRadius; kx++) 
			{
				int x = id_x + kx;
				int y = id_y + ky;
				// Boundary check
				if (y >= 0 && y < height && x >= 0 && x < width) 
				{
					uchar4 const& pixel = in_image[y * width + x];
					float weight = kernel[(ky + kernelHalfRadius) * kernel_radius + kx + kernelHalfRadius];

					pixel_sum.x += static_cast<float>(pixel.x) * weight;
					pixel_sum.y += static_cast<float>(pixel.y) * weight;
					pixel_sum.z += static_cast<float>(pixel.z) * weight;
					pixel_sum.w += static_cast<float>(pixel.w) * weight;

					weight_sum += weight;
				}
			}
		}

		uchar4 result;
		result.x = static_cast<unsigned char>(pixel_sum.x / weight_sum);
		result.y = static_cast<unsigned char>(pixel_sum.y / weight_sum);
		result.z = static_cast<unsigned char>(pixel_sum.z / weight_sum);
		result.w = static_cast<unsigned char>(pixel_sum.w / weight_sum);
		out_image[index] = result;
	}

	void GaussianBlur(void* const out_image, void const* const input_image, int const& width, int const& height, int const& kernelHalfRadius, float* kernel)
	{
		int max_count = width * height;
		int num_block = (max_count + 127) / 128;
		
		uchar4 const* in = reinterpret_cast<uchar4 const*>(input_image);
		uchar4* out = reinterpret_cast<uchar4*>(out_image);

		thrust::device_vector<uchar4> thrust_dev_in(in, in + max_count);
		thrust::device_vector<uchar4> thrust_dev_out(out, out + max_count);

		uint16_t kernel_radius = (kernelHalfRadius << 1) + 1;
		uint16_t kernel_size = kernel_radius * kernel_radius;
		thrust::device_vector<float> thrust_dev_kernel(kernel, kernel + kernel_size);

		GaussianKernal << <num_block, 128 >> > (max_count, width, height, 
			thrust_dev_out.data().get(), 
			thrust_dev_in.data().get(), 
			thrust_dev_kernel.data().get(), 
			kernelHalfRadius);

		cudaMemcpy(out_image, thrust_dev_out.data().get(), max_count * sizeof(uchar4), cudaMemcpyDeviceToHost);
	}
}