#include "light.h"
namespace VK_Renderer 
{
	AreaLight::AreaLight(LIGHT_TYPE type, const std::vector<glm::vec3>& lightPts
		, const std::array<glm::vec3, 4>& boundPositions
		, const std::array<glm::vec2, 4>& boundUV)
		:m_LightType(type), m_LightVertex(lightPts), m_BoundaryVertex(boundPositions), m_BoundaryUV(boundUV)
	{
	}
	LightInfo AreaLight::GetLightInfo() const
	{
		LightInfo res;
		res.lightType = static_cast<int32_t>(m_LightType);
		res.arraySize = m_LightVertex.size();
		assert(res.arraySize <= MAX_LIGHT_VERTEX);
		glm::mat4 modelMatrix = m_Transform.GetTransformation();
		for (int i = 0;i < 4;++i) {
			res.boundUV[i] = m_BoundaryUV[i];
			res.boundPositions[i] = modelMatrix * glm::vec4(m_BoundaryVertex[i], 1.0);
		}
		for (int i = 0;i < m_LightVertex.size();++i) {
			res.lightVertex[i] = modelMatrix * glm::vec4(m_LightVertex[i], 1.0);
		}
		return res;
	}

	//--------------------
	//SceneLight
	//--------------------
	AreaLight* SceneLight::GetLight(size_t idx)
	{
		if (idx > m_AreaLights.size())return nullptr;
		return &m_AreaLights[idx];
	}
	void SceneLight::AddLight(const AreaLight& lt)
	{
		m_AreaLights.emplace_back(lt);
	}
	std::vector<LightInfo> SceneLight::GetPackedLightInfo() const
	{
		auto n = m_AreaLights.size();
		std::vector<LightInfo> ans(n);
		for (auto i = 0;i < n;++i) {
			ans[i] = m_AreaLights[i].GetLightInfo();
		}
		return ans;
	}
}