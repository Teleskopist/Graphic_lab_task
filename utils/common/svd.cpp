#include "svd.h"

float dot(StrideView<const float> x, StrideView<const float> y, size_t dimensions)
{
    float ans = 0;
    for (size_t i = 0; i < dimensions; ++i)
    {
        ans += x[i] * y[i];
    }
    return ans;
}

bool converged(const Matrix2D<float> &A, float tolerance)
{
    size_t m = A.rowsCount();
    size_t n = A.colsCount();
    float max_off_diag = 0.0f;

    for (size_t i = 0; i < n - 1; ++i)
    {
        for (size_t j = i + 1; j < n; ++j)
        {
            auto col_i = A.col(i);
            auto col_j = A.col(j);
            float dot_ij = std::abs(dot(col_i, col_j, m));
            max_off_diag = std::max(max_off_diag, dot_ij);
        }
    }

    return max_off_diag < tolerance;
}

Matrix2D<float> get_identity_matrix(size_t n)
{
    Matrix2D<float> A(n, n, 0.0f);
    for (size_t i = 0; i < n; i++)
    {
        A[{i, i}] = 1.0f;
    }
    return A;
}

void jacobian_rotation(Matrix2D<float> &A, Matrix2D<float> &V, size_t p, size_t q)
{
    auto x_p = A.col(p);
    auto x_q = A.col(q);
    
    float rows = A.rowsCount();
    float w_p = dot(x_p, x_p, rows);
    float w_q = dot(x_q, x_q, rows);
    float xi = dot(x_p, x_q, rows);
    
    float Q = w_p - w_q;
    float t;
    
    if (std::abs(Q) < 1e-4 && std::abs(xi) < 1e-4) {
        t = 0.0f;
    } else {
        float root = std::sqrt(Q * Q + 4 * xi * xi);
        t = 2 * xi / (Q + (Q >= 0 ? root : -root));
    }
    
    float c = 1.0f / std::sqrt(1.0f + t * t);
    float s = c * t;
    
    // Rotate columns of A
    for (size_t i = 0; i < rows; ++i) {
        float a_ip = x_p[i];
        float a_iq = x_q[i];
        x_p[i] = c * a_ip + s * a_iq;
        x_q[i] = -s * a_ip + c * a_iq;
    }
    
    // Update rotation matrix V
    auto v_p = V.col(p);
    auto v_q = V.col(q);
    for (size_t i = 0; i < V.rowsCount(); ++i) {
        float v_ip = v_p[i];
        float v_iq = v_q[i];
        v_p[i] = c * v_ip + s * v_iq;
        v_q[i] = -s * v_ip + c * v_iq;
    }
}

std::pair<Matrix2D<float>, Matrix2D<float>> svd_get_XV(Matrix2D<float> A)
{
    auto V = get_identity_matrix(A.colsCount());
    int c = 0;
    int max_iters = 10;
    for (int k = 0; k < max_iters; ++k)
    {
        for (size_t i = 0; i < A.colsCount() - 1; ++i)
        {
            for (size_t j = i + 1; j < A.colsCount(); ++j)
            {
                jacobian_rotation(A, V, i, j);
            }
        }
        if (converged(A, 1e-7))
        {
            break;
        }
    }
    return {A, V};
}