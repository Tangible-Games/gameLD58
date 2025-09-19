#pragma once

namespace Symphony {
namespace Text {
class Font;

struct Glyph {
  int texture_x{0};
  int texture_y{0};
  int texture_width{0};
  int texture_height{0};
  int x_offset{0};
  int y_offset{0};
  int x_advance{0};
  Font* from_font{nullptr};
};

class Font {
 public:
  virtual ~Font() = default;

  virtual int GetLineHeight() = 0;

  virtual int GetBase() = 0;

  virtual Glyph GetGlyph(uint32_t code_position) = 0;
};
}  // namespace Text
}  // namespace Symphony
