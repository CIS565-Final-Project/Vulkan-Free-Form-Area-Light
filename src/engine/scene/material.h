#pragma once

#include "image.h"

namespace VK_Renderer
{
	struct MaterialInfo
	{
		std::string albedoTex{ "" };
		std::string normalTex{ "" };
		std::string roughnessTex{ "" };
		std::string metallicTex{ "" };
	};

	class Material
	{
	public:
		Material(MaterialInfo const& info);
		~Material();

		void Free();
		void Load(MaterialInfo const& info);

	protected:
		DeclareWithGetFunc(protected, Image, m, AlbedoTex, const);
		DeclareWithGetFunc(protected, Image, m, NormalTex, const);
		DeclareWithGetFunc(protected, Image, m, RoughnessTex, const);
		DeclareWithGetFunc(protected, Image, m, MetallicTex, const);
	};
}