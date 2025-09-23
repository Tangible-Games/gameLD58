#pragma once

#include <memory>

#include "font.hpp"
#include "formatted_text.hpp"

namespace Symphony {
namespace Text {
namespace {
struct FormattedTextReader {
  size_t current_paragraph{0};
  size_t current_style_run{0};
  size_t current_char{0};
};
};  // namespace

struct MeasuredGlyph {
  int x{0};
  int y{0};
  uint32_t color{0xFFFFFFFF};
  Glyph glyph;
};

// See: https://www.angelcode.com/products/bmfont/doc/render_text.html.
struct MeasuredTextLine {
  int line_height{0};
  int base{0};
  std::vector<MeasuredGlyph> fonts;
};

struct MeasuredText {
  std::vector<MeasuredTextLine> measured_lines;
  std::map<std::string, std::shared_ptr<Font>> fonts;
};

MeasuredText MeasureText(
    int container_width, const FormattedText& formatted_text,
    const std::map<std::string, std::string>& variables,
    const std::map<std::string, std::shared_ptr<Font>>& fonts) {
  (void)container_width;
  (void)variables;
  MeasuredText result;
  result.fonts = fonts;

  for (const auto& paragraph : formatted_text.paragraphs) {
    result.measured_lines.push_back(MeasuredTextLine());

    auto paragraph_default_font_it = fonts.find(paragraph.font);
    if (paragraph_default_font_it != fonts.end()) {
      result.measured_lines.back().line_height =
          paragraph_default_font_it->second->GetLineHeight();
      result.measured_lines.back().base =
          paragraph_default_font_it->second->GetBase();
    } else {
      // Should very, very rarely be here.
      result.measured_lines.back().line_height = 20;
      result.measured_lines.back().base = 16;
    }
  }

  return result;
}
}  // namespace Text
}  // namespace Symphony

// Lorem ips|um dolor sit amet
