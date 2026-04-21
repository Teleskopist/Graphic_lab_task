#pragma once

#include "utils/common/matrix.h"
#include <complex>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cassert>

namespace LiteMath
{
  //3x3 ROW-MAJOR rotation matrix
  struct Rotation
  {
    Rotation()
    {
      for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
          mat[i][j] = (i == j) ? 1.0f : 0.0f;
    }
    float mat[3][3];
  };

  // Helper function: compute factorial
  inline double factorial(int n) {
      if (n <= 1) return 1.0;
      double result = 1.0;
      for (int i = 2; i <= n; i++) {
          result *= i;
      }
      return result;
  }

  // Helper function: extract ZYZ Euler angles from rotation matrix
  // Returns {alpha, beta, gamma} for ZYZ convention
  inline std::vector<float> rotation_to_euler_zyz(const Rotation& R) {
      float alpha, beta, gamma;
      
      // Extract beta (angle around Y axis) 
      float cos_beta = std::clamp(R.mat[2][2], -1.0f, 1.0f);
      beta = std::acos(cos_beta);
      
      float sin_beta = std::sin(beta);
      
      if (std::abs(sin_beta) > 1e-6f) 
      {
          // Normal case
          alpha = std::atan2(R.mat[1][2],  R.mat[0][2]);
          gamma = std::atan2(R.mat[2][1], -R.mat[2][0]);
      } 
      else if (cos_beta > 0)
      {
        alpha = 0.0f;
        beta = 0.0f;
        gamma = std::atan2(R.mat[1][0], R.mat[0][0]);
      }
      else
      {
        alpha = 0.0f;
        beta = M_PI;
        gamma = std::atan2(-R.mat[1][0], -R.mat[0][0]);
      }
      
      return {alpha, beta, gamma};
  }

  // Compute single element of Wigner small-d matrix
  inline double wigner_d_element(int l, int m1, int m2, float beta) {
      int smin = std::max(0, m2 - m1);
      int smax = std::min(l + m2, l - m1);
      
      double pre = std::sqrt(factorial(l + m1) * 
                            factorial(l - m1) * 
                            factorial(l + m2) * 
                            factorial(l - m2));
      
      double cos_half = std::cos(beta / 2.0);
      double sin_half = std::sin(beta / 2.0);
      
      double v = 0.0;
      for (int s = smin; s <= smax; s++) {
          double num = std::pow(-1.0, m1 - m2 + s) * 
                      std::pow(cos_half, 2 * l + m2 - m1 - 2 * s) * 
                      std::pow(sin_half, m1 - m2 + 2 * s);
          
          double denom = factorial(l + m2 - s) * 
                        factorial(s) * 
                        factorial(m1 - m2 + s) * 
                        factorial(l - m1 - s);
          
          v += num / denom;
      }
      
      return pre * v;
  }

  // Compute Wigner small-d matrix for order l and angle beta
  inline Matrix<double> wigner_d(int l, float beta) {
      int size = 2 * l + 1;
      Matrix<double> d(size, size);
      
      for (int i = 0, m1 = -l; m1 <= l; m1++, i++) {
          for (int j = 0, m2 = -l; m2 <= l; m2++, j++) {
              d[i][j] = wigner_d_element(l, m1, m2, beta);
          }
      }
      
      return d;
  }

  // Compute complex Wigner D-matrix for canonical spherical harmonics
  inline Matrix<std::complex<double>> wigner_D(int l, const Rotation& R) {
      // Extract Euler angles (ZYZ convention)
      auto euler = rotation_to_euler_zyz(R);
      float alpha = euler[0];
      float beta = euler[1];
      float gamma = euler[2];
      
      // Compute small-d matrix
      auto d = wigner_d(l, beta);
      printf("RRR = %f %f %f\n%f %f %f\n %f %f %f\n", R.mat[0][0], R.mat[0][1], R.mat[0][2], 
             R.mat[1][0], R.mat[1][1], R.mat[1][2], R.mat[2][0], R.mat[2][1], R.mat[2][2]);
      printf("ABG %f %f %f\n d\n", alpha, beta, gamma);
      print(d);
      
      int size = 2 * l + 1;
      Matrix<std::complex<double>> D(size, size);
      
      // Apply diagonal phase matrices: D = diag(exp(i*m*gamma)) @ d @ diag(exp(i*m*alpha))
      for (int i = 0, m_i = -l; m_i <= l; m_i++, i++) {
          std::complex<double> phase_gamma = std::exp(std::complex<double>(0.0, m_i * gamma));
          
          for (int j = 0, m_j = -l; m_j <= l; m_j++, j++) {
              std::complex<double> phase_alpha = std::exp(std::complex<double>(0.0, m_j * alpha));
              D[i][j] = phase_gamma * d[i][j] * phase_alpha;
          }
      }
      
      return D;
  }

  // Compute tesseral transformation matrix for order l
  inline Matrix<std::complex<double>> tesseral_transformation(int l) {
      int size = 2 * l + 1;
      Matrix<std::complex<double>> T(size, size, std::complex<double>(0.0, 0.0));
      
      double invsq2r = 1.0 / std::sqrt(2.0);
      std::complex<double> invsq2i(0.0, invsq2r);
      
      for (int i = 0, m1 = -l; m1 <= l; m1++, i++) {
          for (int j = 0, m2 = -l; m2 <= l; m2++, j++) {
              if (std::abs(m1) != std::abs(m2)) {
                  continue;
              }
              
              if (m1 < 0) {
                  if (m2 < 0) {
                      T[i][j] = invsq2i;
                  } else if (m2 > 0) {
                      T[i][j] = -std::pow(-1.0, m1) * invsq2i;
                  }
              }
              
              if (m1 > 0) {
                  if (m2 < 0) {
                      T[i][j] = invsq2r;
                  } else if (m2 > 0) {
                      T[i][j] = std::pow(-1.0, m1) * invsq2r;
                  }
              }
          }
      }
      
      // Center element (m=0)
      T[l][l] = std::complex<double>(1.0, 0.0);
      
      return T;
  }

  // Extract real part of complex matrix
  template<typename T>
  inline Matrix<T> real_part(const Matrix<std::complex<T>>& M) {
      Matrix<T> result(M.n_rows, M.n_cols);
      for (uint32_t i = 0; i < M.n_rows; i++) {
          for (uint32_t j = 0; j < M.n_cols; j++) {
              result[i][j] = M[i][j].real();
          }
      }
      return result;
  }

  // Produce Wigner D-matrix for tesseral spherical harmonics for a rotation
  inline Matrix<double> tesseral_wigner_D(int l, const Rotation& rotation) {
      auto T = tesseral_transformation(l);
      auto D = wigner_D(l, rotation);
      auto T_conj_transpose = conjugate(transpose(T));
      
      // Compute: real(T @ D @ T^H)
      auto result_complex = T * D * T_conj_transpose;
      return real_part(result_complex);
  }

  // Produce Wigner D-matrix for tesseral spherical harmonics for a mirror operation
  inline Matrix<double> tesseral_wigner_D_mirror(int l, const std::vector<float>& normal_vec) {
      assert(normal_vec.size() == 3);
      
      // Normalize the normal vector
      float norm = std::sqrt(normal_vec[0] * normal_vec[0] + 
                            normal_vec[1] * normal_vec[1] + 
                            normal_vec[2] * normal_vec[2]);
      std::vector<float> normal = {normal_vec[0] / norm, 
                                  normal_vec[1] / norm, 
                                  normal_vec[2] / norm};
      
      // Construct mirror matrix: M = I - 2 * n * n^T
      Rotation M = Rotation();
      for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
              M.mat[i][j] -= 2.0f * normal[i] * normal[j];
          }
      }
      
      // Decompose mirror into rotation and inversion: R = -M
      Rotation R = Rotation();
      for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
              R.mat[i][j] = -M.mat[i][j];
          }
      }
      
      // Inversion factor
      double inv = std::pow(-1.0, l);
      
      auto T = tesseral_transformation(l);
      auto D = wigner_D(l, R);
      auto T_conj_transpose = conjugate(transpose(T));
      
      // Compute: inv * real(T @ D @ T^H)
      auto result_complex = T * D * T_conj_transpose;
      printf("R = %f %f %f\n%f %f %f\n %f %f %f\n", R.mat[0][0], R.mat[0][1], R.mat[0][2], 
             R.mat[1][0], R.mat[1][1], R.mat[1][2], R.mat[2][0], R.mat[2][1], R.mat[2][2]);
      print(T);
      print(D);
      print(T_conj_transpose);
      print(result_complex);
      auto result = real_part(result_complex);
      
      // Apply inversion factor
      for (uint32_t i = 0; i < result.n_rows; i++) {
          for (uint32_t j = 0; j < result.n_cols; j++) {
              result[i][j] *= inv;
          }
      }
      
      return result;
  }

  // Produce Wigner D-matrix for tesseral spherical harmonics under improper rotation
  inline Matrix<double> tesseral_wigner_D_improper(int l, const Rotation& rotation) {
      // First compute the rotation part
      auto T = tesseral_transformation(l);
      auto D = wigner_D(l, rotation);
      auto T_conj_transpose = conjugate(transpose(T));
      printf("TOTOTOTO\n");
        print(T);
        print(T_conj_transpose);
        print(D);
      auto rotation_part_complex = T * D * T_conj_transpose;
      auto rotation_part = real_part(rotation_part_complex);
      
      // Extract rotation axis from rotation matrix (approximate axis-angle extraction)
      // For improper rotation, we need the axis that the rotation is around
      float trace = rotation.mat[0][0] + rotation.mat[1][1] + rotation.mat[2][2];
      float angle = std::acos((trace - 1.0f) / 2.0f);
      
      std::vector<float> axis(3);
      if (std::abs(angle) > 1e-6f) {
          axis[0] = (rotation.mat[2][1] - rotation.mat[1][2]) / (2.0f * std::sin(angle));
          axis[1] = (rotation.mat[0][2] - rotation.mat[2][0]) / (2.0f * std::sin(angle));
          axis[2] = (rotation.mat[1][0] - rotation.mat[0][1]) / (2.0f * std::sin(angle));
      } else {
          // Small angle, use arbitrary axis
          axis = {0.0f, 0.0f, 1.0f};
      }
      
      // Compute mirror matrix for this axis
      printf("axis %f %f %f\n", axis[0], axis[1], axis[2]);
      auto M = tesseral_wigner_D_mirror(l, axis);
      
      // Combine: M @ rotation_part
      return M * rotation_part;
  }
}