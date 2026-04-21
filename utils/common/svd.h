#pragma once
#include "utils/common/matrix2D.h"
#include "LiteMath.h"

// One-sided Jacobi algorithm
// A is m x n, where m >= n
// A = XV, where V's columns are not sorted by singular values
// Norms of R columns are singular values and u_i = R[:,i] / sigma_i
std::pair<Matrix2D<float>, Matrix2D<float>> svd_get_XV(Matrix2D<float> A);
float dot(StrideView<const float> x, StrideView<const float> y, size_t dimensions);