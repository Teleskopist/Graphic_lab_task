#pragma once
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <complex>

namespace LiteMath
{
	template <typename T>
	struct Matrix
	{
		struct Row
		{
			Row() = default;
			Row(T *_data, uint32_t _size)
			{
				data = _data;
				size = _size;
			}
			T* data = nullptr;
			uint32_t size = 0;
			T &operator[](uint32_t idx) { return data[idx]; }
  		const T &operator[](size_t idx) const { return data[idx]; }
		};

		struct ConstRow
		{
			ConstRow() = default;
			ConstRow(const T *_data, uint32_t _size)
			{
				data = _data;
				size = _size;
			}
			const T* data = nullptr;
			uint32_t size = 0;
  		const T &operator[](size_t idx) const { return data[idx]; }
		};

		Matrix() = default;
		Matrix(uint32_t _n_rows, uint32_t _n_cols)
		{
			n_rows = _n_rows;
			n_cols = _n_cols;
			data.resize(n_rows*n_cols, T());
		}
		Matrix(uint32_t _n_rows, uint32_t _n_cols, T val)
		{
			n_rows = _n_rows;
			n_cols = _n_cols;
			data.resize(n_rows*n_cols, val);
		}
		Matrix(uint32_t _n_rows, uint32_t _n_cols, const std::vector<T> &in_data)
		{
			n_rows = _n_rows;
			n_cols = _n_cols;
			assert(in_data.size() == n_rows*n_cols);
			data = in_data;
		}
		Matrix(uint32_t _n_rows, uint32_t _n_cols, const T *in_data)
		{
			n_rows = _n_rows;
			n_cols = _n_cols;
			data.clear();
			data.insert(data.end(), in_data, in_data+n_rows*n_rows);
		}

		Row operator[](uint32_t col) { return Row(data.data() + n_rows*col, n_rows); }
		ConstRow operator[](uint32_t col) const { return ConstRow(data.data() + n_rows*col, n_rows); }

		uint32_t n_rows = 0;
		uint32_t n_cols = 0;
		std::vector<T> data; // data, stored by rows
	};

	template <typename T>
	static Matrix<T> mul(const Matrix<T> &A, const Matrix<T> &B)
	{
		if (A.n_cols != B.n_rows)
		{
			assert(false);
			return Matrix<T>();
		}
		Matrix<T> res(A.n_rows, B.n_cols);
		for (int i=0;i<A.n_rows;i++)
		{
			for (int j=0;j<B.n_cols;j++)
			{
				T res_ij = T();
				for (int k=0;k<A.n_cols;k++)
					res_ij = res_ij + A[i][k] * B[k][j];
				res[i][j] = res_ij;
			}
		}
		return res;
	}

	template <typename T>
	Matrix<T> operator*(const Matrix<T> &A, const Matrix<T> &B)
	{
		return mul(A, B);
	}

	template <typename T>
	static std::vector<T> mul(const Matrix<T> &A, const std::vector<T> &v)
	{
		if (A.n_cols != v.size())
		{
			assert(false);
			return {};
		}
		std::vector<T> res(A.n_rows);
		for (int i = 0; i < A.n_rows; i++)
		{
			T res_i = T();
			for (int k = 0; k < A.n_cols; k++)
				res_i = res_i + A[i][k] * v[k];
			res[i] = res_i;
		}
		return res;
	}

	template <typename T>
	std::vector<T> operator*(const Matrix<T> &A, const std::vector<T> &v)
	{
		return mul(A, v);
	}

	template <typename T>
	static Matrix<T> transpose(const Matrix<T> &A)
	{
		Matrix<T> res(A.n_cols, A.n_rows);
		for (int i=0;i<A.n_rows;i++)
		{
			for (int j=0;j<A.n_cols;j++)
			{
				res[j][i] = A[i][j];
			}
		}
		return res;
	}

	template <typename T>
	static Matrix<T> conjugate(const Matrix<T> &A)
	{
		Matrix<T> res(A.n_rows, A.n_cols);
		for (int i=0;i<A.n_rows;i++)
		{
			for (int j=0;j<A.n_cols;j++)
			{
				res[i][j] = std::conj(A[i][j]);
			}
		}
		return res;
	}

	template <typename T>
	static Matrix<T> identity(uint32_t size)
	{
		Matrix<T> res(size, size, T(0));
		for (int i=0;i<size;i++)
			res[i][i]	= T(1);
		return res;
	}

	template <typename T>
	static Matrix<T> scale(uint32_t size, T val)
	{
		Matrix<T> res(size, size, T(0));
		for (int i=0;i<size;i++)
			res[i][i]	= val;
		return res;
	}

	template <typename T>
	static Matrix<T> diag(uint32_t size, const T *values)
	{
		Matrix<T> res(size, size, T(0));
		for (int i=0;i<size;i++)
			res[i][i]	= values[i];
		return res;
	}

	template <typename T>
	static Matrix<T> translate(uint32_t size, const T *vec)
	{
		Matrix<T> res = identity<T>(size);
		for (int i=0;i<size-1;i++)
			res[i][size-1] = vec[i];
		return res;
	}

	template <typename T>
	static void print(const Matrix<T> &m)
	{
		printf("Matrix %dx%d:\n", m.n_cols, m.n_rows);
		for (int i=0;i<m.n_rows;i++)
		{
			for (int j=0;j<m.n_cols;j++)
				printf("%lg ", double(m[i][j]));
			printf("\n");
		}
	}

	template <typename T>
	static void print(const Matrix<std::complex<T>> &m)
	{
		printf("Matrix %dx%d:\n", m.n_cols, m.n_rows);
		for (int i=0;i<m.n_rows;i++)
		{
			for (int j=0;j<m.n_cols;j++)
				printf("%lg %+lgj ", double(std::real(m[i][j])), double(std::imag(m[i][j])));
			printf("\n");
		}
	}
}