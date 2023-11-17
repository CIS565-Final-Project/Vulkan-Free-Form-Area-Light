#include "image.h"

#include <stb_image.h>
#include <dds.h>
#include <iostream>
namespace VK_Renderer
{
	Image::~Image()
	{
		if (m_RawData) free(m_RawData);
		m_RawData = nullptr;
	}

	void Image::LoadFromFile(std::string const& file)
	{
		size_t postfix_start = file.find_last_of(".");
		std::string file_postfix = file.substr(postfix_start + 1, file.size() - postfix_start);
		std::cout << "load a " << file_postfix << " image" << std::endl;
        if (file_postfix == "dds") {
            DDSImage dds_image = LoadDDS(file.c_str());
            m_Resolution.x = dds_image.width;
            m_Resolution.y = dds_image.height;
            m_Resolution.z = 1;
            m_Size = dds_image.data.size();
            m_RawData = malloc(m_Size * sizeof(uint8_t));
            std::memcpy(m_RawData, dds_image.data.data(), m_Size * sizeof(uint8_t));
        }
        else {
            stbi_set_flip_vertically_on_load(true);
            m_RawData = stbi_load(file.c_str(),
                &m_Resolution.x, &m_Resolution.y, &m_Resolution.z,
                STBI_rgb_alpha);
            m_Size = m_Resolution.x * m_Resolution.y * 4 * sizeof(unsigned char);
            if (!m_RawData) {
                throw std::runtime_error("Failed to load texture image");
            }
            stbi_set_flip_vertically_on_load(false);
        }
	}
}