#include "light.h"
namespace VK_Renderer 
{
	AreaLight::AreaLight(LIGHT_TYPE type, const std::vector<glm::vec3>& lightPts
		, const std::array<glm::vec3, 4>& boundPositions
		, const std::array<glm::vec2, 4>& boundUV)
	{
		m_LightInfo.lightType = static_cast<int32_t>(type);
		m_LightInfo.arraySize = lightPts.size();
		assert(lightPts.size() <= MAX_LIGHT_VERTEX);
		for (int i = 0;i < 4;++i) {
			m_LightInfo.boundUV[i] = boundUV[i];
			m_LightInfo.boundPositions[i] = glm::vec4(boundPositions[i], 1.0);
		}
		for (int i = 0;i < lightPts.size();++i) {
			m_LightInfo.lightVertex[i] = glm::vec4(lightPts[i], 1.0);
		}
	}

	void AreaLight::UpdateLightInfo()
	{
		glm::mat4 model = m_Transform.GetTransformation();
		for (auto& ltVert : m_LightInfo.boundPositions) {
			ltVert = model * ltVert;
		}
		for (auto& ltVert : m_LightInfo.lightVertex) {
			ltVert = model * ltVert;
		}
	}
	void AreaLight::Rotate(float const& angle, glm::vec3 const& axis)
	{
		m_Transform.Rotate(angle, axis);
		UpdateLightInfo();
	}
	void AreaLight::SetRotation(const glm::quat& rot)
	{
		m_Transform.rotation = rot;
		UpdateLightInfo();
	}
	void AreaLight::Translate(glm::vec3 const& translate)
	{
		m_Transform.Translate(translate);
		UpdateLightInfo();
	}
	void AreaLight::SetPosition(const glm::vec3& pos)
	{
		m_Transform.position = pos;
		UpdateLightInfo();
	}
	LightInfo AreaLight::getLightInfo() const
	{
		return m_LightInfo;
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
			ans[i] = m_AreaLights[i].getLightInfo();
		}
		return ans;
	}
}