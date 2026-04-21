#pragma once
#include <vector>

namespace equations
{

    /*
        p(x) = c[0] + x * c[1] + ... + x**n * c[n]
    */
    std::vector<float> compute_polynomial_roots(const std::vector<float> &coefficients);

}
