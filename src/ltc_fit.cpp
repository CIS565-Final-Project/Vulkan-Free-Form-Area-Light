#include "ltc_fit.h"
#include "nelder_mead.h"//如果直接放到ltc_fit.h 那么这个cpp会编译一次nelder_mead.h include ltc_fit.h的main也会编译一次
#include <iostream>
#define DEBUG 0
// size of precomputed table (theta, alpha)
const int N = 64;
// number of samples used to compute the error during fitting
const int Nsample = 50;
// minimal roughness (avoid singularities)
const float MIN_ALPHA = 0.0001f;
namespace LTCFit {
	void LTCFitter::Update(const float* params)
	{
		float m11 = std::max(params[0], MIN_ALPHA);
		float m22 = std::max(params[1], MIN_ALPHA);
		float m13 = params[2];
		float m23 = params[3];
		if (m_isotropic) {
			m_ltc.m11 = m11;
			m_ltc.m22 = m11;
			m_ltc.m13 = 0.f;
			m_ltc.m23 = 0.f;
		}
		else {
			m_ltc.m11 = m11;
			m_ltc.m22 = m22;
			m_ltc.m13 = m13;
			m_ltc.m23 = m23;
		}
		m_ltc.Update();
	}
	float LTCFitter::operator()(const float* params)
	{
		Update(params);
		return ComputeError(m_ltc, m_brdf, m_view, m_alpha);
	}

	float ComputeError(const LTC& ltc, const BRDF& brdf, const glm::vec3& V, const float alpha)
	{
		double error = 0.0;

		for (int j = 0; j < Nsample; ++j)
			for (int i = 0; i < Nsample; ++i)
			{
				const float U1 = (i + 0.5f) / (float)Nsample;
				const float U2 = (j + 0.5f) / (float)Nsample;

				// importance sample LTC
				{
					// sample
					const glm::vec3 L = ltc.Sample(U1, U2);

					// error with MIS weight
					float pdf_brdf;
					float eval_brdf = brdf.Eval(V, L, alpha, pdf_brdf);
					float eval_ltc = ltc.Eval(L);
					float pdf_ltc = eval_ltc / ltc.m_amplitude;
					double error_ = fabsf(eval_brdf - eval_ltc);
					error_ = error_ * error_ * error_;
					error += error_ / (pdf_ltc + pdf_brdf);
				}

				// importance sample BRDF
				{
					// sample
					const glm::vec3 L = brdf.Sample(V, alpha, U1, U2);

					// error with MIS weight
					float pdf_brdf;
					float eval_brdf = brdf.Eval(V, L, alpha, pdf_brdf);
					float eval_ltc = ltc.Eval(L);
					float pdf_ltc = eval_ltc / ltc.m_amplitude;
					double error_ = fabsf(eval_brdf - eval_ltc);
					error_ = error_ * error_ * error_;
					error += error_ / (pdf_ltc + pdf_brdf);
				}
			}
		return (float)error / (float)(Nsample * Nsample);
	}
	float ComputeNorm(const BRDF& brdf, const glm::vec3& V, const float alpha)
	{
		float norm = 0.0;

		for (int j = 0; j < Nsample; ++j)
			for (int i = 0; i < Nsample; ++i)
			{
				const float U1 = (i + 0.5f) / (float)Nsample;
				const float U2 = (j + 0.5f) / (float)Nsample;

				// sample
				const glm::vec3 L = brdf.Sample(V, alpha, U1, U2);

				// eval
				float pdf;
				float eval = brdf.Eval(V, L, alpha, pdf);

				// accumulate
				norm += (pdf > 0) ? eval / pdf : 0.0f;
			}

		return norm / (float)(Nsample * Nsample);
	}
	glm::vec3 ComputeAverageDir(const BRDF& brdf, const glm::vec3& V, const float alpha)
	{
		glm::vec3 averageDir(0);

		for (int j = 0; j < Nsample; ++j)
			for (int i = 0; i < Nsample; ++i)
			{
				const float U1 = (i + 0.5f) / (float)Nsample;
				const float U2 = (j + 0.5f) / (float)Nsample;

				// sample
				const glm::vec3 L = brdf.Sample(V, alpha, U1, U2);

				// eval
				float pdf;
				float eval = brdf.Eval(V, L, alpha, pdf);

				// accumulate
				averageDir += (pdf > 0) ? eval / pdf * L : glm::vec3(0);
			}

		// clear y component, which should be zero with isotropic BRDFs
		averageDir.y = 0.0f;

		return normalize(averageDir);
	}
	void GenerateTexture()
	{
		BRDF brdf;
		glm::mat3* tab = new glm::mat3[N * N];
		glm::vec2* tab_amp = new glm::vec2[N * N];
		std::cout << "start" << std::endl;
		LTC ltc;
		for (int a = N - 1; a >= 0; --a) {
			for (int t = 0; t <= N - 1; ++t) {
				float theta = std::min(1.57f, t / (float)(N - 1) * 1.57079f);
				const glm::vec3 V(sinf(theta), 0.f, cosf(theta));

				float roughness = a / (float)(N - 1);
				float alpha = std::max(roughness * roughness, MIN_ALPHA);
#if DEBUG
				std::cout << "a = " << a << "\t t = " << t << std::endl;
				std::cout << "alpha = " << alpha << "\t theta = " << theta << std::endl;
				std::cout << std::endl;
#endif
				ltc.m_amplitude = ComputeNorm(brdf, V, alpha);
				const glm::vec3 avg_dir = ComputeAverageDir(brdf, V, alpha);
				bool isotropic;

				if (t == 0) {
					ltc.X = glm::vec3(1, 0, 0);
					ltc.Y = glm::vec3(0, 1, 0);
					ltc.Z = glm::vec3(0, 0, 1);

					if (a == N - 1) {
						ltc.m11 = 1.f;
						ltc.m22 = 1.f;
					}
					else {
						ltc.m11 = std::max(tab[a + 1 + t * N][0][0], MIN_ALPHA);
						ltc.m22 = std::max(tab[a + 1 + t * N][1][1], MIN_ALPHA);
					}

					ltc.m13 = 0;
					ltc.m23 = 0;
					ltc.Update();
					isotropic = true;
				}
				else {
					glm::vec3 L = glm::normalize(avg_dir);
					glm::vec3 T1(L.z, 0, -L.x);
					glm::vec3 T2(0, 1, 0);
					//???
					ltc.X = T1;
					ltc.Y = T2;
					ltc.Z = L;

					ltc.Update();
					isotropic = false;
				}

				float epsilon = 0.05f;
				// refine first guess by exploring parameter space
				{
					float startFit[4] = { ltc.m11, ltc.m22, ltc.m13, ltc.m23 };
					float resultFit[4];

					LTCFitter fitter(ltc, brdf, V, alpha, isotropic);

					// Find best-fit LTC lobe (scale, alphax, alphay)
					float error = nelder_mead::NelderMead<4>(resultFit, startFit, epsilon, 1e-5f, 100, fitter);

					// Update LTC with best fitting values
					fitter.Update(resultFit);
				}

				int cur_idx = a + t * N;
				tab[cur_idx] = ltc.M;
				tab_amp[cur_idx][0] = ltc.m_amplitude;
				tab_amp[cur_idx][1] = 0;

				tab[cur_idx][0][1] = 0;
				tab[cur_idx][1][0] = 0;
				tab[cur_idx][2][1] = 0;
				tab[cur_idx][1][2] = 0;
				tab[cur_idx] = 1.f / tab[cur_idx][2][2] * tab[cur_idx];
#if DEBUG
				std::cout << tab[cur_idx][0][0] << "\t " << tab[cur_idx][1][0] << "\t " << tab[cur_idx][2][0] << std::endl;
				std::cout << tab[cur_idx][0][1] << "\t " << tab[cur_idx][1][1] << "\t " << tab[cur_idx][2][1] << std::endl;
				std::cout << tab[cur_idx][0][2] << "\t " << tab[cur_idx][1][2] << "\t " << tab[cur_idx][2][2] << std::endl;
				std::cout << std::endl;
#endif
			}
			std::cout << "ltc progress: " << (N - a) << "/" << N << std::endl;
		}
		std::cout << "end" << std::endl;
		delete[] tab;
		delete[] tab_amp;
	}
}
