#pragma once
#define HAS_LIBUTILS 1
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>
#include <iostream>
#include <istream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#if __has_include(<fmt/format.h>)
#include <fmt/format.h>
#include <fmt/std.h>
#define FMT_TYPE fmt
#define HAS_FORMAT 1
#elif __has_include(<format>) && __has_include(<print>) && __cplusplus >= 202002L
#include <format>
#include <print>
#define FMT_TYPE std
#define HAS_FORMAT 1
#endif
#ifndef HAS_FORMAT
#define HAS_FORMAT 0
#endif

#define A_inline __attribute__((always_inline)) inline
#define AD_inline [[nodiscard]] __attribute__((always_inline)) inline
#define NDES [[nodiscard]]