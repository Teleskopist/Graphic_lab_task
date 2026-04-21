#pragma once

#if defined(__clang__) || defined(__GNUC__)
#define unreachable() __builtin_unreachable()
#define assume(x) (!!(x) ? (void)0 : unreachable())
#elif defined(_WIN32)
#define assume(x) __assume(x)
#define unreachable() assume(0)
#else
#define unreachable() ((void)0)
#define assume(x) ((void)x)
#endif
