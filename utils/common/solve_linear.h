#pragma once

#include <vector>

template <typename T>
std::vector<T> solveLinear(std::vector<std::vector<T>> &A, std::vector<T> &b)
{
    int n = A.size();
    std::vector<T> x(n, 1);

    // printf("started\n");
    // for (int i = 0; i < A.size(); ++i)
    // {
    //   for (int j = 0; j < A[i].size(); ++j)
    //   {
    //     printf("%f ", A[i][j]);
    //   }
    //   printf("| %f\n", b[i]);
    // }

    for (int i = 0; i < n; i++)
    {
        int max_row = i;
        for (int k = i + 1; k < n; k++)
        {
            if (std::abs(A[k][i]) > std::abs(A[max_row][i]))
            {
                max_row = k;
            }
        }
        std::swap(A[max_row], A[i]);
        std::swap(b[max_row], b[i]);

        if (std::abs(A[i][i]) < 1e-9)
        {
            continue;
        }

        for (int k = i + 1; k < n; k++)
        {
            T c = A[k][i] / A[i][i];
            for (int j = i; j < n; j++)
            {
                A[k][j] -= c * A[i][j];
            }
            b[k] -= c * b[i];
        }
    }

    for (int i = n - 1; i >= 0; i--)
    {
        if (std::abs(A[i][i]) > 1e-9)
        {
            x[i] = b[i];
            for (int j = i + 1; j < n; j++)
            {
                x[i] -= A[i][j] * x[j];
            }
            x[i] /= A[i][i];
        }
    }

    return x;
}
