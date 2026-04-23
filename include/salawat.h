#pragma once
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <expected>
#include <fstream>
#include <type_traits>
#include <utility>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "date.h"
#include "httplib.h"
#include "libutils/libutils.h"
#include "types.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;



constexpr const char *cache_file = "/tmp/athan_cache.json";

enum class CalculationMethod : i32
{
  Karachi = 1,
  ISNA = 2,
  MWL = 3,
  Makkah = 4,
  Egypt = 5,
  Tehran = 7,
  Gulf = 8,
  London = 11,
  France = 12,
  Turkey = 13
};

class Salawat
{
public:
  using str = util::static_str_base;

  enum class Result : u8
  {
    loaded_from_file,
    loaded_form_net,
    err
  };

  struct NextPrayer
  {
    const char *name;
    Time time;
  };

  Salawat(const char *country, const char *city, CalculationMethod method)
      : _city(city), _country(country), _method(static_cast<i32>(method)), _flags(0)
  {
    _check_init();
  }

  Salawat() : _method(0), _flags(0)
  {
  }

  std::expected<Time, const char *> fajr() noexcept
  {
    return _get_timing(0);
  }

  std::expected<Time, const char *> dhuhr() noexcept
  {
    return _get_timing(1);
  }

  std::expected<Time, const char *> asr() noexcept
  {
    return _get_timing(2);
  }

  std::expected<Time, const char *> maghrib() noexcept
  {
    return _get_timing(3);
  }

  std::expected<Time, const char *> isha() noexcept
  {
    return _get_timing(4);
  }

  void print_timings() noexcept
  {
    if (!_check_flag(Flags::timings_loaded))
    {
      FMT_TYPE::print(stderr, "Cannot print: {}\n", _timing_err);
      return;
    }

    static constexpr std::array labels{"Fajr", "Dhuhr", "Asr", "Maghrib", "Isha"};
    for (size_t i = 0; i < labels.size(); ++i)
    {
      FMT_TYPE::println("{:<8} : {:02}:{:02}", labels[i], _prayer_times[i].hour, _prayer_times[i].min);
    }
  }

  NextPrayer get_next_prayer() const noexcept
  {
    std::time_t t = std::time(nullptr);
    std::tm tm_now;
    localtime_r(&t, &tm_now);
    u32 now_min = tm_now.tm_hour * 60 + tm_now.tm_min;

    static constexpr std::array labels{"Fajr", "Dhuhr", "Asr", "Maghrib", "Isha"};

    for (size_t i = 0; i < 5; ++i)
    {
      if (_prayer_times[i].to_min() > now_min)
      {
        return {labels[i], _prayer_times[i]};
      }
    }
    return {labels[0], _prayer_times[0]};
  }

  i64 seconds_until_next() const noexcept
  {
    std::time_t t = std::time(nullptr);
    std::tm tm_now;
    localtime_r(&t, &tm_now);
    
    u32 now_sec = tm_now.tm_hour * 3600 + tm_now.tm_min * 60 + tm_now.tm_sec;
    
    auto next = get_next_prayer();
    u32 next_sec = next.time.hour * 3600 + next.time.min * 60;
    
    if (next_sec <= now_sec) 
    {
      // It's Fajr of the next day
      return (86400LL - now_sec) + next_sec;
    }
    return static_cast<i64>(next_sec) - now_sec;
  }

  void set_country(str country) noexcept
  {
    _country.copy(country);
    _check_init();
  }

  void set_city(str city) noexcept
  {
    _city.copy(city);
    _check_init();
  }

  void set_method(i32 method) noexcept
  {
    _method = method;
    _check_init();
  }

  template <class T = std::nullptr_t> Result load(T &&on_err = nullptr) noexcept;

private:
  enum class Flags : u8
  {
    settings_loaded = 1 << 0,
    timings_loaded = 1 << 1
  };

  static constexpr const char *_timing_err = "Prayer times aren't loaded";
  util::static_str<100> _city;
  util::static_str<100> _country;
  i32 _method;
  u8 _flags;
  Time _prayer_times[5];

  std::expected<Time, const char *> _get_timing(size_t index) noexcept
  {
    if (!_check_flag(Flags::timings_loaded))
      return std::unexpected(_timing_err);
    return _prayer_times[index];
  }

  void _parse_timings(const json &js) noexcept;
  std::expected<std::string, const char *> fetch_timings() noexcept;

  std::expected<json, const char *> _parse_response(const std::string &body)
  {
    auto data = json::parse(body, nullptr, false);
    if (data.is_discarded())
    {
      return std::unexpected("JSON Parse Error");
    }
    if (!data.contains("data") || !data["data"].contains("timings"))
    {
      return std::unexpected("JSON Error: missing data/timings");
    }
    return data["data"]["timings"];
  }

  void _set_flag(Flags f) noexcept
  {
    _flags |= static_cast<u8>(f);
  }

  bool _check_flag(Flags f) const noexcept
  {
    return (_flags & static_cast<u8>(f));
  }

  void _check_init() noexcept
  {
    if (_method > 0 && !_country.is_empty() && !_city.is_empty())
    {
      _set_flag(Flags::settings_loaded);
    }
  }
};

void Salawat::_parse_timings(const json &js) noexcept
{
  auto parse = [&](const char *prayer, u32 index)
  {
    std::string_view s = js[prayer].get<std::string_view>();
    size_t mid = s.find(':');
    if (mid != std::string_view::npos)
    {
      std::from_chars(s.data(), s.data() + mid, _prayer_times[index].hour);
      std::from_chars(s.data() + mid + 1, s.data() + s.length(), _prayer_times[index].min);
    }
  };

  static constexpr std::array keys{"Fajr", "Dhuhr", "Asr", "Maghrib", "Isha"};
  for (size_t i = 0; i < keys.size(); ++i)
  {
    parse(keys[i], i);
  }
  _set_flag(Flags::timings_loaded);
}

std::expected<std::string, const char *> Salawat::fetch_timings() noexcept
{
  if (!_check_flag(Flags::settings_loaded))
  {
    return std::unexpected("Error: Athan object not fully initialized.");
  }

  httplib::Client cli("https://api.aladhan.com");
  cli.set_follow_location(true);

  util::static_str<512> path;
  path.format("/v1/timingsByCity?city={}&country={}&method={}", _city.data(), _country.data(), _method);

  if (auto res = cli.Get(path.data()))
  {
    if (res->status == 200)
    {
      return std::move(res->body);
    }
    return std::unexpected("HTTP Error");
  }
  return std::unexpected("Network Error");
}

template <class T> Salawat::Result Salawat::load(T &&on_err) noexcept
{
  auto handle_err = [&](const char *err)
  {
    if constexpr (!std::is_same_v<std::decay_t<T>, std::nullptr_t>)
    {
      on_err(err);
    }
  };

  bool from_cache = false;
  json js;

  std::ifstream file(cache_file);
  if (file.is_open())
  {
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (!content.empty())
    {
      js = json::parse(content, nullptr, false);
      if (!js.is_discarded() && js.contains("date"))
      {
        Date today;
        Date file_date(js["date"].get<std::string_view>());
        if (today == file_date)
        {
          from_cache = true;
        }
      }
    }
  }

  if (!from_cache)
  {
    auto net_res = fetch_timings();
    if (!net_res)
    {
      handle_err(net_res.error());
      return Result::err;
    }

    auto parsed = _parse_response(*net_res);
    if (!parsed)
    {
      handle_err(parsed.error());
      return Result::err;
    }

    js = std::move(*parsed);
    char buff[32];
    Date d;
    d.fill_buff(buff, sizeof(buff));
    js["date"] = buff;

    std::ofstream out(cache_file);
    if (out.is_open())
    {
      out << js.dump();
    }
  }

  _parse_timings(js);
  return from_cache ? Result::loaded_from_file : Result::loaded_form_net;
}
