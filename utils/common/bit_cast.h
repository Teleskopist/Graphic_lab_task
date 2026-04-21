#pragma once
#include <type_traits>
#include <cstring>

template <typename T, typename U>
T bit_cast(const U &x) noexcept
{
    static_assert(sizeof(T) == sizeof(U));
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_copyable_v<U>);
    static_assert(std::is_trivially_constructible_v<T>);

    T y;
    std::memcpy(&y, &x, sizeof(y));
    return y;
}
