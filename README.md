# Athan

Athan is a lightweight and blazing-fast C++ utility for fetching and displaying Islamic prayer times. It leverages the Al-Adhan API and provides a simple command-line interface, as well as a specialized daemon-like mode for system status bars.

## Features

- **Accurate Prayer Times**: Fetches timings for Fajr, Dhuhr, Asr, Maghrib, and Isha via the [Al-Adhan API](https://aladhan.com/api).
- **Efficient Caching**: Limits unnecessary API calls by caching the current day's prayer timings locally at `/tmp/athan_cache.json`.
- **Bar Mode**: Designed seamlessly for status bars like Polybar or Waybar. It continuously monitors the countdown to the next prayer and dynamically updates `/dev/shm/prayer_status` with live timing information.
- **Fast & Lightweight**: Built with performance in mind using C++20 and custom stack-allocated string helpers (`libutils`) for zero-allocation string manipulation and minimal memory footprint.

## Dependencies

- A C++20 compatible compiler.
- **External Dependencies**:
  - [`nlohmann/json`](https://github.com/nlohmann/json)
  - [`fmt`](https://github.com/fmtlib/fmt) (or native C++20 `<format>`)
  - OpenSSL (required for HTTPS support)
- *Note: `httplib` (cpp-httplib) is bundled directly within the project's source tree.*

## Usage

### Standard Run

Running `athan` directly will load the timings for the day and print them to the standard output:

```bash
$ athan
Fajr     : 05:30
Dhuhr    : 12:45
Asr      : 15:15
Maghrib  : 18:20
Isha     : 19:45
```

### Bar Mode

Bar mode allows `athan` to run as a continuous process, writing the current status (such as a countdown to the next prayer) to a shared memory file. 

```bash
$ athan -b
# or
$ athan --bar
```

This will frequently update `/dev/shm/prayer_status`. You can configure your status bar (like Polybar, Waybar, or i3blocks) to read from this file using a custom script or module that watches or periodically `cat`s its contents.

## Internal Architecture

- **`Salawat` Core**: Manages fetching, caching, and parsing the Al-Adhan JSON responses.
- **Custom `Date` / `Time` Types**: Efficiently represent calendar dates and daily times with standard math and helper methods.
- **`libutils`**: A lightweight library i wrote included for stack-allocated and static string management (`static_str`,  etc.), minimizing heap allocations during high-frequency status bar updates and more!

## License

This project is released under the MIT License.
