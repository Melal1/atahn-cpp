// Copyright (c) 2026 Basel Saramijou (github:melal1)
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "../include/salawat.h"
#include <chrono>
#include <string_view>
#include <unistd.h>

i32 status_fd = open("/dev/shm/prayer_status", O_WRONLY | O_CREAT | O_TRUNC, 0644);

void sleep_for(u32 sleep_for_sec, u32 &sec_l) noexcept
{
  if (sec_l == 0)
    return;

  if (sleep_for_sec >= sec_l)
  {
    sleep_for_sec = sec_l;
    sec_l = 0;
  }
  else
  {
    sec_l -= sleep_for_sec;
  }

  std::this_thread::sleep_for(std::chrono::seconds(sleep_for_sec));
}

inline void write_bar_status(std::string_view str)
{
  static us_t previous_len = 0;
  if (status_fd == -1)
    return;

  if (pwrite(status_fd, str.data(), str.length(), 0) <= 0)
    return;

  if (previous_len > str.length())
    if (ftruncate(status_fd, str.length()))
      return;
  previous_len = str.length();
}

void run_bar_mode(Salawat &ath)
{
  u32 sec_l = Time::now().seconds_till_end_of_day();

  util::static_str<512> buff;
  while (true)
  {
    auto next = ath.get_next_prayer();
    i64 diff = ath.seconds_until_next();

    for (i32 i = 0; i < 10 && sec_l > 0; ++i)
    {
      i64 h = diff / 3600;
      i64 m = (diff % 3600) / 60;
      i64 s = diff % 60;

      buff.format("{} in {}:{}:{:02}", next.name, h, m, s);
      write_bar_status(buff);

      sleep_for(1, sec_l);

      if (diff > 0)
      {
        diff--;
      }
    }

    if (sec_l > 0)
    {
      buff.format("{} at {:02}:{:02}", next.name, next.time.hour, next.time.min);
      write_bar_status(buff);
      sleep_for(10, sec_l);
    }

    if (sec_l == 0)
    {
      using namespace std::chrono_literals;
      using namespace std::chrono;
      ath.load();
      sec_l = seconds(24h).count();
    }
  }
}

auto main(i32 argc, char **argv) -> i32
{
  bool bar_mode = false;
  if (argc > 1 && (std::string_view(argv[1]) == "-b" || std::string_view(argv[1]) == "--bar"))
  {
    bar_mode = true;
  }

  Salawat ath("Syria", "Al-Tal", CalculationMethod::Makkah);
  auto print_err = [](const char *err) { FMT_TYPE::print(stderr, "Error: {}\n", err); };

  if (bar_mode)
  {
    while (ath.load(print_err) == Salawat::Result::err)
    {
      std::string_view s = "Athan: Offline";
      write_bar_status(s);
      std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    run_bar_mode(ath);
  }
  else
  {
    if (ath.load(print_err) == Salawat::Result::err)
    {
      return 1;
    }
    ath.print_timings();
  }

  return 0;
}
