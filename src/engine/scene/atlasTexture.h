#pragma once

#include "material.h"

namespace VK_Renderer
{
	struct TextureBlock2D
	{
		glm::ivec2 start{ 0, 0 };

		uint32_t width{ 0 };
		uint32_t height{ 0 };
		uint32_t id{ 0 };

		bool operator < (TextureBlock2D const& other) const
		{
			return (start.x != other.start.x ? start.x < other.start.x :
				start.y < other.start.y);
		}
	};

	struct AtlasTexture2DCreateInfo
	{
		uint8_t channels{ 4 };
	};

	class AtlasTexture2D
	{
	public:
		AtlasTexture2D(AtlasTexture2DCreateInfo const& info = {});
		AtlasTexture2D(std::vector<Material> const& materials, 
						AtlasTexture2DCreateInfo const& info = {});
		~AtlasTexture2D();

		void Free();
		void ComputeAtlas(std::vector<Material> const& materials);

	protected:
		DeclareWithGetSetFunc(protected, uint8_t, m, Channels, const);
		DeclareWithGetFunc(protected, uint64_t, m, Size, const);
		DeclareWithGetFunc(protected, glm::ivec2, m, Resolution, const);
		DeclareWithGetFunc(protected, std::vector<unsigned char>, m, Data, const);
		DeclareWithGetFunc(protected, std::vector<TextureBlock2D>, m, FinishedAtlas, const);
	};
}