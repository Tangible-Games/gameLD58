#pragma once

#include <iostream>
#include <memory>
#include <tuple>

#include "font.hpp"
#include "formatted_text.hpp"
#include "utf8.hpp"

namespace Symphony {
namespace Text {

struct MeasuredGlyph {
  int x{0};
  int y{0};
  uint32_t color{0xFFFFFFFF};
  int base{0};
  Glyph glyph;
  Font* from_font{nullptr};
};

// See: https://www.angelcode.com/products/bmfont/doc/render_text.html.
struct MeasuredTextLine {
  int line_height{0};
  int base{0};
  int align_offset{0};
  std::vector<MeasuredGlyph> glyphs;
  std::unordered_map<Font*, std::vector<size_t>> font_to_glyph_index;
};

struct MeasuredText {
  std::vector<MeasuredTextLine> measured_lines;
  std::map<std::string, std::shared_ptr<Font>> fonts;
};

std::optional<MeasuredText> MeasureText(
    int container_width, const FormattedText& formatted_text,
    const std::map<std::string, std::string>& variables,
    const std::map<std::string, std::shared_ptr<Font>>& fonts) {
  (void)variables;

  MeasuredText result;
  result.fonts = fonts;

  if (formatted_text.paragraphs.empty()) {
    return result;
  }

  for (size_t paragraph_index = 0;
       const auto& paragraph : formatted_text.paragraphs) {
    result.measured_lines.push_back(MeasuredTextLine());
    if (paragraph.paragraph_parameters.wrapping == Wrapping::kClip ||
        paragraph.paragraph_parameters.wrapping == Wrapping::kNoClip) {
      int line_x_advance = 0;
      int line_width = 0;

      for (const auto& style_run : paragraph.style_runs) {
        auto font_it = fonts.find(style_run.style.font);
        if (font_it == fonts.end()) {
          std::cerr
              << "[Symphony::Text::MeasuredText] Unknown font, paragraph: "
              << paragraph_index << std::endl;
          return std::nullopt;
        }

        auto font_measurements = font_it->second->GetFontMeasurements();
        result.measured_lines.back().line_height =
            std::max(result.measured_lines.back().line_height,
                     font_measurements.line_height);
        result.measured_lines.back().base =
            std::max(result.measured_lines.back().base, font_measurements.base);

        uint32_t color = style_run.style.color;

        const char* text = style_run.text.data();
        size_t text_length = style_run.text.size();

        while (text_length) {
          auto utf_result = ParseUtf8Sequence<false>(text, text_length);
          if (!utf_result.code_position.has_value()) {
            std::cerr << "[Symphony::Text::MeasuredText] Not a valid UTF-8 "
                         "text, paragraph: "
                      << paragraph_index << std::endl;
            return std::nullopt;
          }

          text += utf_result.parsed_sequence_length;
          text_length -= utf_result.parsed_sequence_length;

          result.measured_lines.back().glyphs.push_back(MeasuredGlyph());
          MeasuredGlyph& measured_glyph =
              result.measured_lines.back().glyphs.back();
          measured_glyph.from_font = font_it->second.get();

          measured_glyph.color = color;
          measured_glyph.base = font_measurements.base;
          measured_glyph.glyph =
              font_it->second->GetGlyph(utf_result.code_position.value());

          if (result.measured_lines.back().glyphs.size() == 1) {
            line_x_advance = measured_glyph.glyph.x_offset;
          }

          measured_glyph.x = line_x_advance + measured_glyph.glyph.x_offset;

          line_width += line_x_advance + measured_glyph.glyph.x_offset +
                        measured_glyph.glyph.texture_width;
          line_x_advance += measured_glyph.glyph.x_advance;
        }
      }

      if (paragraph.paragraph_parameters.align == HorizontalAlignment::kLeft) {
        result.measured_lines.back().align_offset = 0;
      } else if (paragraph.paragraph_parameters.align ==
                 HorizontalAlignment::kCenter) {
        result.measured_lines.back().align_offset =
            (container_width - line_width) / 2;
      } else if (paragraph.paragraph_parameters.align ==
                 HorizontalAlignment::kRight) {
        result.measured_lines.back().align_offset =
            container_width - line_width;
      }

      int base = result.measured_lines.back().base;
      for (auto& measured_glyph : result.measured_lines.back().glyphs) {
        int above_base_height =
            measured_glyph.base - measured_glyph.glyph.y_offset;
        measured_glyph.y = base - above_base_height;
      }
    } else {
    }

    ++paragraph_index;
  }

  for (auto& measured_line : result.measured_lines) {
    for (size_t measured_glyph_index = 0;
         const auto& measured_glyph : measured_line.glyphs) {
      measured_line.font_to_glyph_index[measured_glyph.from_font].push_back(
          measured_glyph_index);
      ++measured_glyph_index;
    }
  }

  return result;
}
}  // namespace Text
}  // namespace Symphony

// Lorem ips|um dolor sit amet
