#include "atlasTexture.h"

namespace VK_Renderer
{
	AtlasTexture2D::AtlasTexture2D(AtlasTexture2DCreateInfo const& info)
		:m_Channels(info.channels)
	{
	}

	AtlasTexture2D::AtlasTexture2D(std::vector<Material> const& materials,
									AtlasTexture2DCreateInfo const& info)
		:AtlasTexture2D(info)
	{
		ComputeAtlas(materials);
	}

	AtlasTexture2D::~AtlasTexture2D()
	{
		Free();
	}
	
	void AtlasTexture2D::Free()
	{
		m_Size = 0;
		m_Data.clear();
		m_FinishedAtlas.clear();
	}

	struct MaterialProxy
	{
		uint32_t id;
		uint32_t size;
		bool operator <(MaterialProxy const& other) const
		{
			return size > other.size; // in decending order
		}
	};

	void AtlasTexture2D::ComputeAtlas(std::vector<Material> const& materials)
	{
		Free();
		if (materials.size() == 0) return;

		m_FinishedAtlas.resize(materials.size());

		TextureBlock2D init_atlas;

		std::vector<MaterialProxy> sorted_materials;

		for (size_t i = 0; i < materials.size(); ++i)
		{
			glm::ivec3 const& dim = materials[i].GetTextures()[0].GetResolution();
			
			init_atlas.width += dim.x;
			init_atlas.height += dim.y;

			sorted_materials.emplace_back(i, dim.x * dim.y);
		}
		std::sort(sorted_materials.begin(), sorted_materials.end());

		std::set<TextureBlock2D> avaliable_atlas;
		avaliable_atlas.insert(init_atlas);

		for (auto const& material_proxy : sorted_materials)
		{
			glm::ivec3 const& dim = materials[material_proxy.id].GetTextures()[0].GetResolution();
			for (TextureBlock2D const& atlas : avaliable_atlas)
			{
				if (atlas.width >= dim.x && atlas.height >= dim.y)
				{
					// Step 1. Add a new atlas finished list
					m_FinishedAtlas[material_proxy.id] = TextureBlock2D{
						.start = atlas.start,
						.width = static_cast<unsigned int>(dim.x),
						.height = static_cast<unsigned int>(dim.y),
					};
					// compute some necessary data
					glm::ivec2 end = atlas.start + glm::ivec2{ dim.x, dim.y };

					unsigned int w = atlas.width - dim.x;
					unsigned int h = atlas.height - dim.y;

					// Step 2. Add splited atlas into aviable set
					if (w > 0)
					{
						avaliable_atlas.insert(TextureBlock2D{
							.start = atlas.start + glm::ivec2{0, dim.y},
							.width = w,
							.height = static_cast<unsigned int>(dim.y),
							});
					}
					if (h > 0)
					{
						avaliable_atlas.insert(TextureBlock2D{
							.start = atlas.start + glm::ivec2{dim.x, 0},
							.width = static_cast<unsigned int>(dim.x),
							.height = h,
							});
					}
					if (w > 0 && h > 0)
					{
						avaliable_atlas.insert(TextureBlock2D{
							.start = end,
							.width = w,
							.height = h,
							});
					}

					// Step 3. update max extent
					m_Resolution = glm::max(m_Resolution, end);

					// Step 4. remove current atlas from aviable set
					avaliable_atlas.erase(atlas);

					break;
				}
			}
		}

		// copy texture data
		m_Data.resize(m_Resolution.x * m_Resolution.y * m_Channels * materials[0].GetTextures().size());
		
		m_Size = m_Data.size() * sizeof(unsigned char);

		for (size_t i = 0; i < m_FinishedAtlas.size(); ++i)
		{
			TextureBlock2D const& atlas = m_FinishedAtlas[i];
			Material const& material = materials[i];

			uint32_t start = (atlas.start.y * (m_Resolution.x) + atlas.start.x) * m_Channels;
			uint32_t size = atlas.width * m_Channels * sizeof(unsigned char);

			for (size_t k = 0; k < material.GetTextures().size(); ++k)
			{
				uint64_t layer_offset = k * m_Resolution.x * m_Resolution.y * m_Channels;
				Image const& image = material.GetTextures()[k];
				if (image.GetSize() > 0)
				{
					unsigned char const* data = reinterpret_cast<unsigned char*>(image.GetRawData());
					for (uint32_t h = 0; h < atlas.height; ++h)
					{
						uint32_t copy_start = start + h * m_Resolution.x * m_Channels;
						std::memcpy(m_Data.data() + layer_offset + copy_start, data + h * size, size);
					}
				}
				
			}
		}
	}
}