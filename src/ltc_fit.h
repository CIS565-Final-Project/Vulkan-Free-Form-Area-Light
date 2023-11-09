#pragma once
#include "ltc.h"
#include "brdf.h"

namespace LTCFit {
	float ComputeError(const LTC& ltc, const BRDF& brdf, const glm::vec3& V, const float alpha);
	float ComputeNorm(const BRDF& brdf, const glm::vec3& V, const float alpha);
	glm::vec3 ComputeAverageDir(const BRDF& brdf, const glm::vec3& V, const float alpha);
	struct LTCFitter {
		LTC& m_ltc;
		const BRDF& m_brdf;
		const glm::vec3 m_view;
		const float m_alpha;
		const bool m_isotropic;
		LTCFitter(LTC& _ltc, const BRDF& _brdf, const glm::vec3& V, const float _alpha, const bool _isotropic = false)
			:m_ltc(_ltc), m_brdf(_brdf), m_view(V), m_alpha(_alpha), m_isotropic(_isotropic) {};
		void Update(const float* params);
		float operator()(const float* params);
	};
	void WriteToTextures(const std::string& filename, const glm::mat3* tab, const glm::vec2* tab_amp, const int N);
	void GenerateTexture(const std::string& baseFilename);
}
