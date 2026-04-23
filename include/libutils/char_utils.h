// Copyright (c) 2026 Basel Saramijou (github:melal1)
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT
#pragma once
#include "core.h"

namespace util
{
constexpr bool is_ascii_alpha(unsigned char c) noexcept
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr bool is_ascii_digit(unsigned char c) noexcept
{
  return c >= '0' && c <= '9';
}

constexpr bool is_ascii_upper(unsigned char c) noexcept
{
  return c >= 'A' && c <= 'Z';
}

constexpr bool is_ascii_punct(unsigned char c) noexcept
{
  return (c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 126);
}
} // namespace util
