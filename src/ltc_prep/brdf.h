#pragma once
#include <glm.hpp>

class BRDF {
public:
	virtual float Eval(const glm::vec3& V, const glm::vec3& L, const float alpha, float& pdf) const;
	virtual glm::vec3 Sample(const glm::vec3& V, const float alpha, const float U1, const float U2) const;
};