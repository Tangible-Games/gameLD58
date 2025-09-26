#include "measured_text.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Symphony::Text;
using namespace testing;

class MonoFont : public Font {
 public:
  MonoFont() {}

  MonoFont(int line_height, int base, int width)
      : line_height_(line_height), base_(base), width_(width) {}

  FontMeasurements GetFontMeasurements() const override {
    FontMeasurements result;
    result.line_height = line_height_;
    result.base = base_;
    return result;
  }

  Glyph GetGlyph(uint32_t code_position) const override {
    Glyph result;
    result.texture_x = 0;
    result.texture_y = 0;
    result.texture_width = width_;
    result.texture_height = line_height_;
    result.x_offset = 0;
    result.y_offset = 0;
    result.x_advance = width_;
    result.code_position = code_position;
    return result;
  }

  void* GetTexture() override { return nullptr; }

 private:
  int line_height_{0};
  int base_{0};
  int width_{0};
};

std::string ToStringWhenAscii(const std::list<MeasuredGlyph*>& glyphs) {
  std::string result;
  result.reserve(glyphs.size());

  for (const auto* glyph : glyphs) {
    result.push_back(static_cast<char>(glyph->glyph.code_position));
  }

  return result;
}

TEST(MeasuredText, NoWrappingProducesSingleLine) {
  auto formatted_text = FormatText(
      "<style font=\"mono_24\">One two three four five <style "
      "font=\"mono_32\">six seven eight nine",
      Style("mono_24", 0xFFFF0000),
      ParagraphParameters(HorizontalAlignment::kLeft, Wrapping::kClip), {});

  std::shared_ptr<Font> mono_24 = std::make_shared<MonoFont>(
      /*new_line_height*/ 46, /*new_base*/ 40, /*width*/ 24);
  std::shared_ptr<Font> mono_32 = std::make_shared<MonoFont>(
      /*new_line_height*/ 52, /*new_base*/ 44, /*width*/ 32);
  auto result = MeasureText(/*container_width*/ 20, formatted_text.value(),
                            /*variables*/ {},
                            {{"mono_24", mono_24}, {"mono_32", mono_32}});

  ASSERT_TRUE(result.has_value());

  EXPECT_THAT(result->measured_lines,
              ElementsAreArray({Field(&MeasuredTextLine::glyphs, SizeIs(44))}));

  const auto& mono_24_glyphs =
      result->measured_lines.front().font_to_glyph[mono_24.get()];
  ASSERT_EQ(ToStringWhenAscii(mono_24_glyphs), "One two three four five ");

  const auto& mono_32_glyphs =
      result->measured_lines.front().font_to_glyph[mono_32.get()];
  ASSERT_EQ(ToStringWhenAscii(mono_32_glyphs), "six seven eight nine");
}

TEST(MeasuredText, SingleParagraphManyLines) {
  auto formatted_text = FormatText(
      "One two three four five six seven eight nine",
      Style("mono_24", 0xFFFF0000),
      ParagraphParameters(HorizontalAlignment::kLeft, Wrapping::kWordWrap), {});
  // One two
  // three four
  // five six
  // seven
  // eight nine

  std::shared_ptr<Font> mono_24 = std::make_shared<MonoFont>(
      /*new_line_height*/ 46, /*new_base*/ 40, /*width*/ 24);
  auto result = MeasureText(/*container_width*/ 240, formatted_text.value(),
                            /*variables*/ {}, {{"mono_24", mono_24}});

  ASSERT_TRUE(result.has_value());
  EXPECT_THAT(result->measured_lines,
              ElementsAreArray({
                  Field(&MeasuredTextLine::glyphs, SizeIs(7)),
                  Field(&MeasuredTextLine::glyphs, SizeIs(10)),
                  Field(&MeasuredTextLine::glyphs, SizeIs(8)),
                  Field(&MeasuredTextLine::glyphs, SizeIs(5)),
                  Field(&MeasuredTextLine::glyphs, SizeIs(10)),
              }));
}
