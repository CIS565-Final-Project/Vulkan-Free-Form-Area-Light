#include "scene.h"

#include "mesh.h"

namespace VK_Renderer
{
	Scene::Scene()
	{
	
	}

	Scene::~Scene()
	{

	}

	void Scene::AddMesh(uPtr<Mesh>& mesh)
	{
		m_Meshes.emplace_back(std::move(mesh));
	}

	void Scene::AddMesh(const std::string& file)
	{
		m_Meshes.emplace_back(mkU<Mesh>(file));
	}
}
