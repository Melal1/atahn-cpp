// Copyright (c) 2026 Basel Saramijou (github:melal1)
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
#pragma once
#include "core.h"
#include "string_utils.h"

namespace util
{
template <class T> AD_inline constexpr bool range(T min, T max, T value) noexcept
{
  static_assert(std::is_arithmetic_v<T>, "Value should be a number");
  return (value >= min && value <= max);
}

void num_to_words(size_t nume, static_str_base &buffer) noexcept;
} // namespace util

namespace __detail
{
inline constexpr char const *_numbers_1_to_19[] = {
    "one ",
    "two ",
    "three ",
    "four ",
    "five ",
    "six ",
    "seven ",
    "eight ",
    "nine ",
    "ten ",
    "eleven ",
    "twelve ",
    "thirteen ",
    "fourteen ",
    "fifteen ",
    "sixteen ",
    "seventeen ",
    "eighteen ",
    "nineteen "};
inline constexpr char const *_tens_place[] = {
    "twenty ", "thirty ", "forty ", "fifty ", "sixty ", "seventy ", "eighty ", "ninety "};
inline constexpr char const *_scales[] = {
    "Thousand ", "Million ", "Billion ", "Trillion ", "Quadrillion ", "Quintillion "};
inline constexpr std::array<size_t, 7> _pow1000 = {
    1u,
    1000u,
    1000u * 1000u,
    1000u * 1000u * 1000u,
    1000u * 1000u * 1000u * 1000u,
    1000u * 1000u * 1000u * 1000u * 1000u,
    1000u * 1000u * 1000u * 1000u * 1000u * 1000u};

AD_inline uint32_t _calc_chunks(size_t num) noexcept
{

  uint32_t chunks = 0;
  while (num != 0)
  {
    ++chunks;
    num /= 1000;
  }
  return chunks;
}

AD_inline static bool _1_to_19(size_t num, util::static_str_base &str) noexcept
{
  if (num < 1 || num > 19)
    return false;

  str.add(_numbers_1_to_19[num - 1]);
  return true;
}

AD_inline static bool _1_to_99(size_t num, util::static_str_base &str) noexcept
{
  if (num > 99)
    return false;

  if (num < 20)
    return _1_to_19(num, str);

  int tens = num / 10;
  int ones = num % 10;
  str.add(_tens_place[tens - 2]);
  if (ones > 0)
    str.add(_numbers_1_to_19[ones - 1]);

  return true;
}

AD_inline static bool _1_to_999(size_t num, util::static_str_base &str)
{
  if (num > 999)
    return false;
  if (num < 100)
    return _1_to_99(num, str);

  int hundreds = num / 100;
  (void)_1_to_19(hundreds, str);
  str.add("hundred ");
  (void)_1_to_99(num % 100, str);
  return true;
}
} // namespace __detail

inline void util::num_to_words(size_t num, static_str_base &buffer) noexcept
{
  uint32_t chunks = ::__detail::_calc_chunks(num);

  while (chunks != 0)
  {
    size_t pow = 1;
    if (chunks <= ::__detail::_pow1000.size())
    {
      pow = ::__detail::_pow1000[chunks - 1];
    }
    else
    {
      for (uint32_t i = 1; i < chunks; ++i)
        pow *= 1000;
    }
    (void)::__detail::_1_to_999(num / pow, buffer);
    if (chunks >= 2)
      buffer.add(::__detail::_scales[chunks - 2]);
    num %= pow;
    --chunks;
  }
}
