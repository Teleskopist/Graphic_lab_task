#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace vtk
{
  // Switches byte order from little-endian to big-endian or from big-endian to
  // little-endian
  template <typename T>
  void switch_byte_order(T *ptr, size_t len)
  {
    static constexpr size_t N_BYTES = sizeof(T);
    static_assert(
      N_BYTES == 1 || N_BYTES == 2 || N_BYTES == 4 || N_BYTES == 8 ||
        N_BYTES == 16,
      "invalid type for byte-order switch"
    );

    for (size_t i = 0; i < len; ++i)
    {
      const auto elem_ptr = reinterpret_cast<uint8_t *>(ptr + i);

      for (size_t j = 0; j < N_BYTES / 2; ++j)
      {
        std::swap(elem_ptr[j], elem_ptr[sizeof(T) - j - 1]);
      }
    }
  }

  constexpr uint8_t INVALID_BINARY64_DIGIT = 0xFF;
  constexpr uint8_t PADDING_BINARY64_DIGIT = 0xFE;

  uint8_t convert_binary64_digit_to_byte(char value);

  void convert_binary64_to_raw(
    const char *data_ptr, size_t data_len, std::vector<uint8_t> &out_buf
  );
} // namespace vtk
