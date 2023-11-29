#include "material.h"

namespace VK_Renderer
{
	Material::Material(MaterialInfo const& info)
	{
		Load(info);
	}
	Material::~Material()
	{
	}
	void Material::Free()
	{

	}
	void Material::Load(MaterialInfo const& info)
	{
		m_Textures.resize(info.texPath.size());
		for (int i = 0; i < m_Textures.size(); ++i)
		{
			if(info.texPath[i].size() > 0) m_Textures[i].LoadFromFile(info.texPath[i]);
		}
	}
}