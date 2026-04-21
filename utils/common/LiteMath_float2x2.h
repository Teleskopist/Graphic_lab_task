#pragma once
#include "LiteMath.h"

namespace LiteMath
{
  static inline void mat2_colmajor_mul_vec2(float* __restrict RES, const float* __restrict B, const float* __restrict V) 
  {
  	RES[0] = V[0] * B[0] + V[1] * B[2];
  	RES[1] = V[0] * B[1] + V[1] * B[3];
  }

  static inline void transpose2(const float2 in_rows[2], float2 out_rows[2])
  {
    float rows[4];
    store(rows+0,  in_rows[0]);
    store(rows+2,  in_rows[1]);
  
    out_rows[0] = float2{rows[0], rows[2]};
    out_rows[1] = float2{rows[1], rows[3]};
  }

  /**
  \brief this class use colmajor memory layout for effitient vector-matrix operations
  */
  struct float2x2
  {
    inline float2x2()  { identity(); }
    
    inline explicit float2x2(const float rhs)
    {
      m_col[0] = float2(rhs);
      m_col[1] = float2(rhs);
      m_col[2] = float2(rhs); 
    } 

    inline float2x2(const float2x2& rhs) 
    { 
      m_col[0] = rhs.m_col[0];
      m_col[1] = rhs.m_col[1];
      m_col[2] = rhs.m_col[2]; 
    }

    inline float2x2& operator=(const float2x2& rhs)
    {
      m_col[0] = rhs.m_col[0];
      m_col[1] = rhs.m_col[1];
      m_col[2] = rhs.m_col[2]; 
      return *this;
    }

    // col-major matrix from row-major array
    inline explicit float2x2(const float A[4])
    {
      m_col[0] = float2{ A[0], A[2]};
      m_col[1] = float2{ A[1], A[3]};
    }

    inline explicit float2x2(float A0, float A1, float A2, float A3)
    {
      m_col[0] = float2{ A0, A2};
      m_col[1] = float2{ A1, A3};
    }

    inline void identity()
    {
      m_col[0] = float2{ 1.0, 0.0};
      m_col[1] = float2{ 0.0, 1.0};
    }

    inline void zero()
    {
      m_col[0] = float2{ 0.0, 0.0};
      m_col[1] = float2{ 0.0, 0.0};
    }

    inline float2 get_col(int i) const { return m_col[i]; }
    inline void set_col(int i, const float2& a_col) { m_col[i] = a_col; }

    inline float2 get_row(int i) const { return float2{ m_col[0][i], m_col[1][i]}; }
    inline void set_row(int i, const float2& a_col)
    {
      m_col[0][i] = a_col[0];
      m_col[1][i] = a_col[1];
    }

    inline float2& col(int i)       { return m_col[i]; }
    inline float2  col(int i) const { return m_col[i]; }

    inline float& operator()(int row, int col)       { return m_col[col][row]; }
    inline float  operator()(int row, int col) const { return m_col[col][row]; }

    struct RowTmp 
    {
      float2x2* self;
      int row;
      inline float& operator[](int col)       { return self->m_col[col][row]; }
      inline float  operator[](int col) const { return self->m_col[col][row]; }
    };

    inline RowTmp operator[](int a_row) 
    {
      RowTmp row;
      row.self = this;
      row.row  = a_row;
      return row;
    }

    float2 m_col[2];
  };
  
  static inline float2x2 make_float2x2_from_rows(float2 a, float2 b)
  {
    float2x2 m;
    m.set_row(0, a);
    m.set_row(1, b);
    return m;
  }

  static inline float2x2 make_float2x2_from_cols(float2 a, float2 b)
  {
    float2x2 m;
    m.set_col(0, a);
    m.set_col(1, b);
    return m;
  }

  static inline float2 operator*(const float2x2& m, const float2& v)
  {
    float2 res;
    mat2_colmajor_mul_vec2((float*)&res, (const float*)&m, (const float*)&v);
    return res;
  }

  static inline float2 mul(const float2x2& m, const float2& v)
  {
    float2 res;                             
    mat2_colmajor_mul_vec2((float*)&res, (const float*)&m, (const float*)&v);
    return res;
  }

  static inline float2x2 transpose(const float2x2& rhs)
  {
    float2x2 res;
    transpose2(rhs.m_col, res.m_col);
    return res;
  }

  static inline float determinant(const float2x2& mat)
  {
    const float a = mat.m_col[0].x;
    const float b = mat.m_col[1].x;
    const float c = mat.m_col[0].y;
    const float d = mat.m_col[1].y;
    return a*b - c*d;
  }

  static inline float2x2 scale2x2(float2 t)
  {
    float2x2 res;
    res.set_col(0, float2{t.x, 0.0});
    res.set_col(1, float2{0.0, t.y});
    return res;
  }

  static inline float2x2 rotate2x2(float phi)
  {
    float2x2 res;
    res.set_col(0, float2{ std::cos(phi), std::sin(phi)});
    res.set_col(1, float2{-std::sin(phi), std::cos(phi)});
    return res;
  }
  
  static inline float2x2 mul(float2x2 m1, float2x2 m2)
  {
    const float2 column1 = mul(m1, m2.col(0));
    const float2 column2 = mul(m1, m2.col(1));
    float2x2 res;
    res.set_col(0, column1);
    res.set_col(1, column2);

    return res;
  }

  static inline float2x2 operator*(float2x2 m1, float2x2 m2)
  {
    const float2 column1 = mul(m1, m2.col(0));
    const float2 column2 = mul(m1, m2.col(1));

    float2x2 res;
    res.set_col(0, column1);
    res.set_col(1, column2);
    return res;
  }

  static inline float2x2 operator*(float scale, float2x2 m)
  {
    float2x2 res;
    res.m_col[0] = m.m_col[0] * scale;
    res.m_col[1] = m.m_col[1] * scale;
    return res;
  }

  static inline float2x2 operator*(float2x2 m, float scale)
  {
    float2x2 res;
    res.m_col[0] = m.m_col[0] * scale;
    res.m_col[1] = m.m_col[1] * scale;
    return res;
  }

  static inline float2x2 operator+(float2x2 m1, float2x2 m2)
  {
    float2x2 res;
    res.m_col[0] = m1.m_col[0] + m2.m_col[0];
    res.m_col[1] = m1.m_col[1] + m2.m_col[1];
    return res;
  }

  static inline float2x2 operator-(float2x2 m1, float2x2 m2)
  {
    float2x2 res;
    res.m_col[0] = m1.m_col[0] - m2.m_col[0];
    res.m_col[1] = m1.m_col[1] - m2.m_col[1];
    return res;
  }
}