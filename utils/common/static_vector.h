#pragma once
#include <initializer_list>
#include <stdexcept>
#include <array>
#include "unreachable.h"

template <typename T, size_t Size>
class static_vector
{
public:
    using iterator = T *;
    using const_iterator = const T *;

    static_vector(size_t size = 0, const T &value = {}) : size_(0)
    {
        resize(size);
    }

    static_vector(std::initializer_list<T> init) : size_(0)
    {
        resize(init.size());
        auto it = init.begin();
        for (size_t i = 0; i < init.size(); i++, it++)
        {
            data_[i] = *it;
        }
    }

    template <size_t S>
    static_vector(const static_vector<T, S> &other) : size_(0)
    {
        resize(other.size());
        for (size_t i = 0; i < size(); i++)
        {
            data_[i] = other.data()[i];
        }
    }

    template <size_t S>
    static_vector(T (&data)[S])
    {
        resize(S);
        for (size_t i = 0; i < S; i++)
        {
            data_[i] = data[i];
        }
    }

    template <size_t S>
    static_vector(const T (&data)[S])
    {
        resize(S);
        for (size_t i = 0; i < S; i++)
        {
            data_[i] = data[i];
        }
    }

    template <size_t S>
    static_vector(const std::array<T, S> &data)
    {
        resize(S);
        for (size_t i = 0; i < S; i++)
        {
            data_[i] = data[i];
        }
    }

    template <size_t S = Size>
    std::array<T, S> to_array() const
    {
        if (S > size_)
            throw std::out_of_range("Size of static array is larger than of dynamic array.");

        std::array<T, S> out;
        for (size_t i = 0; i < S; i++)
        {
            out[i] = data_[i];
        }
        return out;
    }

    T *data() { return std::launder(data_); }
    const T *data() const { return std::launder(data_); }
    size_t size() const
    {
        // false positive for static vector https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104165
        assume(size_ <= Size);
        return size_;
    }

    static constexpr size_t capacity() { return Size; }

    iterator begin() { return data(); }
    const_iterator begin() const { return data(); }
    const_iterator cbegin() const { return data(); }

    iterator end() { return data() + size(); }
    const_iterator end() const { return data() + size(); }
    const_iterator cend() const { return data() + size(); }

    bool empty() const { return size() == 0; }
    size_t max_size() const { return Size; }

    T &front() { return *begin(); }
    const T &front() const { return *begin(); }
    T &back() { return *(end() - 1); }
    const T &back() const { return *(end() - 1); }

    void clear()
    {
        size_ = 0;
    }

    void resize(size_t new_size, const T &value = {})
    {
        if (new_size > Size)
        {
            throw std::bad_alloc();
        }
        size_ = new_size;
        for (size_t i = size_; i < new_size; i++)
        {
            data_[i] = value;
        }
    }

    void push_back(const T &value)
    {
        resize(size() + 1);
        back() = value;
    }

    void pop_back()
    {
        resize(size() - 1);
    }

    T &operator[](size_t i) { return *(data() + i); }
    const T &operator[](size_t i) const { return *(data() + i); }

private:
    T data_[Size];
    size_t size_;
};
