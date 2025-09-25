#pragma once

namespace Symphony {
namespace Text {
class Font;

struct FontMeasurements {
  FontMeasurements() = default;

  FontMeasurements(int new_line_height, int new_base)
      : line_height(new_line_height), base(new_base) {}

  int line_height{0};
  int base{0};
};

struct Glyph {
  int texture_x{0};
  int texture_y{0};
  int texture_width{0};
  int texture_height{0};
  int x_offset{0};
  int y_offset{0};
  int x_advance{0};
  uint32_t code_position{0};
};

class Font {
 public:
  virtual ~Font() = default;

  virtual FontMeasurements GetFontMeasurements() const = 0;

  virtual Glyph GetGlyph(uint32_t code_position) const = 0;

  virtual void* GetTexture() = 0;
};
}  // namespace Text
}  // namespace Symphony
