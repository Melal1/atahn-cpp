#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../include/httplib.h"
#include "../include/libutils/libutils.h"
#include "../include/types.h"
#include <fmt/core.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum class CalculationMethod : i32
{
  Karachi = 1,
  ISNA = 2,
  MWL = 3,
  Makkah = 4,
  Egypt = 5, // Best for Al-Tal / Syria
  Tehran = 7,
  Gulf = 8,
  London = 11,
  France = 12,
  Turkey = 13
};

class Athan
{
public:
  using str = util::static_str_base;

  Athan(const char *country, const char *city, CalculationMethod method)
      : _country(country), _city(city), _method(static_cast<i32>(method)), _flags(0)
  {
    _set_flag(Flags::loaded);
  }

  Athan() : _method(0), _flags(0)
  {
  }

  auto set_country(str country) noexcept -> void
  {
    _country = country;
    _check_init();
  }

  auto set_city(str city) noexcept -> void
  {
    _city = city;
    _check_init();
  }

  auto set_method(i32 method) noexcept -> void
  {
    _method = method;
    _check_init();
  }

  auto fetch_timings() -> bool
  {
    if (!_check_flag(Flags::loaded))
    {
      fmt::println(stderr, "Error: Athan object not fully initialized.");
      return false;
    }

    httplib::Client cli("https://api.aladhan.com");
    cli.set_follow_location(true);

    util::static_str<512> path;
    path.format("/v1/timingsByCity?city={}&country={}&method={}", _city.data(), _country.data(), _method);

    if (auto res = cli.Get(path.data()))
    {
      if (res->status == 200)
      {
        return _parse_response(res->body);
      }
      fmt::println(stderr, "HTTP ERROR: {}", res->body);
    }
    else
    {
      fmt::println("Network Error.");
    }
    return false;
  }

private:
  using fl_t = u8;
  util::static_str<100> _city;
  util::static_str<100> _country;
  i32 _method;
  fl_t _flags;

  enum class Flags : fl_t
  {
    loaded = 1 << 0
  };

  auto _parse_response(const std::string &body) -> bool
  {
    auto data = json::parse(body);
    if (!data.contains("data") || !data["data"].contains("timings"))
    {
      fmt::println("JSON Error");
      return false;
    }
    auto timings = data["data"]["timings"];

    fmt::print("\n--- Timings for {} ---\n", _city.data());
    fmt::print("Fajr:    {}\n", timings["Fajr"].get<std::string>());
    fmt::print("Dhuhr:   {}\n", timings["Dhuhr"].get<std::string>());
    fmt::print("Asr:     {}\n", timings["Asr"].get<std::string>());
    fmt::print("Maghrib: {}\n", timings["Maghrib"].get<std::string>());
    fmt::print("Isha:    {}\n", timings["Isha"].get<std::string>());
    fmt::print("All {}", data.dump(4));
    return true;
  }

  auto _set_flag(Flags f) noexcept -> void
  {
    _flags |= static_cast<fl_t>(f);
  }

  auto _check_flag(Flags f) noexcept -> bool
  {
    return (_flags & static_cast<fl_t>(f));
  }

  auto _check_init() noexcept -> void
  {
    if (_method > 0 && _country.is_empty() && _city.is_empty())
    {
      _set_flag(Flags::loaded);
    }
  }
};

auto main() -> i32
{
  Athan ath("Syria", "Al-Tal", CalculationMethod::Makkah);

  if (!ath.fetch_timings())
  {
    return 1;
  }

  return 0;
}
