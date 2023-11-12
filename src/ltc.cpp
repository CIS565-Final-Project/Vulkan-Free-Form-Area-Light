#include "ltc.h"
static constexpr float PI = 3.14159f;
void LTC::Update()
{
    M = glm::mat3(X, Y, Z) * glm::mat3(
        m11, 0, 0,
        0, m22, 0,
        m13, m23, 1
    );
    invM = glm::inverse(M);
    detM = std::abs(glm::determinant(M));
}

float LTC::Eval(const glm::vec3& L) const
{
    glm::vec3 loriginal = glm::normalize(invM * L);
    glm::vec3 L_ = M * loriginal;
    float l = glm::length(L_);
    float jacobian = detM / (l * l * l);
    float D = 1.f / PI  * std::max(0.f, loriginal.z);
    float res = m_amplitude * D / jacobian;
    return res;
}

glm::vec3 LTC::Sample(const float u1, const float u2) const
{
	float theta = acosf(sqrtf(u1));
	float phi = 2 * PI * u2;
	return glm::normalize(M * glm::vec3(
		sinf(theta) * cosf(phi)
		, sinf(theta) * sinf(phi)
		, cosf(theta)
	    )
	);
}
