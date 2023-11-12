#pragma once

#include <glm.hpp>
#include <iostream>

struct LTC {
    float m_amplitude;//don't know what's for
    float m_fresnel;
    float m11, m22, m13, m23;
    glm::vec3 X, Y, Z;
    glm::mat3 M;
    glm::mat3 invM;
    float detM;

    LTC()
        :m_amplitude(1),m_fresnel(1),
        m11(1), m22(1), m13(0), m23(0),
        X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1)
    {
        Update();
    };
    void Update();
    float Eval(const glm::vec3& L) const;
    glm::vec3 Sample(const float u1, const float u2) const;
};