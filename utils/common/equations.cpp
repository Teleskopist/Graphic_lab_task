#include "equations.h"
#include <complex>
#include "LiteMath.h"

namespace equations
{
    using R = float;
    using P = std::vector<R>;
    using C = std::complex<R>;

    static C int_pow(C z, size_t n)
    {
        C out = 1;
        for (size_t i = 0; i < n; i++)
            out *= z;
        return out;
    }

    static C evaluate_polynomial(const P &p, C z)
    {
        C out = 0;
        for (size_t i = 0; i < p.size(); i++)
            out += int_pow(z, i) * p[i];
        return out;
    }

    static C evaluate_polynomial_derivative(const P &p, C z)
    {
        C out = 0;
        for (size_t i = 1; i < p.size(); i++)
            out += int_pow(z, i - 1) * (R)i * p[i];
        return out;
    }

    static C evaluate_start_polynomial(size_t n, C z)
    {
        return int_pow(z, n) - (R)1;
    }

    static C evaluate_start_polynomial_derivative(size_t n, C z)
    {
        return int_pow(z, n - 1) * (R)n;
    }

    static C evaluate_homotopy(const P &p, C z, R t, C gamma)
    {
        size_t n = p.size() - 1;
        return (1 - t) * evaluate_start_polynomial(n, z) * gamma + t * evaluate_polynomial(p, z);
    }

    static C evaluate_homotopy_partial_x(const P &p, C z, R t, C gamma)
    {
        size_t n = p.size() - 1;
        return (1 - t) * evaluate_start_polynomial_derivative(n, z) * gamma + t * evaluate_polynomial_derivative(p, z);
    }

    static C evaluate_homotopy_partial_t(const P &p, C z, R t, C gamma)
    {
        size_t n = p.size() - 1;
        return evaluate_polynomial(p, z) - gamma * evaluate_start_polynomial(n, z);
    }

    static C trace_root(const P &p, C root, C gamma)
    {

        constexpr size_t STEPS = 10;
        constexpr size_t MAX_CORRECTIONS = 10;
        constexpr R EPS = 0.001;

        R dt = 1.0 / STEPS;

        R t = 0;
        C z = root;

        size_t total_corrections = 0;
        size_t total_steps = 0;

        for (int i = 0; i < STEPS + 1; i++, t += dt, total_steps++)
        {
            // predict
            C dz = -evaluate_homotopy_partial_t(p, z, t, gamma) / evaluate_homotopy_partial_x(p, z, t, gamma) * dt;
            z += dz;

            // correct
            for (int j = 0; j < MAX_CORRECTIONS; j++, total_corrections++)
            {
                C value = evaluate_homotopy(p, z, t, gamma);
                if (std::abs(value) < EPS)
                    break;
                dz = -value / evaluate_homotopy_partial_x(p, z, t, gamma);
                z += dz;
            }
            printf("Trace: %f + %fi\n", z.real(), z.imag());
        }

        printf("Steps: %d\n", (int)total_steps);
        printf("Average corrections: %f\n", (float)total_corrections / total_steps);

        return z;
    }

    std::vector<float> compute_polynomial_roots(const std::vector<float> &coefficients)
    {

        C gamma = {0, 1};

        size_t n = coefficients.size() - 1;

        std::vector<R> out;
        out.reserve(n);

        for (size_t i = 0; i < n; i++)
        {
            float angle = LiteMath::M_TWOPI * i / n;
            C start_root = {std::cos(angle), std::sin(angle)};
            printf("Start root = %f + %fi\n", start_root.real(), start_root.imag());
            C end_root = trace_root(coefficients, start_root, gamma);
            out.push_back(end_root.real());
            printf("End root = %f + %fi\n", end_root.real(), end_root.imag());
        }

        return out;
    }

}
