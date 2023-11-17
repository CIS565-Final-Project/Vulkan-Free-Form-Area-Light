#include "image.h"

#include <stb_image.h>

namespace VK_Renderer
{
	Image::~Image()
	{
		if (m_RawData) free(m_RawData);
		m_RawData = nullptr;
	}

	void Image::LoadFromFile(std::string const& file)
	{
		m_RawData = stbi_load(file.c_str(), 
								&m_Resolution.x, &m_Resolution.y, &m_Resolution.z, 
								STBI_rgb_alpha);

		if (!m_RawData) {
			throw std::runtime_error("Failed to load texture image");
		}
	}
}