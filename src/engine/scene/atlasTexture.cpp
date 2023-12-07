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

		TextureBlock2D init_blocks;

		std::vector<MaterialProxy> sorted_materials;

		for (size_t i = 0; i < materials.size(); ++i)
		{
			glm::ivec3 const& dim = materials[i].GetTextures()[0].GetResolution();
			
			init_blocks.width += dim.x;
			init_blocks.height += dim.y;

			sorted_materials.emplace_back(i, dim.x * dim.y);
		}
		std::sort(sorted_materials.begin(), sorted_materials.end());

		std::list<TextureBlock2D> avaliable_blocks;
		avaliable_blocks.push_back(init_blocks);
		
		m_Resolution = glm::ivec2(0);

		for (auto const& material_proxy : sorted_materials)
		{
			glm::ivec3 const& dim = materials[material_proxy.id].GetTextures()[0].GetResolution();
			
			std::list<TextureBlock2D>::iterator best_block_it = avaliable_blocks.end();
			auto it = avaliable_blocks.begin();
			glm::ivec2 min_end = m_Resolution + glm::ivec2(dim.x + 1, dim.y + 1);
			for (; it != avaliable_blocks.end(); ++it)
			{
				glm::ivec2 new_end = glm::max(m_Resolution, it->start + glm::ivec2{ dim.x, dim.y });
				
				if (it->width >= dim.x && it->height >= dim.y && 
					(new_end.x * new_end.y < min_end.x * min_end.y))
				{
					min_end = new_end;
					best_block_it = it;
				}
			}
			assert(best_block_it != avaliable_blocks.end());

			// Step 1. Add a new atlas finished list
			m_FinishedAtlas[material_proxy.id] = TextureBlock2D{
				.start = best_block_it->start,
				.width = static_cast<unsigned int>(dim.x),
				.height = static_cast<unsigned int>(dim.y),
			};
			// compute some necessary data
			glm::ivec2 end = best_block_it->start + glm::ivec2{ dim.x, dim.y };

			unsigned int w = best_block_it->width - dim.x;
			unsigned int h = best_block_it->height - dim.y;
			// Step 2. Add splited atlas into aviable set
			if (w > 0)
			{
				avaliable_blocks.push_back(TextureBlock2D{
					.start = best_block_it->start + glm::ivec2{0, dim.y},
					.width = w,
					.height = static_cast<unsigned int>(dim.y),
				});
			}
			if (h > 0)
			{
				avaliable_blocks.push_back(TextureBlock2D{
					.start = best_block_it->start + glm::ivec2{dim.x, 0},
					.width = static_cast<unsigned int>(dim.x),
					.height = h,
				});
			}
			if (w > 0 && h > 0)
			{
				avaliable_blocks.push_back(TextureBlock2D{
					.start = end,
					.width = w,
					.height = h,
				});
			}
			// Step 3. update max extent
			m_Resolution = min_end;

			// Step 4. remove current atlas from aviable set
			avaliable_blocks.erase(best_block_it);
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
					if (image.GetResolution().x == 1 && image.GetResolution().y == 1)
					{
						// special case that texture is a 1x1 image
						unsigned char const* data = reinterpret_cast<unsigned char*>(image.GetRawData());
						for (uint32_t h = 0; h < atlas.height; ++h)
						{
							uint32_t copy_start = start + h * m_Resolution.x * m_Channels;
							for (int pixel = 0; pixel < atlas.width; ++pixel)
							{
								*(m_Data.data() + layer_offset + copy_start + pixel * 3) = data[0];
								*(m_Data.data() + layer_offset + copy_start + pixel * 3 + 1) = data[1];
								*(m_Data.data() + layer_offset + copy_start + pixel * 3 + 2) = data[2];
							}
						}
					}
					else
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
}