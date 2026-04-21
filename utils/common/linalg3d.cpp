#include "linalg3d.h"
#include <complex>

namespace linalg3d
{

    constexpr double RANK_EPS = 1.0e-6;

    double3 orthogonal(double3 v)
    {
        size_t max_i = 0;
        for (size_t i = 1; i < 3; i++)
            if (std::abs(v[i]) > std::abs(v[max_i]))
                max_i = i;

        size_t other_i = (max_i + 1) % 3;

        if (v[max_i] == 0.0)
            return {0, 0, 0};

        double3 out = {0, 0, 0};
        out[other_i] = -1;
        out[max_i] = v[other_i] / v[max_i];
        return out;
    }

    std::array<double3, 3> complete_basis(static_vector<double3, 3> vectors)
    {
        if (vectors.size() == 0)
            vectors.push_back({1, 0, 0});
        if (vectors.size() == 1)
            vectors.push_back(normalize(orthogonal(vectors[0])));
        if (vectors.size() == 2)
            vectors.push_back(normalize(cross(normalize(vectors[0]), normalize(vectors[1]))));
        return {normalize(vectors[0]), normalize(vectors[1]), normalize(vectors[2])};
    }

    static_vector<double3, 3> compute_kernel_basis(double3x3 matrix)
    {

        {
            // Trying to compute as if matrix has rank 2

            double3 candidate = {0, 0, 0};
            for (size_t i = 0; i < 3; i++)
            {
                for (size_t j = 0; j < 3; j++)
                {
                    if (i != j)
                    {
                        double3 a = matrix.get_row(i);
                        double3 b = matrix.get_row(j);
                        if (length(a) > RANK_EPS && length(b) > RANK_EPS)
                        {
                            double3 c = cross(normalize(a), normalize(b));
                            if (length(c) > length(candidate))
                                candidate = c;
                        }
                    }
                }
            }
            if (length(candidate) > RANK_EPS)
            {
                return {candidate};
            }
        }

        {
            // Trying to compute as if matrix has rank 1

            double3 max_row = matrix.get_row(0);
            for (size_t i = 1; i < 3; i++)
            {
                double3 a = matrix.get_row(i);
                if (length(a) > length(max_row))
                {
                    max_row = a;
                }
            }

            if (length(max_row) > RANK_EPS)
            {
                max_row = normalize(max_row);
                double3 first = normalize(orthogonal(max_row));
                return {first, cross(first, max_row)};
            }
        }

        // Matrix has rank 0
        return complete_basis({});
    }

    static_vector<double3, 3> reorthogonalize(static_vector<double3, 3> vectors)
    {

        if (vectors.size() > 0)
        {
            vectors[0] = normalize(vectors[0]);

            for (size_t i = 1; i < vectors.size(); i++)
            {
                double3 res = vectors[i];
                for (size_t j = 0; j < i; j++)
                    res -= vectors[j] * dot(vectors[j], vectors[i]);
                vectors[i] = normalize(res);
            }
        }
        return vectors;
    }

    bool are_linearly_independent(static_vector<double3, 3> vectors)
    {
        if (vectors.size() == 0)
            return true;
        if (vectors.size() == 1)
            return length(vectors[0]) > 0.001;
        if (vectors.size() == 2)
            return length(cross(vectors[0], vectors[1])) > 0.001;
        double3x3 m;
        for (int i = 0; i < 3; i++)
            m.set_row(i, vectors[i]);
        return std::abs(LiteMath::determinant(m)) > 0.001;
    }

    static_vector<double3, 3> make_linearly_independent(static_vector<double3, 3> vectors)
    {
        std::reverse(vectors.begin(), vectors.end());
        static_vector<double3, 3> out;
        // Push first
        if (!vectors.empty())
        {
            if (length(vectors.back()) > RANK_EPS)
                out.push_back(vectors.back());
            vectors.pop_back();
        }

        // Push second
        if (!vectors.empty())
        {
            if (length(vectors.back()) > RANK_EPS)
                out.push_back(vectors.back());
            vectors.pop_back();
        }

        // Removing second
        if (!are_linearly_independent(out))
            out.pop_back();

        // Push third
        if (!vectors.empty())
        {
            if (length(vectors.back()) > RANK_EPS)
                out.push_back(vectors.back());
            vectors.pop_back();
        }

        // Removing third
        if (!are_linearly_independent(out))
            out.pop_back();

        return out;
    }

    static_vector<double, 3> solve_depressed_cubic(double p, double q)
    {

        /* Q = (p/3)^3 + (q/2)^2 */
        double Q = p * p * p / 27 + q * q / 4;

        std::complex Q_sq = std::sqrt(std::complex(Q));

        std::complex alpha = std::pow(-q / 2 + Q_sq, 1.0 / 3);
        std::complex beta = -p / 3 / alpha;

        if (q == 0.0 && p == 0.0)
        {
            alpha = 0.0;
            beta = 0.0;
        }

        std::complex i(0.0, 1.0);

        std::complex y1 = alpha + beta;
        std::complex y2 = -(alpha + beta) / 2.0 + i * (alpha - beta) / 2.0 * std::sqrt(3.0);
        std::complex y3 = -(alpha + beta) / 2.0 - i * (alpha - beta) / 2.0 * std::sqrt(3.0);

        if (std::abs(Q) < 1.0e-12)
        {
            if (p == 0.0 && q == 0.0)
            {
                return {y1.real()};
            }
            else
            {
                return {y1.real(), y2.real()};
            }
        }
        else if (Q > 0)
        {
            return {y1.real()};
        }
        else
        {
            return {y1.real(), y2.real(), y3.real()};
        }
    }

    static_vector<double, 3> solve_cubic(double4 cubic)
    {
        /* ax^3 + bx^2 + c^x + d = 0 */
        double a = cubic[3];
        double b = cubic[2];
        double c = cubic[1];
        double d = cubic[0];

        /* x^3 + px + q = 0 */
        double p = (3 * a * c - b * b) / (3 * a * a);
        double q = (2 * b * b * b - 9 * a * b * c + 27 * a * a * d) / (27 * a * a * a);

        auto roots = solve_depressed_cubic(p, q);

        for (auto &i : roots)
            i = i - b / 3 / a;

        return roots;
    }

    static void compute_characteristic_polynomial(double3x3 matrix, double4 &polynomial)
    {
        /* Codegened with scripts/codegen_eigenvalues.py */
        double a11 = matrix(0, 0);
        double a12 = matrix(0, 1);
        double a13 = matrix(0, 2);
        double a21 = matrix(1, 0);
        double a22 = matrix(1, 1);
        double a23 = matrix(1, 2);
        double a31 = matrix(2, 0);
        double a32 = matrix(2, 1);
        double a33 = matrix(2, 2);
        /* Solving: det(A - t*I) = 0 */
        /* Expanded: a11*a22*a33 - a11*a22*t - a11*a23*a32 - a11*a33*t + a11*t**2 - a12*a21*a33 + a12*a21*t + a12*a23*a31 + a13*a21*a32 - a13*a22*a31 + a13*a31*t - a22*a33*t + a22*t**2 + a23*a32*t + a33*t**2 - t**3 = 0 */
        /* As sum of powers: a11*a22*a33 - a11*a23*a32 - a12*a21*a33 + a12*a23*a31 + a13*a21*a32 - a13*a22*a31 - t**3 + t**2*(a11 + a22 + a33) + t*(-a11*a22 - a11*a33 + a12*a21 + a13*a31 - a22*a33 + a23*a32) = 0 */
        polynomial[0] = a11 * a22 * a33 - a11 * a23 * a32 - a12 * a21 * a33 + a12 * a23 * a31 + a13 * a21 * a32 - a13 * a22 * a31;
        polynomial[1] = -a11 * a22 - a11 * a33 + a12 * a21 + a13 * a31 - a22 * a33 + a23 * a32;
        polynomial[2] = a11 + a22 + a33;
        polynomial[3] = -1;
    }

    static_vector<double, 3> compute_eigenvalues(double3x3 matrix)
    {
        double4 cubic;
        compute_characteristic_polynomial(matrix, cubic);
        return solve_cubic(cubic);
    }

    static_vector<double3, 3> compute_eigenvectors(double3x3 matrix, double eigenvalue)
    {
        for (size_t i = 0; i < 3; i++)
        {
            matrix(i, i) -= eigenvalue;
        }
        return compute_kernel_basis(matrix);
    }

    static_vector<double3, 3> compute_eigenvectors(double3x3 matrix)
    {
        auto eigenvalues = compute_eigenvalues(matrix);
        std::sort(eigenvalues.begin(), eigenvalues.end(), [](auto a, auto b)
                  { return std::abs(a) > std::abs(b); });

        static_vector<static_vector<double3, 3>, 3> eigenvectors;
        for (auto v : eigenvalues)
            eigenvectors.push_back(compute_eigenvectors(matrix, v));

        static_vector<double3, 3> out;

        for (auto &vs : eigenvectors)
            if (!vs.empty() && out.size() < 3)
            {
                out.push_back(vs.back());
                vs.pop_back();
            }

        for (auto &vs : eigenvectors)
            if (!vs.empty() && out.size() < 3)
            {
                out.push_back(vs.back());
                vs.pop_back();
            }

        for (auto &vs : eigenvectors)
            if (!vs.empty() && out.size() < 3)
            {
                out.push_back(vs.back());
                vs.pop_back();
            }

        return out;
    }

}
