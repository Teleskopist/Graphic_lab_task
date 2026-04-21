#pragma once
#include <vector>
#include <cstdint>
#include "utils/common/rand.h"

class IMABSolver
{
public:
  virtual ~IMABSolver() {}
  virtual uint32_t getAction() const = 0;
  virtual void update(uint32_t action, float reward) = 0;
  virtual void addAction(uint32_t action) = 0;
  virtual void reset() = 0;
};

// ==================== random (baseline for testing) ====================
class MABRandomSolver : public IMABSolver
{
public:
  virtual uint32_t getAction() const override { return m_actions[rand() % m_actions.size()]; }
  virtual void update(uint32_t action, float reward) override {}
  virtual void addAction(uint32_t action) override { m_actions.push_back(action); }
  virtual void reset() override { m_actions.clear(); }
private:
  std::vector<uint32_t> m_actions;
};

// ==================== eps‑greedy ====================
class EpsilonGreedySolver : public IMABSolver {
public:
    explicit EpsilonGreedySolver(float epsilon) : m_epsilon(epsilon) {}

    uint32_t getAction() const override {
        if (m_actions.empty()) return 0;
        // Explore with probability eps
        if (static_cast<float>(rand()) / RAND_MAX < m_epsilon) {
            return m_actions[rand() % m_actions.size()];
        }
        // Otherwise exploit: choose arm with highest average reward
        size_t best_idx = 0;
        float best_value = -1e9f;
        for (size_t i = 0; i < m_actions.size(); ++i) {
            float value = (m_counts[i] == 0) ? 0.0f : m_rewards[i] / m_counts[i];
            if (value > best_value) {
                best_value = value;
                best_idx = i;
            }
        }
        return m_actions[best_idx];
    }

    void update(uint32_t action, float reward) override {
        for (size_t i = 0; i < m_actions.size(); ++i) {
            if (m_actions[i] == action) {
                m_counts[i]++;
                m_rewards[i] += reward;
                break;
            }
        }
    }

    void addAction(uint32_t action) override {
        m_actions.push_back(action);
        m_counts.push_back(0);
        m_rewards.push_back(0.0f);
    }

    void reset() override {
        m_actions.clear();
        m_counts.clear();
        m_rewards.clear();
    }

private:
    float m_epsilon;
    std::vector<uint32_t> m_actions;
    std::vector<uint32_t> m_counts;
    std::vector<float> m_rewards;
};

// ==================== Softmax ====================
class SoftmaxSolver : public IMABSolver {
public:
    explicit SoftmaxSolver(float temperature) : m_tau(temperature) {}

    uint32_t getAction() const override {
        if (m_actions.empty()) return 0;
        size_t n = m_actions.size();
        // Compute softmax probabilities
        std::vector<float> probs(n, 0.0f);
        float max_q = -1e9f;
        for (size_t i = 0; i < n; ++i) {
            float q = (m_counts[i] == 0) ? 0.0f : m_rewards[i] / m_counts[i];
            if (q > max_q) max_q = q;
        }
        float sum = 0.0f;
        for (size_t i = 0; i < n; ++i) {
            float q = (m_counts[i] == 0) ? 0.0f : m_rewards[i] / m_counts[i];
            float exp_val = std::exp((q - max_q) / m_tau); // subtract max for numerical stability
            probs[i] = exp_val;
            sum += exp_val;
        }
        if (sum <= 0.0f) { // fallback to uniform
            return m_actions[rand() % n];
        }
        float r = static_cast<float>(rand()) / RAND_MAX * sum;
        float cum = 0.0f;
        for (size_t i = 0; i < n; ++i) {
            cum += probs[i];
            if (r < cum) return m_actions[i];
        }
        return m_actions[n - 1];
    }

    void update(uint32_t action, float reward) override {
        for (size_t i = 0; i < m_actions.size(); ++i) {
            if (m_actions[i] == action) {
                m_counts[i]++;
                m_rewards[i] += reward;
                break;
            }
        }
    }

    void addAction(uint32_t action) override {
        m_actions.push_back(action);
        m_counts.push_back(0);
        m_rewards.push_back(0.0f);
    }

    void reset() override {
        m_actions.clear();
        m_counts.clear();
        m_rewards.clear();
    }

private:
    float m_tau;
    std::vector<uint32_t> m_actions;
    std::vector<uint32_t> m_counts;
    std::vector<float> m_rewards;
};

// ==================== UCB1 ====================
class UCBSolver : public IMABSolver {
public:
    UCBSolver() = default;

    uint32_t getAction() const override {
        if (m_actions.empty()) return 0;
        size_t n = m_actions.size();
        // First, play any untried arm
        for (size_t i = 0; i < n; ++i) {
            if (m_counts[i] == 0) {
                return m_actions[i];
            }
        }
        // Compute total number of pulls
        uint32_t total_pulls = 0;
        for (size_t i = 0; i < n; ++i) total_pulls += m_counts[i];
        float log_total = std::log(static_cast<float>(total_pulls));
        size_t best_idx = 0;
        float best_ucb = -1e9f;
        for (size_t i = 0; i < n; ++i) {
            float avg = m_rewards[i] / m_counts[i];
            float ucb = avg + std::sqrt(2.0f * log_total / m_counts[i]);
            if (ucb > best_ucb) {
                best_ucb = ucb;
                best_idx = i;
            }
        }
        return m_actions[best_idx];
    }

    void update(uint32_t action, float reward) override {
        for (size_t i = 0; i < m_actions.size(); ++i) {
            if (m_actions[i] == action) {
                m_counts[i]++;
                m_rewards[i] += reward;
                break;
            }
        }
    }

    void addAction(uint32_t action) override {
        m_actions.push_back(action);
        m_counts.push_back(0);
        m_rewards.push_back(0.0f);
    }

    void reset() override {
        m_actions.clear();
        m_counts.clear();
        m_rewards.clear();
    }

private:
    std::vector<uint32_t> m_actions;
    std::vector<uint32_t> m_counts;
    std::vector<float> m_rewards;
};

// ==================== Thompson Sampling (Beta‑Bernoulli) ====================
class ThompsonSamplingSolver : public IMABSolver {
public:
    explicit ThompsonSamplingSolver(float alpha_prior = 1.0f, float beta_prior = 1.0f)
        : m_alpha_prior(alpha_prior), m_beta_prior(beta_prior), m_rng(std::random_device{}()) {}

    uint32_t getAction() const override {
        if (m_actions.empty()) return 0;
        size_t n = m_actions.size();
        float best_sample = -1e9f;
        size_t best_idx = 0;
        for (size_t i = 0; i < n; ++i) {
            float alpha = m_alpha_prior + m_successes[i];
            float beta  = m_beta_prior  + m_failures[i];
            float sample = betaSample(alpha, beta);
            if (sample > best_sample) {
                best_sample = sample;
                best_idx = i;
            }
        }
        return m_actions[best_idx];
    }

    void update(uint32_t action, float reward) override {
        // Assume reward is binary (0 or 1). For non‑binary, extend appropriately.
        for (size_t i = 0; i < m_actions.size(); ++i) {
            if (m_actions[i] == action) {
                if (reward > 0.5f)
                    m_successes[i]++;
                else
                    m_failures[i]++;
                break;
            }
        }
    }

    void addAction(uint32_t action) override {
        m_actions.push_back(action);
        m_successes.push_back(0);
        m_failures.push_back(0);
    }

    void reset() override {
        m_actions.clear();
        m_successes.clear();
        m_failures.clear();
    }

private:
    float betaSample(float alpha, float beta) const {
        // Generate a Beta(alpha, beta) sample via two independent Gamma variables
        std::gamma_distribution<float> gamma_alpha(alpha, 1.0f);
        std::gamma_distribution<float> gamma_beta(beta, 1.0f);
        float x = gamma_alpha(m_rng);
        float y = gamma_beta(m_rng);
        return x / (x + y);
    }

    float m_alpha_prior;
    float m_beta_prior;
    std::vector<uint32_t> m_actions;
    std::vector<uint32_t> m_successes;   // number of rewards ≈ 1
    std::vector<uint32_t> m_failures;    // number of rewards ≈ 0
    mutable std::mt19937 m_rng;          // mutable to allow sampling in const getAction()
};