#pragma once

#if __has_include(<fmt/format.h>)
#include <fmt/chrono.h>
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
static_assert(false, "This lib requires a format header (fmt/format.h or std::format)");
#endif

#include <chrono>
#include <compare>
#include <cstdint>
#include <fmt/base.h>
#include <string_view>

#define A_inline __attribute__((always_inline)) inline
#define AD_inline [[nodiscard]] __attribute__((always_inline)) inline
#define NDES [[nodiscard]]

struct with_time_t
{
};

inline constexpr with_time_t with_time{};

class Date
{
public:
  Date() noexcept : _tp(std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now()))
  {
  }

  explicit Date(with_time_t) noexcept : _tp(std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()))
  {
  }

  explicit Date(std::string_view date_str) noexcept
  {
    using namespace std::chrono;
    size_t i = 0;
    auto parse_uint = [&date_str, &i](uint32_t &out) -> bool
    {
      out = 0;
      size_t start = i;
      while (i < date_str.size() && is_ascii_digit(static_cast<unsigned char>(date_str[i])))
      {
        out = out * 10 + (date_str[i] - '0');
        ++i;
      }
      return i > start;
    };

    uint32_t d = 0, m = 0, y = 0;
    if (!parse_uint(d) || i >= date_str.size() || date_str[i] != '/')
      return;
    ++i;

    if (!parse_uint(m) || i >= date_str.size() || date_str[i] != '/')
      return;
    ++i;

    if (!parse_uint(y) || i != date_str.size())
      return;

    _tp = sys_days{year{(int)y} / month{m} / day{d}};
  }

  AD_inline auto is_leap() const noexcept -> bool
  {
    return get_ymd_struct().year().is_leap();
  }

  AD_inline static constexpr auto is_leap(uint16_t year) noexcept -> bool
  {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
  }

  void add_days(uint32_t days_to_add) noexcept
  {
    _tp += std::chrono::days{days_to_add};
  }

  AD_inline auto is_last_day_year() const noexcept -> bool
  {
    auto ymd = get_ymd_struct();
    return ymd.month() == std::chrono::December && ymd.day() == std::chrono::day{31};
  }

  AD_inline auto is_last_day_month() const noexcept -> bool
  {
    auto next_day = _tp + std::chrono::days{1};
    return std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(next_day)}.day() == std::chrono::day{1};
  }

  AD_inline uint8_t get_last_day_month() const noexcept
  {
    auto ymd = get_ymd_struct();
    auto m = static_cast<unsigned>(ymd.month());

    if (m == 2 && ymd.year().is_leap())
      return 29;

    return _month_days[m - 1];
  }

  AD_inline auto get_day_order_week() const noexcept -> uint8_t
  {
    return std::chrono::weekday{std::chrono::floor<std::chrono::days>(_tp)}.c_encoding();
  }

  AD_inline auto operator<=>(const Date &) const -> std::strong_ordering = default;

  AD_inline auto get_year() const noexcept -> int
  {
    return static_cast<int>(get_ymd_struct().year());
  }

  AD_inline auto get_month() const noexcept -> unsigned
  {
    return static_cast<unsigned>(get_ymd_struct().month());
  }

  AD_inline auto get_day() const noexcept -> unsigned
  {
    return static_cast<unsigned>(get_ymd_struct().day());
  }

  AD_inline auto get_tp() const noexcept -> std::chrono::sys_seconds
  {
    return _tp;
  }

  A_inline void set_year(int y) noexcept
  {
    auto ymd = get_ymd_struct();
    _tp = std::chrono::sys_days{std::chrono::year{y} / ymd.month() / ymd.day()};
  }

  A_inline void set_month(unsigned m) noexcept
  {
    auto ymd = get_ymd_struct();
    _tp = std::chrono::sys_days{ymd.year() / std::chrono::month{m} / ymd.day()};
  }

  A_inline void set_day(unsigned d) noexcept
  {
    auto ymd = get_ymd_struct();
    _tp = std::chrono::sys_days{ymd.year() / ymd.month() / std::chrono::day{d}};
  }

  AD_inline auto is_between(const Date &start, const Date &end) const noexcept -> bool
  {
    return _tp >= start._tp && _tp <= end._tp;
  }

  size_t fill_buff(char *buffer, size_t size, char sep = '/') const noexcept
  {
    auto ymd = get_ymd_struct();
    auto res = FMT_TYPE::format_to_n(
        buffer, size - 1, "{:%d}{:c}{:%m}{:c}{:%Y}", ymd.day(), sep, ymd.month(), sep, ymd.year());
    *res.out = '\0';
    return res.out - buffer;
  }

  size_t fill_buff_time(char *buffer, size_t size, char sep = ':') const noexcept
  {
    auto hms = get_hms_struct();
    auto res = FMT_TYPE::format_to_n(
        buffer, size - 1, "{:%H}{:c}{:%M}{:c}{:%S}", hms.hours(), sep, hms.minutes(), sep, hms.seconds());
    *res.out = '\0';
    return res.out - buffer;
  }

  size_t fill_buff_with_time(char *buffer, size_t size, char d_sep = '/', char between = '-', char t_sep = ':') const noexcept
  {
    auto ymd = get_ymd_struct();
    auto hms = get_hms_struct();
    auto res = FMT_TYPE::format_to_n(
        buffer,
        size - 1,
        "{:%d}{:c}{:%m}{:c}{:%Y}{:c}{:%H}{:c}{:%M}{:c}{:%S}",
        ymd.day(), d_sep, ymd.month(), d_sep, ymd.year(),
        between,
        hms.hours(), t_sep, hms.minutes(), t_sep, hms.seconds());
    *res.out = '\0';
    return res.out - buffer;
  }

  void print(char sep = '/') const
  {
    auto ymd = get_ymd_struct();
    fmt::print("{:%d}{:c}{:%m}{:c}{:%Y}", ymd.day(), sep, ymd.month(), sep, ymd.year());
  }

  void println(char sep = '/') const
  {
    print(sep);
    fmt::print("\n");
  }

  void print_time(char sep = ':') const
  {
    auto hms = get_hms_struct();
    fmt::print("{:%H}{:c}{:%M}{:c}{:%S}", hms.hours(), sep, hms.minutes(), sep, hms.seconds());
  }

  void println_time(char sep = ':') const
  {
    print_time(sep);
    fmt::print("\n");
  }

  void print_with_time(char d_sep = '/', char t_sep = ':') const
  {
    auto ymd = get_ymd_struct();
    auto hms = get_hms_struct();
    fmt::print(
        "{:%d}{:c}{:%m}{:c}{:%Y} {:%H}{:c}{:%M}{:c}{:%S}",
        ymd.day(), d_sep, ymd.month(), d_sep, ymd.year(),
        hms.hours(), t_sep, hms.minutes(), t_sep, hms.seconds());
  }

  void println_with_time(char d_sep = '/', char t_sep = ':') const
  {
    print_with_time(d_sep, t_sep);
    fmt::print("\n");
  }

private:
  std::chrono::sys_seconds _tp;

  AD_inline static constexpr bool is_ascii_digit(unsigned char c) noexcept
  {
    return c >= '0' && c <= '9';
  }

  AD_inline auto get_ymd_struct() const noexcept -> std::chrono::year_month_day
  {
    return std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(_tp)};
  }

  AD_inline auto get_hms_struct() const noexcept -> std::chrono::hh_mm_ss<std::chrono::seconds>
  {
    return std::chrono::hh_mm_ss{_tp - std::chrono::floor<std::chrono::days>(_tp)};
  }

  static constexpr uint32_t _month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
};
