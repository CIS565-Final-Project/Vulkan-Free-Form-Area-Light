#pragma once

#include "image.h"

namespace VK_Renderer
{
	struct MaterialInfo
	{
		std::vector<std::string> texPath;
	};

	class Material
	{
	public:
		Material(MaterialInfo const& info);
		~Material();

		void Free();
		void Load(MaterialInfo const& info);

	protected:
		DeclareWithGetFunc(protected, std::vector <Image>, m, Textures, const);
	};
}