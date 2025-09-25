#include "measured_text.hpp"

#include <gtest/gtest.h>

using namespace Symphony::Text;

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
  auto result = MeasureText(/*container_width*/ 240, formatted_text.value(),
                            /*variables*/ {},
                            {{"mono_24", mono_24}, {"mono_32", mono_32}});

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ((int)result->measured_lines.size(), 1);
  ASSERT_EQ((int)result->measured_lines[0].glyphs.size(), 44);
  ASSERT_EQ(result->measured_lines[0].align_offset, 0);

  const auto& mono_24_glyphs =
      result->measured_lines[0].font_to_glyph_index[mono_24.get()];
  ASSERT_EQ((int)mono_24_glyphs.size(), 24);
  ASSERT_EQ((int)mono_24_glyphs[0], 0);
  ASSERT_EQ((int)mono_24_glyphs[23], 23);
  const auto& mono_32_glyphs =
      result->measured_lines[0].font_to_glyph_index[mono_32.get()];
  ASSERT_EQ((int)mono_32_glyphs.size(), 20);
  ASSERT_EQ((int)mono_32_glyphs[0], 24);
  ASSERT_EQ((int)mono_32_glyphs[19], 43);
}

TEST(MeasuredText, DISABLED_SingleParagraphManyLines) {
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
  ASSERT_EQ((int)result->measured_lines.size(), 5);
}
