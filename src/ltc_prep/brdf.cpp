#include "brdf.h"

float BRDF::Eval(const glm::vec3& V, const glm::vec3& L, const float alpha, float& pdf) const
{
    if (V.z <= 0)
    {
        pdf = 0;
        return 0;
    }

    auto lambda = [](const float _alpha, const float _cosTheta) {
        const float a = 1.0f / _alpha / tanf(acosf(_cosTheta));
        return (_cosTheta < 1.0f) ? 0.5f * (-1.0f + sqrtf(1.0f + 1.0f / a / a)) : 0.0f;
    };

    // masking
    const float LambdaV = lambda(alpha, V.z);

    // shadowing
    float G2;
    if (L.z <= 0.0f)
        G2 = 0;
    else
    {
        const float LambdaL = lambda(alpha, L.z);
        G2 = 1.0f / (1.0f + LambdaV + LambdaL);
    }

    // D
    const glm::vec3 H = glm::normalize(V + L);
    const float slopex = H.x / H.z;
    const float slopey = H.y / H.z;
    float D = 1.0f / (1.0f + (slopex * slopex + slopey * slopey) / alpha / alpha);	//这个slopex*slopex + slopey*slopey其实等于(x^2+y^2)/z^2，结果就是tan(theta) * tan(theta)
    D = D * D;
    D = D / (3.14159f * alpha * alpha * H.z * H.z * H.z * H.z);

    pdf = fabsf(D * H.z / 4.0f / glm::dot(V, H));
    float res = D * G2 / 4.0f / V.z;		//这里实际上算的是BRDF * dot(N, L)，只是被BRDF分母里的dot(N, L)约掉了

    return res;
}

glm::vec3 BRDF::Sample(const glm::vec3& V, const float alpha, const float U1, const float U2) const
{
    const float phi = 2.0f * 3.14159f * U1;
    const float r = alpha * sqrtf(U2 / (1.0f - U2));
    const glm::vec3 N = glm::normalize(glm::vec3(r * cosf(phi), r * sinf(phi), 1.0f));
    const glm::vec3 L = -V + 2.0f * N * glm::dot(N, V);
    return L;
}
