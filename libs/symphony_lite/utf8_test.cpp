#include "utf8.hpp"

#include <gtest/gtest.h>

using namespace Symphony::Text;

TEST(Utf8, ParsesLatin) {
  auto result = ParseUtf8Sequence<true>("a", 1);
  ASSERT_TRUE(result.code_position.has_value());
  ASSERT_EQ((int)result.code_position.value(), 97);
  ASSERT_EQ((int)result.parsed_sequence_length, 1);
}

TEST(Utf8, ParsesTwoByteSequencies) {
  // See: https://en.wikipedia.org/wiki/Cyrillic_script_in_Unicode.
  auto result = ParseUtf8Sequence<true>("Ж", 2);
  ASSERT_TRUE(result.code_position.has_value());
  ASSERT_EQ((int)result.code_position.value(), 0x0416);
  ASSERT_EQ((int)result.parsed_sequence_length, 2);
}

TEST(Utf8, ParsesThreeByteSequencies) {
  // See: https://en.wikipedia.org/wiki/List_of_Unicode_characters
  auto result = ParseUtf8Sequence<true>("ऄ", 3);
  ASSERT_TRUE(result.code_position.has_value());
  ASSERT_EQ((int)result.code_position.value(), 0x0904);
  ASSERT_EQ((int)result.parsed_sequence_length, 3);
}

TEST(Utf8, ParsesFourByteSequencies) {
  // See: https://en.wikipedia.org/wiki/List_of_Unicode_characters
  auto result = ParseUtf8Sequence<true>("𐍅", 4);
  ASSERT_TRUE(result.code_position.has_value());
  ASSERT_EQ((int)result.code_position.value(), 0x10345);
  ASSERT_EQ((int)result.parsed_sequence_length, 4);
}

TEST(Utf8, FailsToParseWhenSequenceIsShort1) {
  auto result = ParseUtf8Sequence<true>("Ж", 1);
  ASSERT_FALSE(result.code_position.has_value());
}

TEST(Utf8, FailsToParseWhenSequenceIsShort2) {
  auto result = ParseUtf8Sequence<true>("ऄ", 2);
  ASSERT_FALSE(result.code_position.has_value());
}

TEST(Utf8, FailsToParseWhenSequenceIsShort3) {
  auto result = ParseUtf8Sequence<true>("𐍅", 3);
  ASSERT_FALSE(result.code_position.has_value());
}
