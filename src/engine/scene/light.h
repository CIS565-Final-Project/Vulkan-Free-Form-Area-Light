#pragma once
//also defined in light shaders
#define MAX_LIGHT_VERTEX 10 
#include "transformation.h"
#include "material.h"
#include "atlasTexture.h"
#include <vector>
#include <array>
namespace VK_Renderer
{
	struct LightInfo {
		glm::vec2 boundUV[4]; // use for texture
		glm::vec4 boundPositions[4]; //use for texture
		glm::vec4 boundSphere;
		glm::vec4 lightVertex[MAX_LIGHT_VERTEX];
		int32_t arraySize;
		int32_t lightType;//0: polygon 1: bezier
		float amplitude;
		float padding;
	};

	enum LIGHT_TYPE {
		POLYGON,
		BEZIER
	};

	struct AreaLightCreateInfo
	{
		LIGHT_TYPE type;
		glm::vec4 boundSphere = glm::vec4(0);
		float amplitude = 1;
		Transformation transform = {};
		std::array<glm::vec3, 4> boundPositions = { glm::vec3(),glm::vec3(),glm::vec3(),glm::vec3() };
		std::array<glm::vec2, 4> boundUV = { glm::vec2(1.f,0.f),glm::vec2(0.f,0.f),glm::vec2(0.f,1.f),glm::vec2(1.f,1.f) };
		std::vector<glm::vec3> lightVertex;
		MaterialInfo lightMaterial;
	};

	class AreaLight {
	private:
		//LightInfo m_LightInfo;
		LIGHT_TYPE m_LightType;
		std::array<glm::vec3, 4> m_BoundaryVertex;
		std::array<glm::vec2, 4> m_BoundaryUV;
		std::vector<glm::vec3> m_LightVertex;
		glm::vec4 m_BoundarySphere;
	public:
		float m_Amplitude;
		Transformation m_Transform;
		MaterialInfo m_LightMaterial;
		AreaLight(AreaLightCreateInfo const & createInfo);
		AreaLight(const std::string& objfile); //only for quad light
		LightInfo GetLightInfo()const;
		friend class SceneLight;
	};

	class SceneLight {
	private:
		std::vector<AreaLight> m_AreaLights;
		std::vector<MaterialInfo> m_MaterialInfos;
	public:
		AreaLight* GetLight(size_t idx);
		void AddLight(const AreaLight& lt);
		inline uint32_t GetLightCount()const { return m_AreaLights.size(); };
		inline uint32_t GetLightTextureArrayLayer()const { return m_MaterialInfos[0].texPath.size();};
		std::vector<LightInfo> GetPackedLightInfo();
		AtlasTexture2D GetLightTexture();
	};
}