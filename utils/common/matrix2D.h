#pragma once
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>

#ifdef _WIN32
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#include "LiteMath.h"

template <typename T> class StrideIterator {
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = ptrdiff_t;
  using pointer = T *;
  using reference = T &;

public:
  reference operator*() const { return *m_ptr; }
  pointer operator->() const { return m_ptr; }
  StrideIterator &operator+=(difference_type n) {
    m_ptr += n * m_stride;
    return *this;
  }
  StrideIterator &operator-=(difference_type n) { return (*this += -n); }
  StrideIterator &operator++() { return (*this += 1); }
  StrideIterator operator++(int) {
    StrideIterator tmp = *this;
    ++(*this);
    return tmp;
  }
  StrideIterator &operator--() { return (*this -= 1); }
  StrideIterator operator--(int) {
    StrideIterator tmp = *this;
    --(*this);
    return tmp;
  };
  friend StrideIterator operator+(StrideIterator it, difference_type n) {
    it += n;
    return it;
  }
  friend StrideIterator operator+(difference_type n, StrideIterator it) {
    return it + n;
  }
  friend StrideIterator operator-(StrideIterator it, difference_type n) {
    return it + (-n);
  }
  reference operator[](difference_type n) const { return *(*this + n); }
  friend difference_type operator-(StrideIterator it1, StrideIterator it2) {
    return (it1.m_ptr - it2.m_ptr) / it1.m_stride;
  }
  friend bool operator<(const StrideIterator &lhs, const StrideIterator &rhs) {
    return lhs.m_ptr < rhs.m_ptr;
  }
  friend bool operator>(const StrideIterator &lhs, const StrideIterator &rhs) {
    return rhs < lhs;
  }
  friend bool operator<=(const StrideIterator &lhs, const StrideIterator &rhs) {
    return !(lhs > rhs);
  }
  friend bool operator>=(const StrideIterator &lhs, const StrideIterator &rhs) {
    return !(lhs < rhs);
  }
  friend bool operator==(const StrideIterator &lhs, const StrideIterator &rhs) {
    return !(lhs < rhs) && !(lhs > rhs);
  }
  friend bool operator!=(const StrideIterator &lhs, const StrideIterator &rhs) {
    return !(lhs == rhs);
  }

public:
  StrideIterator(T *ptr = nullptr, size_t stride = 1)
      : m_ptr(ptr), m_stride(stride) {}

private:
  T *m_ptr = nullptr;
  size_t m_stride = 1;
};

template <typename T> class StrideView {
public:
  using iterator = StrideIterator<T>;
  using const_iterator = StrideIterator<const T>;
  static constexpr size_t npos = size_t(-1);

public:
  T &operator[](size_t indx) noexcept { return m_data[indx * m_stride]; }
  const T &operator[](size_t indx) const noexcept {
    return m_data[indx * m_stride];
  }
  T *data() noexcept { return m_data; }
  const T *data() const noexcept { return m_data; }
  size_t size() const noexcept { return m_size; }
  size_t stride() const noexcept { return m_stride; }
  StrideView slice(size_t startIndex = 0, size_t endIndex = npos) noexcept {
    if (endIndex == npos) {
      endIndex = m_size;
    }
    assert(startIndex <= endIndex);
    assert(endIndex <= m_size);
    return StrideView(m_data + startIndex * m_stride, endIndex - startIndex,
                      m_stride);
  }
  StrideView<const T> slice(size_t startIndex = 0,
                            size_t endIndex = npos) const noexcept {
    return StrideView<const T>(m_data + startIndex * m_stride,
                               endIndex - startIndex, m_stride);
  }
  StrideView prefix(size_t size) noexcept {
    assert(size <= m_size);
    return slice(0, size);
  }
  StrideView<const T> prefix(size_t size) const noexcept {
    assert(size <= m_size);
    return slice(0, size);
  }
  StrideView suffix(size_t size) noexcept {
    assert(size <= m_size);
    return slice(m_size - size, m_size);
  }
  StrideView<const T> suffix(size_t size) const noexcept {
    assert(size <= m_size);
    return slice(m_size - size, m_size);
  }
  iterator begin() noexcept { return iterator(m_data, m_stride); }
  iterator end() noexcept {
    return iterator(m_data + m_size * m_stride, m_stride);
  }
  const_iterator begin() const noexcept {
    return const_iterator(m_data, m_stride);
  }
  const_iterator end() const noexcept {
    return const_iterator(m_data + m_size * m_stride, m_stride);
  }
  const_iterator cbegin() const noexcept {
    return const_iterator(m_data, m_stride);
  }
  const_iterator cend() const noexcept {
    return const_iterator(m_data + m_size * m_stride, m_stride);
  }

public:
  explicit StrideView(T *data, size_t size, std::ptrdiff_t stride = 1)
      : m_data(data), m_size(size), m_stride(stride) {}
  StrideView(const StrideView<std::remove_const_t<T>> &other)
      : m_data(other.m_data), m_size(other.m_size), m_stride(other.m_stride) {}

private:
  T *m_data = nullptr;
  size_t m_size = 0;
  size_t m_stride = 1;
  template <typename U> friend class StrideView;
};
// template <typename T> StrideView(T *, size_t) -> StrideView<T>;
// template <typename T> StrideView(const T *, size_t) -> StrideView<const T>;

template <class T, class = void> struct IsIterator : std::false_type {};
template <class T>
struct IsIterator<
    T, std::void_t<typename std::iterator_traits<T>::iterator_category>>
    : std::true_type {};
template <typename T> constexpr bool IsIterator_v = IsIterator<T>::value;

template <typename T> class Matrix2D {
public:
  T &operator[](std::pair<size_t, size_t> ids) {
    return m_elements[ids.first * m_colsCount + ids.second];
  }
  const T &operator[](std::pair<size_t, size_t> ids) const {
    return m_elements[ids.first * m_colsCount + ids.second];
  }
  T &operator[](size_t flatIndex) {
    return m_elements[flatIndex];
  }
  const T &operator[](size_t flatIndex) const {
    return m_elements[flatIndex];
  }
  const T *data() const { return m_elements.data(); }
  T *data() { return m_elements.data(); }
  StrideView<T> row(size_t indx) {
    return StrideView(m_elements.data() + indx * m_colsCount, m_colsCount, 1);
  }
  StrideView<const T> row(size_t indx) const {
    return StrideView(m_elements.data() + indx * m_colsCount, m_colsCount, 1);
  }
  StrideView<T> col(size_t indx) {
    return StrideView(m_elements.data() + indx, m_rowsCount, m_colsCount);
  }
  StrideView<const T> col(size_t indx) const {
    return StrideView(m_elements.data() + indx, m_rowsCount, m_colsCount);
  }
  const std::vector<T> &flatten() const noexcept { return m_elements; }
  size_t rowsCount() const noexcept { return m_rowsCount; }
  size_t colsCount() const noexcept { return m_colsCount; }
  size_t shape(size_t dim) const noexcept {
    return (dim == 0) ? m_rowsCount : m_colsCount;
  }
  size_t size() const noexcept { return m_elements.size(); }
  std::pair<size_t, size_t> shape() const noexcept {
    return {m_rowsCount, m_colsCount};
  }
  size_t size(size_t dim) const noexcept {
    return (dim == 0) ? m_rowsCount : m_colsCount;
  }
  auto begin() noexcept { return m_elements.begin(); }
  auto end() noexcept { return m_elements.end(); }
  auto begin() const noexcept { return m_elements.begin(); }
  auto end() const noexcept { return m_elements.end(); }
  auto cbegin() const noexcept { return m_elements.cbegin(); }
  auto cend() const noexcept { return m_elements.cend(); }
  auto rbegin() noexcept { return m_elements.rbegin(); }
  auto rend() noexcept { return m_elements.rend(); }
  auto crbegin() const noexcept { return m_elements.crbegin(); }
  auto crend() const noexcept { return m_elements.crend(); }
  void resize(size_t rowsCount, size_t colsCount, const T &defaultValue = T()) {
    m_rowsCount = rowsCount;
    m_colsCount = colsCount;
    m_elements.resize(m_rowsCount * m_colsCount, defaultValue);
  }

public:
  explicit Matrix2D(size_t rowsCount = 0, size_t colsCount = 0,
                    const T &defaultValue = T())
      : m_rowsCount(rowsCount), m_colsCount(colsCount),
        m_elements(m_rowsCount * m_colsCount, defaultValue) {}
  template <class It, typename = std::enable_if_t<IsIterator_v<It>>>
  Matrix2D(It begin, It end, size_t elementsPerRow) : m_elements(begin, end) {
    m_colsCount = elementsPerRow;
    m_rowsCount = m_elements.size() / m_colsCount;
    assert(m_elements.size() == m_rowsCount * m_colsCount);
  }

//private:
  size_t m_rowsCount = 0;
  size_t m_colsCount = 0;
  std::vector<T> m_elements;
};