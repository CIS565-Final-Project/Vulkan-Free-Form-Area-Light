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
		if (info.albedoTex.length() > 0)
		{
			m_AlbedoTex.LoadFromFile(info.albedoTex);
		}
		if (info.normalTex.length() > 0)
		{
			m_NormalTex.LoadFromFile(info.normalTex);
		}
		if (info.roughnessTex.length() > 0)
		{
			m_RoughnessTex.LoadFromFile(info.roughnessTex);
		}
		if (info.metallicTex.length() > 0)
		{
			m_MetallicTex.LoadFromFile(info.metallicTex);
		}
	}
}