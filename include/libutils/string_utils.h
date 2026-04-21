#pragma once
#include "core.h"

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

  A_inline bool is_empty() noexcept
  {
    return length() == 0;
  }

  A_inline bool copy(const static_str_base &other) noexcept
  {
    clear();
    return add(other);
  }

  A_inline static_str_base &operator=(const static_str_base &other) noexcept
  {
    copy(other);
    return *this;
  }

  operator bool() noexcept
  {
    return is_empty();
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

    size_t written = (result.size > left) ? left : result.size;
    _size += written;

    _terminate();
    return result;
  }
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
    const __m128i floor_A = _mm_set1_epi8('A' - 1);
    const __m128i ceil_Z = _mm_set1_epi8('Z' + 1);
    const __m128i offset = _mm_set1_epi8(0x20);

    for (; i + 15 < _size; i += 16)
    {
      __m128i chunk = _mm_loadu_si128(reinterpret_cast<__m128i *>(_ptr + i));
      __m128i is_gt_A = _mm_cmpgt_epi8(chunk, floor_A);
      __m128i is_lt_Z = _mm_cmplt_epi8(chunk, ceil_Z);
      __m128i mask = _mm_and_si128(is_gt_A, is_lt_Z);
      __m128i add_values = _mm_and_si128(mask, offset);
      chunk = _mm_add_epi8(chunk, add_values);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(_ptr + i), chunk);
    }
    to_lower_arth(i);
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

  static_str(const static_str_base &other) noexcept : static_str()
  {
    add(other);
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
} // namespace util
