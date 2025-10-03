#pragma once

#include <bitset>
#include <cinttypes>
#include <optional>

namespace Symphony {
namespace Text {
struct UnicodeSequenceParseResult {
  std::optional<uint32_t> code_position;
  size_t parsed_sequence_length{0};
};

// See: https://en.wikipedia.org/wiki/UTF-8.
template <bool check_is_sequence_ill_formed>
UnicodeSequenceParseResult ParseUtf8Sequence(const char* sequence,
                                             size_t sequence_length) {
  UnicodeSequenceParseResult result;

  if (sequence_length == 0) {
    return result;
  }

  uint32_t first_byte = *(uint8_t*)sequence;
  std::bitset<8> first_byte_bits(first_byte);

  if (first_byte_bits[7] == 0) {
    result.code_position = first_byte;
    result.parsed_sequence_length = 1;
    return result;
  }

  if (sequence_length <= 1) {
    return result;
  }

  // Two bytes sequencies.
  if (first_byte_bits[7] == 1 && first_byte_bits[6] == 1 &&
      first_byte_bits[5] == 0) {
    uint32_t second_byte = *(uint8_t*)(sequence + 1);

    if (check_is_sequence_ill_formed) {
      std::bitset<8> second_byte_bits(second_byte);
      if (second_byte_bits[7] != 1 || second_byte_bits[6] != 0) {
        return result;
      }
    }

    result.code_position = ((first_byte & 0x1F) << 6) + (second_byte & 0x3f);
    result.parsed_sequence_length = 2;
    return result;
  }

  if (sequence_length <= 2) {
    return result;
  }

  // Three bytes sequencies.
  if (first_byte_bits[7] == 1 && first_byte_bits[6] == 1 &&
      first_byte_bits[5] == 1 && first_byte_bits[4] == 0) {
    uint32_t second_byte = *(uint8_t*)(sequence + 1);
    uint32_t third_byte = *(uint8_t*)(sequence + 2);

    if (check_is_sequence_ill_formed) {
      std::bitset<8> second_byte_bits(second_byte);
      std::bitset<8> third_byte_bits(third_byte);

      if (second_byte_bits[7] != 1 && second_byte_bits[6] != 0 &&
          third_byte_bits[7] != 1 && third_byte_bits[6] != 0) {
        return result;
      }
    }

    result.code_position = ((first_byte & 0xF) << 12) +
                           ((second_byte & 0x3f) << 6) + (third_byte & 0x3f);
    result.parsed_sequence_length = 3;
    return result;
  }

  if (sequence_length <= 3) {
    return result;
  }

  // Four bytes sequencies.
  if (first_byte_bits[7] == 1 && first_byte_bits[6] == 1 &&
      first_byte_bits[5] == 1 && first_byte_bits[4] == 1 &&
      first_byte_bits[3] == 0) {
    uint32_t second_byte = *(uint8_t*)(sequence + 1);
    uint32_t third_byte = *(uint8_t*)(sequence + 2);
    uint32_t fourth_byte = *(uint8_t*)(sequence + 3);

    if (check_is_sequence_ill_formed) {
      std::bitset<8> second_byte_bits(second_byte);
      std::bitset<8> third_byte_bits(third_byte);
      std::bitset<8> fourth_byte_bits(fourth_byte);

      if (second_byte_bits[7] != 1 || second_byte_bits[6] != 0 ||
          third_byte_bits[7] != 1 || third_byte_bits[6] != 0 ||
          fourth_byte_bits[7] != 1 || fourth_byte_bits[6] != 0) {
        return result;
      }
    }

    result.code_position = ((first_byte & 0x7) << 18) +
                           ((second_byte & 0x3f) << 12) +
                           ((third_byte & 0x3f) << 6) + (fourth_byte & 0x3f);
    result.parsed_sequence_length = 4;
    return result;
  }

  return result;
}
}  // namespace Text
}  // namespace Symphony
