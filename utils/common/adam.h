#pragma once
#include <vector>

  template<typename T>
  class AdamOptimizer
  {
  public:
    AdamOptimizer(int _params_count, T _lr = T(0.15f), T _beta_1 = T(0.9f), T _beta_2 = T(0.999f), T _eps = T(1e-8)) 
    {
      lr = _lr;
      beta_1 = _beta_1;
      beta_2 = _beta_2;
      eps = _eps;
      V = std::vector<T>(_params_count, 0);
      S = std::vector<T>(_params_count, 0);
      params_count = _params_count;
    }
    
    void step(T *params_ptr, const T* grad_ptr, int iter)
    {
      const auto b1 = std::pow(beta_1, iter + 1);
      const auto b2 = std::pow(beta_2, iter + 1);
      for (size_t i = 0; i < params_count; i++)
      {
        auto g = grad_ptr[i];
        V[i] = beta_1 * V[i] + (1 - beta_1) * g;
        auto Vh = V[i] / (T(1) - b1);
        S[i] = beta_2 * S[i] + (1 - beta_2) * g * g;
        auto Sh = S[i] / (T(1) - b2);
        params_ptr[i] -= lr * Vh / (std::sqrt(Sh) + eps);
      }
    }
  private:
    T lr, beta_1, beta_2, eps;
    std::vector<T> V;
    std::vector<T> S;
    size_t params_count;
  };