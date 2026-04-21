#include "conversions.h"
#include <algorithm>
#include <cassert>
#include <array>

namespace vtk
{
  constexpr uint8_t BINARY64_ZERO_INDEX = 52;
  constexpr uint8_t BINARY64_UPPERCASE_A_INDEX = 0;
  constexpr uint8_t BINARY64_LOWERCASE_A_INDEX = 26;
  constexpr uint8_t BINARY64_PLUS_INDEX = 62;
  constexpr uint8_t BINARY64_SLASH_INDEX = 63;

  uint8_t convert_binary64_digit_to_byte(char value)
  {
    if ('+' == value)
    {
      return BINARY64_PLUS_INDEX;
    }
    else if ('/' == value)
    {
      return BINARY64_SLASH_INDEX;
    }
    else if ('0' <= value && value <= '9')
    {
      return value - '0' + BINARY64_ZERO_INDEX;
    }
    else if ('A' <= value && value <= 'Z')
    {
      return value - 'A' + BINARY64_UPPERCASE_A_INDEX;
    }
    else if ('a' <= value && value <= 'z')
    {
      return value - 'a' + BINARY64_LOWERCASE_A_INDEX;
    }
    else if ('=' == value)
    {
      return PADDING_BINARY64_DIGIT;
    }
    else
    {
      return INVALID_BINARY64_DIGIT;
    }
  }

  void convert_binary64_to_raw(
    const char *data_ptr, size_t data_len, std::vector<uint8_t> &out_buf
  ) {
    // see RFC 4648 <https://datatracker.ietf.org/doc/html/rfc4648#section-4>
    assert(
      (data_len & 3) == 0 && "binary64 data length should be divisible by 4"
    );

    const auto result_capacity = data_len * 3 / 4;

    out_buf.reserve(result_capacity);

    for (size_t i = 0; i < data_len; i += 4)
    {
      auto decoded_quad = std::array{
        convert_binary64_digit_to_byte(data_ptr[i + 0]),
        convert_binary64_digit_to_byte(data_ptr[i + 1]),
        convert_binary64_digit_to_byte(data_ptr[i + 2]),
        convert_binary64_digit_to_byte(data_ptr[i + 3]),
      };

      assert(
        std::all_of(
          decoded_quad.begin(), decoded_quad.end(),
          [](uint8_t value) { return value != INVALID_BINARY64_DIGIT; }
        ) &&
        decoded_quad[0] != PADDING_BINARY64_DIGIT &&
        decoded_quad[1] != PADDING_BINARY64_DIGIT && "invalid base64 quad"
      );

      out_buf.push_back((decoded_quad[0] << 2) | (decoded_quad[1] >> 4));

      if (PADDING_BINARY64_DIGIT != decoded_quad[2])
      {
        out_buf.push_back((decoded_quad[1] << 4) | (decoded_quad[2] >> 2));
      }
      else
      {
        out_buf.push_back(decoded_quad[1] << 4);
        break;
      }

      if (PADDING_BINARY64_DIGIT != decoded_quad[3])
      {
        out_buf.push_back((decoded_quad[2] << 6) | decoded_quad[3]);
      }
      else
      {
        out_buf.push_back(decoded_quad[2] << 6);
        break;
      }
    }
  }
} // namespace vtk
