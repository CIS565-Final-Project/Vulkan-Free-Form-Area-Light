#pragma once

namespace VK_Renderer
{
	class Mesh;

	class Scene
	{
	public:
		Scene();
		~Scene();

		void AddMesh(uPtr<Mesh>& mesh);
		void AddMesh(const std::string& file);

	public:
		std::vector<uPtr<Mesh>> m_Meshes;
	};
};