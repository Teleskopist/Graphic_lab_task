#pragma once
#include "LiteMath.h"
#include "static_vector.h"
#include <array>

namespace linalg3d
{

    using LiteMath::double3, LiteMath::double3x3, LiteMath::double4, LiteMath::double4x4;

    double3 orthogonal(double3 v);
    std::array<double3, 3> complete_basis(static_vector<double3, 3> vectors);
    static_vector<double3, 3> compute_kernel_basis(double3x3 matrix);

    static_vector<double3, 3> reorthogonalize(static_vector<double3, 3> vectors);
    
    bool are_linearly_independent(static_vector<double3, 3> vectors);
    static_vector<double3, 3> make_linearly_independent(static_vector<double3, 3> vectors);

    /*
        x^3 + px + q = 0
    */
    static_vector<double, 3> solve_depressed_cubic(double p, double q);

    /*
        c[3]*x^3 + c[2] * x^2 + c[1] * x + c[0]
    */
    static_vector<double, 3> solve_cubic(double4 cubic);

    static_vector<double, 3> compute_eigenvalues(double3x3 matrix);
    static_vector<double3, 3> compute_eigenvectors(double3x3 matrix, double eigenvalue);
    static_vector<double3, 3> compute_eigenvectors(double3x3 matrix);

}
