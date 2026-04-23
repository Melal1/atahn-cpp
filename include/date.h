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
#include <ctime>
#include <fmt/base.h>
#include <string_view>

#define A_inline __attribute__((always_inline)) inline
#define AD_inline [[nodiscard]] __attribute__((always_inline)) inline
#define NDES [[nodiscard]]

// Helper to get local time safely
inline std::tm get_local_time() noexcept {
    std::time_t t = std::time(nullptr);
    std::tm tm_now;
    localtime_r(&t, &tm_now);
    return tm_now;
}

// -----------------------------------------------------------------------------
// Time Class
// -----------------------------------------------------------------------------
class Time
{
public:
  uint16_t hour = 0;
  uint16_t min = 0;
  uint16_t sec = 0;

  constexpr Time() noexcept = default;

  constexpr Time(uint16_t h, uint16_t m, uint16_t s = 0) noexcept 
      : hour(h), min(m), sec(s) {}

  static Time now() noexcept {
      std::tm tm_now = get_local_time();
      return Time(tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
  }

  void set(uint16_t h, uint16_t m, uint16_t s = 0) noexcept {
      hour = h; min = m; sec = s;
  }

  uint32_t to_min() const noexcept {
      return hour * 60 + min;
  }

  uint32_t to_sec() const noexcept {
      return hour * 3600 + min * 60 + sec;
  }

  // Returns seconds remaining until the day ends (midnight)
  uint32_t seconds_till_end_of_day() const noexcept {
      return 86400 - to_sec();
  }
  
  // Returns minutes remaining until the day ends (midnight)
  uint32_t minutes_till_end_of_day() const noexcept {
      return (86400 - to_sec()) / 60;
  }

  void add_seconds(uint32_t s) noexcept {
      uint32_t total = to_sec() + s;
      hour = (total / 3600) % 24;
      min = (total % 3600) / 60;
      sec = total % 60;
  }

  void add_minutes(uint32_t m) noexcept {
      add_seconds(m * 60);
  }

  void add_hours(uint32_t h) noexcept {
      hour = (hour + h) % 24;
  }

  auto operator<=>(const Time &) const = default;

  size_t fill_buff(char *buffer, size_t size, char sep = ':') const noexcept {
      auto res = FMT_TYPE::format_to_n(buffer, size - 1, "{:02}{:c}{:02}{:c}{:02}", hour, sep, min, sep, sec);
      *res.out = '\0';
      return res.out - buffer;
  }

  void print(char sep = ':') const {
      FMT_TYPE::print("{:02}{:c}{:02}{:c}{:02}", hour, sep, min, sep, sec);
  }

  void println(char sep = ':') const {
      print(sep);
      FMT_TYPE::print("\n");
  }
};

// -----------------------------------------------------------------------------
// Date Class
// -----------------------------------------------------------------------------
class Date
{
public:
  Date() noexcept {
      std::tm tm_now = get_local_time();
      _tp = std::chrono::sys_days{
          std::chrono::year{tm_now.tm_year + 1900} / 
          (tm_now.tm_mon + 1) / 
          tm_now.tm_mday
      };
  }

  Date(int y, unsigned m, unsigned d) noexcept {
      _tp = std::chrono::sys_days{std::chrono::year{y} / std::chrono::month{m} / std::chrono::day{d}};
  }

  explicit Date(std::string_view date_str) noexcept
  {
    using namespace std::chrono;
    size_t i = 0;
    auto parse_uint = [&date_str, &i](uint32_t &out) -> bool
    {
      out = 0;
      size_t start = i;
      while (i < date_str.size() && (date_str[i] >= '0' && date_str[i] <= '9'))
      {
        out = out * 10 + (date_str[i] - '0');
        ++i;
      }
      return i > start;
    };

    uint32_t d = 0, m = 0, y = 0;
    if (!parse_uint(d) || i >= date_str.size() || date_str[i] != '/') return;
    ++i;
    if (!parse_uint(m) || i >= date_str.size() || date_str[i] != '/') return;
    ++i;
    if (!parse_uint(y) || i != date_str.size()) return;

    _tp = sys_days{year{(int)y} / month{m} / day{d}};
  }

  AD_inline auto is_leap() const noexcept -> bool {
      return get_ymd_struct().year().is_leap();
  }

  AD_inline static constexpr auto is_leap(uint16_t year) noexcept -> bool {
      return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
  }

  void add_days(uint32_t days_to_add) noexcept {
      _tp += std::chrono::days{days_to_add};
  }
  
  void sub_days(uint32_t days_to_sub) noexcept {
      _tp -= std::chrono::days{days_to_sub};
  }

  AD_inline auto is_last_day_year() const noexcept -> bool {
      auto ymd = get_ymd_struct();
      return ymd.month() == std::chrono::December && ymd.day() == std::chrono::day{31};
  }

  AD_inline auto is_last_day_month() const noexcept -> bool {
      auto next_day = _tp + std::chrono::days{1};
      return std::chrono::year_month_day{next_day}.day() == std::chrono::day{1};
  }

  AD_inline uint8_t get_last_day_month() const noexcept {
      auto ymd = get_ymd_struct();
      auto m = static_cast<unsigned>(ymd.month());
      if (m == 2 && ymd.year().is_leap()) return 29;
      return _month_days[m - 1];
  }

  AD_inline auto get_day_order_week() const noexcept -> uint8_t {
      return std::chrono::weekday{_tp}.c_encoding(); // 0 = Sun, 1 = Mon, ..., 6 = Sat
  }
  
  AD_inline bool is_weekend() const noexcept {
      auto wd = get_day_order_week();
      return wd == 0 || wd == 6; // Sunday or Saturday
  }

  AD_inline auto operator<=>(const Date &) const -> std::strong_ordering = default;

  AD_inline auto get_year() const noexcept -> int {
      return static_cast<int>(get_ymd_struct().year());
  }

  AD_inline auto get_month() const noexcept -> unsigned {
      return static_cast<unsigned>(get_ymd_struct().month());
  }

  AD_inline auto get_day() const noexcept -> unsigned {
      return static_cast<unsigned>(get_ymd_struct().day());
  }

  AD_inline auto get_tp() const noexcept -> std::chrono::sys_days {
      return _tp;
  }

  A_inline void set_year(int y) noexcept {
      auto ymd = get_ymd_struct();
      _tp = std::chrono::sys_days{std::chrono::year{y} / ymd.month() / ymd.day()};
  }

  A_inline void set_month(unsigned m) noexcept {
      auto ymd = get_ymd_struct();
      _tp = std::chrono::sys_days{ymd.year() / std::chrono::month{m} / ymd.day()};
  }

  A_inline void set_day(unsigned d) noexcept {
      auto ymd = get_ymd_struct();
      _tp = std::chrono::sys_days{ymd.year() / ymd.month() / std::chrono::day{d}};
  }

  AD_inline auto is_between(const Date &start, const Date &end) const noexcept -> bool {
      return _tp >= start._tp && _tp <= end._tp;
  }
  
  int days_since(const Date& other) const noexcept {
      return (_tp - other._tp).count();
  }

  size_t fill_buff(char *buffer, size_t size, char sep = '/') const noexcept {
      auto ymd = get_ymd_struct();
      auto res = FMT_TYPE::format_to_n(
          buffer, size - 1, "{:02}{:c}{:02}{:c}{:04}", 
          static_cast<unsigned>(ymd.day()), sep, static_cast<unsigned>(ymd.month()), sep, static_cast<int>(ymd.year()));
      *res.out = '\0';
      return res.out - buffer;
  }

  void print(char sep = '/') const {
      auto ymd = get_ymd_struct();
      FMT_TYPE::print("{:02}{:c}{:02}{:c}{:04}", 
          static_cast<unsigned>(ymd.day()), sep, static_cast<unsigned>(ymd.month()), sep, static_cast<int>(ymd.year()));
  }

  void println(char sep = '/') const {
      print(sep);
      FMT_TYPE::print("\n");
  }

private:
  std::chrono::sys_days _tp;

  AD_inline auto get_ymd_struct() const noexcept -> std::chrono::year_month_day {
      return std::chrono::year_month_day{_tp};
  }

  static constexpr uint32_t _month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
};

// -----------------------------------------------------------------------------
// DateTime Class
// -----------------------------------------------------------------------------
class DateTime
{
public:
    Date date;
    Time time;

    DateTime() noexcept {
        std::tm tm_now = get_local_time();
        date = Date(tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday);
        time = Time(tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
    }
    
    DateTime(Date d, Time t) noexcept : date(d), time(t) {}
    
    void add_seconds(uint32_t s) noexcept {
        uint32_t total_sec = time.to_sec() + s;
        uint32_t days_to_add = total_sec / 86400;
        uint32_t rem_sec = total_sec % 86400;
        
        if (days_to_add > 0) {
            date.add_days(days_to_add);
        }
        
        time.hour = rem_sec / 3600;
        time.min = (rem_sec % 3600) / 60;
        time.sec = rem_sec % 60;
    }
    
    void add_minutes(uint32_t m) noexcept {
        add_seconds(m * 60);
    }
    
    void add_hours(uint32_t h) noexcept {
        add_seconds(h * 3600);
    }
    
    void add_days(uint32_t d) noexcept {
        date.add_days(d);
    }

    int64_t difference_in_seconds(const DateTime& other) const noexcept {
        int64_t days_diff = date.days_since(other.date);
        int64_t sec_diff = static_cast<int64_t>(time.to_sec()) - static_cast<int64_t>(other.time.to_sec());
        return days_diff * 86400 + sec_diff;
    }
    
    auto operator<=>(const DateTime& other) const {
        if (auto cmp = date <=> other.date; cmp != 0) return cmp;
        return time <=> other.time;
    }
    
    bool operator==(const DateTime& other) const {
        return (*this <=> other) == 0;
    }

    size_t fill_buff(char *buffer, size_t size, char d_sep = '/', char between = ' ', char t_sep = ':') const noexcept {
        auto ymd = std::chrono::year_month_day{date.get_tp()};
        auto res = FMT_TYPE::format_to_n(
            buffer, size - 1, 
            "{:02}{:c}{:02}{:c}{:04}{:c}{:02}{:c}{:02}{:c}{:02}", 
            static_cast<unsigned>(ymd.day()), d_sep, static_cast<unsigned>(ymd.month()), d_sep, static_cast<int>(ymd.year()),
            between,
            time.hour, t_sep, time.min, t_sep, time.sec);
        *res.out = '\0';
        return res.out - buffer;
    }

    void print(char d_sep = '/', char between = ' ', char t_sep = ':') const {
        auto ymd = std::chrono::year_month_day{date.get_tp()};
        FMT_TYPE::print(
            "{:02}{:c}{:02}{:c}{:04}{:c}{:02}{:c}{:02}{:c}{:02}", 
            static_cast<unsigned>(ymd.day()), d_sep, static_cast<unsigned>(ymd.month()), d_sep, static_cast<int>(ymd.year()),
            between,
            time.hour, t_sep, time.min, t_sep, time.sec);
    }

    void println(char d_sep = '/', char between = ' ', char t_sep = ':') const {
        print(d_sep, between, t_sep);
        FMT_TYPE::print("\n");
    }
};
