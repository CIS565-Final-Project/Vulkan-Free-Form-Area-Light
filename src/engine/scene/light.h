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
		glm::vec4 lightVertex[MAX_LIGHT_VERTEX];
		int32_t arraySize;
		int32_t lightType;//0: polygon 1: bezier
		glm::vec2 padding;
	};

	enum LIGHT_TYPE {
		POLYGON,
		BEZIER
	};

	struct AreaLightCreateInfo
	{
		LIGHT_TYPE type;
		Transformation transform = {};
	};

	class AreaLight {
	private:
		//LightInfo m_LightInfo;
		LIGHT_TYPE m_LightType;
		std::array<glm::vec3, 4> m_BoundaryVertex;
		std::array<glm::vec2, 4> m_BoundaryUV;
		std::vector<glm::vec3> m_LightVertex;
	public:
		Transformation m_Transform;
		AreaLight(AreaLightCreateInfo const & createInfo, const std::vector<glm::vec3>& lightPts
			, const std::array<glm::vec3, 4>& boundPositions = { glm::vec3(),glm::vec3(),glm::vec3(),glm::vec3() }
			, const std::array<glm::vec2,4>& boundUV = { glm::vec2(1.f,0.f),glm::vec2(0.f,0.f),glm::vec2(0.f,1.f),glm::vec2(1.f,1.f) }
		);
		LightInfo GetLightInfo()const;
		friend class SceneLight;
	};

	class SceneLight {
	private:
		std::vector<AreaLight> m_AreaLights;
		std::vector<MaterialInfo> m_MaterialInfos;
	public:
		AreaLight* GetLight(size_t idx);
		void AddLight(const AreaLight& lt, const MaterialInfo& ltTextureInfo);
		std::vector<LightInfo> GetPackedLightInfo()const;
		inline uint32_t GetLightCount()const { return m_AreaLights.size(); };
		inline uint32_t GetLightTextureArrayLayer()const { return m_MaterialInfos[0].texPath.size();};
		AtlasTexture2D GetLightTexture();
	};
}