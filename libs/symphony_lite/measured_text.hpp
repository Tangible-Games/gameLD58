#pragma once

#include <iostream>
#include <memory>
#include <stack>
#include <tuple>
#include <unordered_map>

#include "font.hpp"
#include "formatted_text.hpp"
#include "log.hpp"
#include "utf8.hpp"

namespace Symphony {
namespace Text {
namespace {
bool IsWhitespace(uint32_t code_position) {
  if (code_position == 32) {
    return true;
  }

  return false;
}
}  // namespace

struct MeasuredGlyph {
  int x{0};
  int y{0};
  uint32_t color{0xFFFFFFFF};
  int base{0};
  int line_x_advance_before_this_glyph{0};
  int line_width_before_this_glyph{0};
  Glyph glyph;
  Font* from_font{nullptr};
};

// See: https://www.angelcode.com/products/bmfont/doc/render_text.html.
struct MeasuredTextLine {
  HorizontalAlignment align{HorizontalAlignment::kLeft};
  Wrapping wrapping{Wrapping::kClip};
  int line_x_advance{0};
  int line_width{0};
  int line_height{0};
  int base{0};
  int align_offset{0};
  std::list<MeasuredGlyph> glyphs;
  std::unordered_map<Font*, std::list<MeasuredGlyph*>> font_to_glyph;
};

namespace {
MeasuredGlyph* addGlyph(MeasuredTextLine& measured_line, const Glyph& glyph) {
  measured_line.glyphs.push_back(MeasuredGlyph());
  auto* cur_measured_glyph_ptr = &measured_line.glyphs.back();

  if (measured_line.glyphs.size() == 1) {
    measured_line.line_x_advance = -glyph.x_offset;
  }

  cur_measured_glyph_ptr->glyph = glyph;

  cur_measured_glyph_ptr->line_x_advance_before_this_glyph =
      measured_line.line_x_advance;
  cur_measured_glyph_ptr->line_width_before_this_glyph =
      measured_line.line_width;
  measured_line.line_width = measured_line.line_x_advance +
                             cur_measured_glyph_ptr->glyph.x_offset +
                             cur_measured_glyph_ptr->glyph.texture_width;
  measured_line.line_x_advance += cur_measured_glyph_ptr->glyph.x_advance;

  return cur_measured_glyph_ptr;
}
}  // namespace

struct MeasuredText {
  std::list<MeasuredTextLine> measured_lines;
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
    auto* cur_measured_line_ptr = &result.measured_lines.back();

    cur_measured_line_ptr->align = paragraph.paragraph_parameters.align;
    cur_measured_line_ptr->wrapping = paragraph.paragraph_parameters.wrapping;

    auto paragraph_font_it = fonts.find(paragraph.font);
    if (paragraph_font_it != fonts.end()) {
      auto font_measurements = paragraph_font_it->second->GetFontMeasurements();
      result.measured_lines.back().line_height = font_measurements.line_height;
    }

    bool has_prev_not_whitespace = false;
    auto line_prev_whitespace_it = cur_measured_line_ptr->glyphs.end();

    for (const auto& style_run : paragraph.style_runs) {
      auto style_font_it = fonts.find(style_run.style.font);
      if (style_font_it == fonts.end()) {
        LOGE("[Symphony::Text::MeasuredText] Unknown font, paragraph: {}",
             paragraph_index);
        return std::nullopt;
      }

      uint32_t color = style_run.style.color;

      const char* text = style_run.text.data();
      size_t text_length = style_run.text.size();

      while (text_length) {
        auto utf_result = ParseUtf8Sequence<false>(text, text_length);
        if (!utf_result.code_position.has_value()) {
          LOGE(
              "[Symphony::Text::MeasuredText] Not a valid UTF-8 text, "
              "paragraph: {}",
              paragraph_index);
          return std::nullopt;
        }

        text += utf_result.parsed_sequence_length;
        text_length -= utf_result.parsed_sequence_length;

        auto* cur_measured_glyph_ptr = addGlyph(
            *cur_measured_line_ptr,
            style_font_it->second->GetGlyph(utf_result.code_position.value()));
        cur_measured_glyph_ptr->color = color;
        cur_measured_glyph_ptr->from_font = style_font_it->second.get();

        if (paragraph.paragraph_parameters.wrapping == Wrapping::kWordWrap) {
          if (IsWhitespace(cur_measured_glyph_ptr->glyph.code_position)) {
            if (has_prev_not_whitespace) {
              has_prev_not_whitespace = false;

              line_prev_whitespace_it = cur_measured_line_ptr->glyphs.end();
              --line_prev_whitespace_it;
            }
          } else {
            has_prev_not_whitespace = true;
            if (cur_measured_line_ptr->line_width > container_width) {
              if (line_prev_whitespace_it !=
                  cur_measured_line_ptr->glyphs.end()) {
                // Reduce current line width:
                cur_measured_line_ptr->line_width =
                    line_prev_whitespace_it->line_width_before_this_glyph;

                result.measured_lines.push_back(MeasuredTextLine());

                auto* next_measured_line_ptr = &result.measured_lines.back();
                next_measured_line_ptr->align =
                    paragraph.paragraph_parameters.align;

                // Find first not whitespace glyph:
                auto not_whitespace_it = line_prev_whitespace_it;
                while (not_whitespace_it !=
                           cur_measured_line_ptr->glyphs.end() &&
                       IsWhitespace(not_whitespace_it->glyph.code_position)) {
                  ++not_whitespace_it;
                }

                if (not_whitespace_it != cur_measured_line_ptr->glyphs.end()) {
                  for (auto it = not_whitespace_it;
                       it != cur_measured_line_ptr->glyphs.end(); ++it) {
                    auto* next_line_measured_glyph_ptr =
                        addGlyph(*next_measured_line_ptr, it->glyph);
                    next_line_measured_glyph_ptr->color = it->color;
                    next_line_measured_glyph_ptr->from_font = it->from_font;
                  }
                }

                cur_measured_line_ptr->glyphs.erase(
                    line_prev_whitespace_it,
                    cur_measured_line_ptr->glyphs.end());

                // New line:
                cur_measured_line_ptr = next_measured_line_ptr;

                has_prev_not_whitespace = false;
                line_prev_whitespace_it = next_measured_line_ptr->glyphs.end();
              }
            }
          }
        }
      }
    }

    ++paragraph_index;
  }

  for (auto& measured_line : result.measured_lines) {
    if (measured_line.align == HorizontalAlignment::kLeft) {
      measured_line.align_offset = 0;
    } else if (measured_line.align == HorizontalAlignment::kCenter) {
      measured_line.align_offset =
          (container_width - measured_line.line_width) / 2;
    } else if (measured_line.align == HorizontalAlignment::kRight) {
      measured_line.align_offset = container_width - measured_line.line_width;
    }

    for (auto& measured_glyph : measured_line.glyphs) {
      auto font_measurements = measured_glyph.from_font->GetFontMeasurements();
      measured_line.line_height =
          std::max(measured_line.line_height, font_measurements.line_height);
      measured_line.base = std::max(measured_line.base, font_measurements.base);
      measured_glyph.base = font_measurements.base;
    }

    int base = measured_line.base;
    for (auto& measured_glyph : measured_line.glyphs) {
      measured_glyph.x = measured_glyph.line_x_advance_before_this_glyph +
                         measured_glyph.glyph.x_offset;

      int above_base_height =
          measured_glyph.base - measured_glyph.glyph.y_offset;
      measured_glyph.y = base - above_base_height;

      measured_line.font_to_glyph[measured_glyph.from_font].push_back(
          &measured_glyph);
    }
  }

  return result;
}
}  // namespace Text
}  // namespace Symphony
