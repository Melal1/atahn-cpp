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

namespace util
{
A_inline void copy_str(const char *src, char *dest, uint16_t max_size) noexcept
{
  if (max_size == 0)
    return;

  const size_t copy_size = std::min<size_t>(strlen(src), max_size - 1);

  std::memcpy(dest, src, copy_size);
  dest[copy_size] = '\0';
}

/*
 * @brief Logic engine for fixed-capacity strings.
 * Note: This class doesn't own memory; it just manages a buffer provided to it.
 */
class static_str_base
{
public:
  /** @brief Number of chars currently stored. @return size_t */
  size_t length() const noexcept
  {
    return _size;
  }

  /** @brief Max chars allowed (room for null terminator is handled internally). @return size_t */
  size_t capacity() const noexcept
  {
    return _cap;
  }

  /** @brief Space left . @return size_t */
  size_t remaining() const noexcept
  {
    return _cap - _size;
  }

  /** @brief Access the raw null-terminated string. @return const char* */
  const char *data() const noexcept
  {
    return _ptr;
  }

  /*
   *@brief add fill char to the string with n of c by std::memset will trancuate if don't fit.
   *@return if didn't trancuate true else false.
   */
  bool fill(char c, size_t n) noexcept
  {
    if (n == 0)
      return true;

    bool full = true;
    if (n > _cap)
    {
      n = _cap;
      full = false;
    }

    std::memset(_ptr, c, n);

    _size = n;
    _terminate();
    return full;
  }

  /*
   *@brief Fill the string with n of c by std::memset will trancuate if don't fit.
   *@return if didn't trancuate true else false.
   */
  bool append_fill(char c, size_t n) noexcept
  {
    if (n == 0)
      return true;

    bool full = n <= remaining();
    if (!full)
      n = remaining();

    std::memset(_ptr + _size, c, n);

    _size += n;
    _terminate();
    return full;
  }

  /** @brief Access the raw buffer for direct editing. @return char* */
  char *data() noexcept
  {
    return _ptr;
  }

  /*
   * @brief Appends a string. Truncates if it doesn't fit.
   * @return true if it fit perfectly, false if it was cut short.
   */
  bool add(const char *str) noexcept
  {
    return add(str, strlen(str));
  }

  /*
   * @brief Appends a single character.
   * @return true if added, false if buffer is at max capacity.
   */
  bool add(char c) noexcept
  {
    if (_size >= _cap)
      return false;
    _ptr[_size++] = c;
    _terminate();
    return true;
  }

  /*
   * @brief Appends a string with given size, Truncates if it doesn't fit.
   * @return true if it fit perfectly, false if it was cut short.
   */
  bool add(const char *str, size_t size) noexcept
  {
    bool full = true;
    if (size > remaining())
    {
      size = _cap - _size;
      full = false;
    }
    std::memcpy(_ptr + _size, str, size);
    _size += size;
    _terminate();
    return full;
  }

  /*
   * @brief Appends a string , Truncates if it doesn't fit.
   * @return true if it fit perfectly, false if it was cut short.
   */
  bool add(const static_str_base &str) noexcept
  {
    return add(str.data(), str.length());
  }

  /** @brief Wipes the string (sets length to 0 and null-terminates). */
  void clear() noexcept
  {
    _size = 0;
    _terminate();
  }

  /** @brief make every char in the str lower case*/
  A_inline void to_lower() noexcept
  {
    if (length() >= 16)
      to_lower_simd();
    else
      to_lower_arth();
  }

  /** @brief make every char in the str lower case*/
  A_inline void to_upper() noexcept
  {
    if (length() >= 16)
      to_upper_simd();
    else
      to_upper_arth();
  }

  static_str_base &operator+=(const static_str_base &other) noexcept
  {
    add(other);
    return *this;
  }

  static_str_base &operator+=(const char *str) noexcept
  {
    add(str);
    return *this;
  }

  static_str_base &operator+=(char c) noexcept
  {
    add(c);
    return *this;
  }

  char &operator[](size_t i) noexcept
  {
    return _ptr[i];
  }

  const char &operator[](size_t i) const noexcept
  {
    return _ptr[i];
  }

  /** @brief Implicit conversion to string_view for easy printing/passing. */
  operator std::string_view() const noexcept
  {
    return {_ptr, _size};
  }

  bool compare(const char *other) const noexcept
  {
    return std::string_view(this->data(), this->_size) == std::string_view(other);
  }

  bool operator==(const char *other) const noexcept
  {
    return compare(other);
  }

  bool operator!=(const char *other) const noexcept
  {
    return !compare(other);
  }

  /** @brief Binary comparison against any string-like type. */
  bool operator==(std::string_view other) const noexcept
  {
    return std::string_view(*this) == other;
  }

  bool operator!=(std::string_view other) const noexcept
  {
    return !operator==(other);
  }

  char *begin()
  {
    return _ptr;
  }

  char *end()
  {
    return _ptr + _size;
  }

  const char *begin() const
  {
    return _ptr;
  }

  const char *end() const
  {
    return _ptr + _size;
  }

  friend std::istream &getline(std::istream &input_stream, util::static_str_base &destination, char delimiter = '\n')
  {

    std::istream::sentry readiness_check(input_stream, true);

    if (readiness_check)
    {
      destination.clear();

      std::streambuf *raw_buffer = input_stream.rdbuf();

      int current_char = raw_buffer->sgetc();

      while (destination.length() < destination.capacity())
      {

        if (current_char == std::char_traits<char>::eof())
        {
          input_stream.setstate(std::ios_base::eofbit);
          break;
        }

        if (current_char == static_cast<int>(delimiter))
        {
          raw_buffer->sbumpc();
          break;
        }

        destination.data()[destination._size++] = static_cast<char>(current_char);

        current_char = raw_buffer->snextc();
      }

      if (destination.length() == 0 && input_stream.eof())
      {
        input_stream.setstate(std::ios_base::failbit);
      }
    }
    destination._terminate();
    return input_stream;
  }

  /**
   * @brief Overload for 'cin >> my_static_str'
   * Skips whitespace and reads one "word" into the buffer.
   */
  friend std::istream &operator>>(std::istream &input_stream, util::static_str_base &dest)
  {
    std::istream::sentry readiness_check(input_stream, false);

    if (readiness_check)
    {
      dest.clear();
      auto *raw = input_stream.rdbuf();
      int current_char = raw->sgetc();

      while (dest.length() < dest.capacity())
      {
        if (current_char == std::char_traits<char>::eof())
        {
          input_stream.setstate(std::ios_base::eofbit);
          break;
        }

        if (std::isspace(static_cast<unsigned char>(current_char)))
        {
          break;
        }

        dest.add(static_cast<char>(current_char));
        current_char = raw->snextc();
      }

      if (dest.length() == 0)
      {
        input_stream.setstate(std::ios_base::failbit);
      }
    }
    dest._terminate();
    return input_stream;
  }

#if HAS_FORMAT
  template <typename... Args> //
  auto format(FMT_TYPE::format_string<Args...> fmt_str, Args &&...args) noexcept
  {
    auto result = FMT_TYPE::format_to_n(_ptr, _cap, fmt_str, std::forward<Args>(args)...);
    _size = (result.size > _cap) ? _cap : result.size;
    _terminate();
    return result;
  }

  template <typename... Args> //
  auto append_fmt(FMT_TYPE::format_string<Args...> fmt_str, Args &&...args) noexcept
  {
    size_t left = remaining();
    auto result = FMT_TYPE::format_to_n(_ptr + _size, left, fmt_str, std::forward<Args>(args)...);
    // this returns the fmt_str string size not what written

    size_t written = (result.size > left) ? left : result.size;
    _size += written;

    _terminate();
    return result;
  }
#else
#endif

protected:
  static_str_base(char *p, size_t c, size_t initial_len = 0) noexcept : _ptr(p), _cap(c), _size(initial_len)
  {
  }

  char *const _ptr;
  const size_t _cap;
  size_t _size;

  inline void _terminate() noexcept
  {
    _ptr[_size] = '\0';
  }

private:
  void to_lower_simd() noexcept
  {
    size_t i = 0;

    // 1. Setup our "Stencils" (Constants)
    // Fill 16 slots with 64 ('A' - 1)
    const __m128i floor_A = _mm_set1_epi8('A' - 1);
    // Fill 16 slots with 91 ('Z' + 1)
    const __m128i ceil_Z = _mm_set1_epi8('Z' + 1);
    // Fill 16 slots with 32 (The offset to go from Upper to Lower)
    const __m128i offset = _mm_set1_epi8(0x20);

    // Loop through the string 16 characters at a time
    for (; i + 15 < _size; i += 16)
    {
      // A. LOAD: Grab 16 bytes from memory into one giant register
      __m128i chunk = _mm_loadu_si128(reinterpret_cast<__m128i *>(_ptr + i));

      // B. COMPARE FLOOR: Check which chars are > 64.
      // Result is 255 for "Yes", 0 for "No" in each of the 16 slots.
      __m128i is_gt_A = _mm_cmpgt_epi8(chunk, floor_A);

      // C. COMPARE CEILING: Check which chars are < 91.
      // Result is 255 for "Yes", 0 for "No" in each of the 16 slots.
      __m128i is_lt_Z = _mm_cmplt_epi8(chunk, ceil_Z);

      // D. CREATE MASK: Combine both checks using AND.
      // Only slots that are between 'A' and 'Z' will stay 255.
      __m128i mask = _mm_and_si128(is_gt_A, is_lt_Z);

      // E. FILTER OFFSET: Use the mask to pick where to apply the "32".
      // (255 AND 32 = 32) | (0 AND 32 = 0)
      __m128i add_values = _mm_and_si128(mask, offset);

      // F. APPLY MATH: Add the filtered offsets to the original chunk.
      // Uppercase letters get +32, everything else gets +0.
      chunk = _mm_add_epi8(chunk, add_values);

      // G. STORE: Put all 16 modified characters back into the string.
      _mm_storeu_si128(reinterpret_cast<__m128i *>(_ptr + i), chunk);
    }

    // 2. TAIL END CLEANUP
    // If the string length isn't a perfect multiple of 16,
    // we handle the last few characters the old-fashioned way.
    to_lower_arth(i);

    // Example:
    // Char:     A    b    c    1         {    d    E    }         f    G    2    !         h
    // Value:   65   98   99   49   32  123  100   69  125   32  102   71   50   33   32  104
    // ------------------------------------------------------------------------------------
    // is_gt_a: 00  255  255   00   00  255  255   00  255   00  255   00   00   00   00  255

    // Char:     A    b    c    1         {    d    E    }         f    G    2    !         h
    // Value:   65   98   99   49   32  123  100   69  125   32  102   71   50   33   32  104
    // ------------------------------------------------------------------------------------
    // is_lt_z: 255  255  255  255  255   00  255  255   00  255  255  255  255  255  255  255

    // im_gt_a: 00  255  255   00   00  255  255   00  255   00  255   00   00   00   00  255
    // is_lt_z: 255  255  255  255  255   00  255  255   00  255  255  255  255  255  255  255
    // ------------------------------------------------------------------------------------
    // Mask(AND):00  255  255   00   00   00  255   00   00   00  255   00   00   00   00  255

    // __m128i add_values = _mm_and_si128(mask, offset);
    // this will do the above but with the offset 32 (0x20) so and the mask slots with 16 slot of 32(0x20)  ,
    // if the mask slot is 255(0xFF) then we are anding 255(0xFF) with 32(0x20) then the offset slot is 32 , if the mask
    // slot is then it's anding 0 with 32 then the offset slot is 0

    // why the 0xFF AND 0x20 = 0x20 , it's because 0xFF in bin is 11111111 , 0x20 is 00100000
    // ANDing them is 00100000
  }

  void to_upper_simd() noexcept
  {
    size_t i = 0;
    const __m128i lower_a = _mm_set1_epi8('a' - 1);
    const __m128i lower_z = _mm_set1_epi8('z' + 1);
    const __m128i lut = _mm_set1_epi8(0x20);

    for (; i + 15 < _size; i += 16)
    {
      __m128i chunk = _mm_loadu_si128(reinterpret_cast<__m128i *>(_ptr + i));

      __m128i mask = _mm_and_si128(_mm_cmpgt_epi8(chunk, lower_a), _mm_cmplt_epi8(chunk, lower_z));

      chunk = _mm_sub_epi8(chunk, _mm_and_si128(mask, lut));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(_ptr + i), chunk);
    }
    to_upper_arth(i);
  }

  void to_lower_arth(size_t index = 0) noexcept
  {
    for (size_t i = index; i < _size; ++i)
    {
      unsigned char c = _ptr[i];
      if (c >= 'A' && c <= 'Z')
        _ptr[i] = c + 32;
    }
  }

  void to_upper_arth(size_t index = 0) noexcept
  {
    for (size_t i = index; i < _size; ++i)
    {
      unsigned char c = _ptr[i];
      if (c >= 'a' && c <= 'z')
        _ptr[i] = c - 32;
    }
  }
};

/*
 * @brief A string that lives entirely on the stack.
 * @tparam T Total buffer size (e.g., 32 includes the \0).
 */
template <size_t T> //
class static_str : public static_str_base
{
  static_assert(T > 0, "Buffer must be at least 1 byte for null terminator.");

public:
  static_str(const static_str &other) noexcept : static_str()
  {
    add(other.data(), other.length());
  }

  template <size_t N> //
  static_str(const static_str<N> &other) noexcept : static_str()
  {
    add(other.data(), other.length());
  }

  static_str() noexcept : static_str_base(_storage, T - 1)
  {
    _storage[0] = '\0';
  }

  /** @brief Construct and initialize with a C-string. */
  static_str(const char *str) noexcept : static_str()
  {
    add(str);
  }

  /** @brief Construct and initialize with a C-string , will copy n to it */
  static_str(const char *str, size_t n) noexcept : static_str()
  {
    add(str, n);
  }

private:
  char _storage[T];
};

/**
 * @brief Use this to wrap char array as a smart string, it will wipes the array !! .
 */
struct wrap_str : public static_str_base
{
  /** * @param p The array to use.
   * @param total_size Size of array (e.g., sizeof(my_array)).
   */
  wrap_str(char *p, size_t total_size) noexcept : static_str_base(p, total_size - 1)
  {
    clear();
  }
};

/*
 * @brief Use this to wrap an char array that already has text in it as smart string.
 */
struct adopt_str : public static_str_base
{
  /*
   * @param p Array with an existing C-string.
   * @param total_size Size of array ( sizeof(my_array)).
   * @note USING IT WITH arr[SIZE]; is UB , must atleast arr[SIZE] = "";.
   */
  adopt_str(char *p, size_t total_size) noexcept : static_str_base(p, total_size - 1)
  {
    _size = std::strlen(p);

    if (_size > _cap)
    {
      _size = _cap;
      _terminate();
    }
  }
};

enum class CharType
{
  small = 0,
  capital,
  digit,
  special,
  mix
};

// @brief seed the rand function with the current time with high precision time
void seed_rand() noexcept;

A_inline void seed_rand(uint32_t seed) noexcept
{
  srand(seed);
}

// @brief generate a random number in the range [min, max]
AD_inline uint32_t ranged_random_num(uint32_t min, uint32_t max) noexcept
{
  if (max < min)
    std::swap(max, min);
  return (rand() % (max - min + 1)) + min;
}

// @brief generate a random character of the specified type
NDES char get_rand_char(CharType type) noexcept;

[[nodiscard]]
std::string gen_rand_word(CharType type, uint32_t length) noexcept;

/*
   @brief generate a random word of the specified type and length, and store it in the provided buffer.
   @param type the type of characters to include in the word
   @param length the length of the word to generate, please consider that extra \0 ( if null == true ).
   @param buffer a character array to store the generated word, must be at least length characters long, will null
   teminate the string if null is true.
   @param null whether to null terminate the string, if true the last character of the buffer will be set to \0, if

*/
void gen_rand_word(CharType type, size_t length, char buffer[], bool null = true) noexcept;

NDES std::string gen_key(CharType type) noexcept;

/*
  @brief generate a key of the specified type and store it in the provided buffer.
  @param type the type of characters to include in the key
  @param buffer a character array to store the  key, must be at least 20 characters long, will null terminate the
  string.
*/
void gen_key(CharType type, char buffer[20]) noexcept;

void gen_keys(CharType type, uint32_t count) noexcept;

/*
  @brief generate a key of the specified type and store it in the provided buffer.
  @param text the text to encrypt, will be modified in place.
  @param encryption_key the key to use for encryption, will be used as the shift value for the Caesar cipher.
*/
void encrypt_text(std::string &text, uint16_t encryption_key) noexcept;

/*
  @brief encrypt the provided text using a simple Caesar cipher with the provided encryption key.
  @param buffer a character array containing the text to encrypt, will be modified in place , will null terminate the
  string.
  @param length the length of the text to encrypt, please consider that extra \0.
  @param encryption_key the key to use for encryption, will be used as the shift value for the Caesar cipher.
*/
void encrypt_text(char buffer[], size_t length, uint16_t encryption_key) noexcept;

/*
   @brief encrypt the provided text using a simple Caesar cipher with the provided encryption key.
   @param text the text to encrypt.
   @param encryption_key the key to use for encryption, will be used as the shift value
   @return the encrypted text as a new string, the original text will not be modified.
*/
[[nodiscard]]
std::string get_encrypted_key(std::string_view text, uint16_t encryption_key) noexcept;

/*
  @brief decrypt the provided text using a simple Caesar cipher with the provided encryption key.
  @param buffer a character array containing the text to decrypt, will be modified in place , will null terminate the
  string.
  @param length the length of the text to decrypt, please consider that extra \0.
  @param encryption_key the key to use for decryption, will be used as the shift value for the Caesar cipher.
*/
void decrypt_text(char *buffer, size_t length, uint16_t encryption_key) noexcept;

/*
  @brief decrypt the provided text using a simple Caesar cipher with the provided encryption key.
  @param text the text to decrypt, will be modified in place.
  @param encryption_key the key to use for decryption, will be used as the shift value for the Caesar cipher.
*/
void decrypt_text(std::string &text, uint16_t encryption_key) noexcept;

/*
  @brief decrypt the provided text using a simple Caesar cipher with the provided encryption key.
  @return the decrypted text as a new string, the original text will not be modified.
*/
[[nodiscard]]
std::string get_decrypted_key(std::string_view text, uint16_t encryption_key) noexcept;

template <class T> AD_inline constexpr bool range(T min, T max, T value) noexcept
{
  static_assert(std::is_arithmetic_v<T>, "Value should be a number");
  return (value >= min && value <= max);
}

void num_to_words(size_t nume, static_str_base &buffer) noexcept;

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

// -------| Section  ( helpers  )|-------
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

static inline uint8_t handle_special_type()
{
  constexpr const std::string_view specials = "!\"#$%&'()*+,./:;<=>?@[\\]^_`{|}~";
  return specials[util::ranged_random_num(0, specials.size() - 1)];
}

} // namespace __detail

//----------------------

inline void util::seed_rand() noexcept
{
  seed_rand(std::chrono::high_resolution_clock::now().time_since_epoch().count());
}

inline char util::get_rand_char(CharType type) noexcept
{
  switch (type)
  {
  case CharType::small:
    return ranged_random_num('a', 'z');
  case CharType::capital:
    return ranged_random_num('A', 'Z');
  case CharType::digit:
    return ranged_random_num('0', '9');
  case CharType::special:
    return ::__detail::handle_special_type();
  case CharType::mix:
  {
    static constexpr CharType pool[] = {CharType::small, CharType::capital, CharType::digit, CharType::special};
    uint32_t s = ranged_random_num(0, 3);
    return get_rand_char(pool[s]);
  }
  default:
    return ' ';
  }
}

inline std::string util::gen_rand_word(CharType type, uint32_t length) noexcept
{
  std::string word;
  word.reserve(length);
  for (__decltype(length) i = 0; i < length; i++)
  {
    word.push_back(get_rand_char(type));
  }

  return word;
}

inline void util::gen_rand_word(CharType type, size_t length, char buffer[], bool null) noexcept
{
  if (length == 0)
    return;
  for (__decltype(length) i = 0; i < length - 1; i++)
  {
    buffer[i] = get_rand_char(type);
  }
  if (null)
    buffer[length - 1] = '\0';
}

inline std::string util::gen_key(CharType type) noexcept
{
#if HAS_FORMAT
  return FMT_TYPE::format(
      "{}-{}-{}-{}", gen_rand_word(type, 4), gen_rand_word(type, 4), gen_rand_word(type, 4), gen_rand_word(type, 4));
#else
  std::string buff;
  buff.reserve(3 + 4 * 12);
  buff += gen_rand_word(type, 4);
  buff += "-";
  buff += gen_rand_word(type, 4);
  buff += "-";
  buff += gen_rand_word(type, 4);
  buff += "-";
  buff += gen_rand_word(type, 4);
  return buff;
#endif
}

inline void util::gen_key(CharType type, char buffer[20]) noexcept
{
  buffer[19] = '\0';
  gen_rand_word(type, 5, buffer, false);
  buffer[4] = '-';
  buffer += 5;
  gen_rand_word(type, 5, buffer, false);
  buffer[4] = '-';
  buffer += 5;
  gen_rand_word(type, 5, buffer, false);
  buffer[4] = '-';
  buffer += 5;
  gen_rand_word(type, 5, buffer, false);
}

inline void util::gen_keys(CharType type, uint32_t count) noexcept
{
  char buffer[20];
  for (__decltype(count) i = 0; i < count; i++)
  {
    gen_key(type, buffer);
#if HAS_FORMAT
    FMT_TYPE::print("Key[{}]: {}\n", i + 1, buffer);
#else
    std::cout << "Key" << '[' << i + 1 << "]: " << buffer << '\n';
#endif
    if (i % 300 == 0)
      std::fflush(stdout);
  }
}

inline void util::encrypt_text(std::string &text, uint16_t encryption_key) noexcept
{
  for (size_t i = 0; i < text.size(); ++i)
    text[i] = static_cast<char>(text[i] + encryption_key);
}

inline void util::encrypt_text(char *buffer, size_t length, uint16_t encryption_key) noexcept
{
  for (size_t i = 0; i < length - 1; ++i)
    buffer[i] = static_cast<char>(buffer[i] + encryption_key);
  buffer[length - 1] = '\0';
}

inline std::string util::get_encrypted_key(std::string_view text, uint16_t encryption_key) noexcept
{
  std::string encrypted(text);
  encrypt_text(encrypted, encryption_key);
  return encrypted;
}

inline void util::decrypt_text(std::string &text, uint16_t encryption_key) noexcept
{
  for (size_t i = 0; i < text.size(); ++i)
    text[i] = static_cast<char>(text[i] - encryption_key);
}

inline void util::decrypt_text(char *buffer, size_t length, uint16_t encryption_key) noexcept
{
  for (size_t i = 0; i < length - 1; ++i)
    buffer[i] = static_cast<char>(buffer[i] - encryption_key);
  buffer[length - 1] = '\0';
}

inline std::string util::get_decrypted_key(std::string_view text, uint16_t encryption_key) noexcept
{
  std::string encrypted(text);
  decrypt_text(encrypted, encryption_key);
  return encrypted;
}

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
