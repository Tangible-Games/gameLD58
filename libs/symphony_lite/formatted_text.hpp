#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

namespace Symphony {
namespace {
struct ParseErrorSource {
  std::string source;
  std::string marker;
};

static const size_t kParseErrorSourceBefore = 30;
static const size_t kParseErrorSourceAfter = 20;

bool isLineBreak(int c) {
  if (c == '\n') {
    return true;
  }
  return false;
}

ParseErrorSource MakeParseErrorSource(std::istringstream& input_stream) {
  ParseErrorSource result;

  const std::string& input = input_stream.str();
  size_t at = input_stream.tellg();

  result.source.resize(kParseErrorSourceBefore + 1 + kParseErrorSourceAfter,
                       ' ');
  result.marker.resize(result.source.size(), ' ');

  for (int i = (int)kParseErrorSourceBefore; i >= 0; --i) {
    int input_index = at - (kParseErrorSourceBefore + 1 - i);
    if (input_index < 0) {
      break;
    }
    if (isLineBreak(input[input_index])) {
      break;
    }
    if (input[input_index] == '\t') {
      break;
    }
    result.source[i] = input[input_index];
  }

  for (size_t i = (int)kParseErrorSourceBefore + 1;; ++i) {
    size_t input_index = at - (kParseErrorSourceBefore + 1 - i);
    if (input_index >= input.size()) {
      break;
    }
    result.source[i] = input[input_index];
  }

  for (size_t i = 0; i < kParseErrorSourceBefore; ++i) {
    result.marker[i] = '-';
  }
  result.marker[kParseErrorSourceBefore] = '^';

  return result;
}

void PrintParseError(std::istringstream& input_stream,
                     const std::string& error_message) {
  std::cerr << error_message << std::endl;
  auto parse_error_source = MakeParseErrorSource(input_stream);
  std::cerr << parse_error_source.source << std::endl;
  std::cerr << parse_error_source.marker << std::endl;
}

// TODO(truvorskameikin): Implement parsing AARRBBGG colors.
std::optional<uint32_t> ColorFromString(const std::string& value) {
  if (value == "red") {
    return 0xFFF00F13;
  } else if (value == "green") {
    return 0xFF29C41B;
  } else if (value == "blue") {
    return 0xFF2B7FEE;
  } else if (value == "black") {
    return 0xFF000000;
  } else if (value == "white") {
    return 0xFFFFFFFF;
  } else if (value == "grey") {
    return 0xFFBFC2C7;
  }

  return std::nullopt;
}

enum Tag {
  kTagP,
  kTagStyle,
  kTagSub,
};
}  // namespace

namespace Text {
// There are two tags:
// 1. <style> tag has the following parameters:
//   - font="font_name.fnt|$variable_name"
//   - color="AARRGGBB|$variable_name"
//   - align="left|right|center|$variable_name"
//   - wrapping="word|clip|noclip|$variable_name"
// 2. <sub> tag has only one parameter: variable="$variable_name".
//
// <style> tag has closing tag </>.
// If new <style> tag changes align - new paragraph is created.
//
// Line break creates new paragraph.
enum class HorizontalAlignment {
  kLeft,
  kRight,
  kCenter,
};

std::optional<HorizontalAlignment> HorizontalAlignmentFromString(
    const std::string& value) {
  if (value == "left") {
    return HorizontalAlignment::kLeft;
  } else if (value == "right") {
    return HorizontalAlignment::kRight;
  } else if (value == "center") {
    return HorizontalAlignment::kCenter;
  }
  return std::nullopt;
}

enum class Wrapping {
  kWordWrap,
  kClip,
  kNoClip,
};

std::optional<Wrapping> WrappingFromString(const std::string& value) {
  if (value == "word") {
    return Wrapping::kWordWrap;
  } else if (value == "clip") {
    return Wrapping::kClip;
  } else if (value == "noclip") {
    return Wrapping::kNoClip;
  }
  return std::nullopt;
}

struct ParagraphParameters {
  ParagraphParameters() = default;

  ParagraphParameters(HorizontalAlignment new_align, Wrapping new_wrapping)
      : align(new_align), wrapping(new_wrapping) {}

  HorizontalAlignment align{HorizontalAlignment::kLeft};
  Wrapping wrapping{Wrapping::kClip};
};

struct Style {
  Style() = default;

  Style(const std::string& new_font, uint32_t new_color)
      : font(new_font), color(new_color) {}

  std::string font;
  uint32_t color;
};

namespace {
struct StyleWithParagraphParameters {
  // We can optimize here, use not font name, but pointer to GlyphLibrary.
  std::optional<std::string> font_opt;
  std::optional<uint32_t> color_opt;
  std::optional<HorizontalAlignment> align_opt;
  std::optional<Wrapping> wrapping_opt;
};

Style StyleFromStyleWithParagraphParameters(
    const StyleWithParagraphParameters& value) {
  Style result;
  if (value.font_opt) {
    result.font = value.font_opt.value();
  }
  if (value.color_opt) {
    result.color = value.color_opt.value();
  }
  return result;
}

StyleWithParagraphParameters
StyleWithParagraphParametersFromStyleAndParagraphParameters(
    const Style& style, ParagraphParameters paragraph_parameters) {
  StyleWithParagraphParameters result;
  result.font_opt = style.font;
  result.color_opt = style.color;
  result.align_opt = paragraph_parameters.align;
  result.wrapping_opt = paragraph_parameters.wrapping;
  return result;
}
}  // namespace

struct StyleRun {
  Style style;
  // TODO(truvorskameikin): Switch to string_view here.
  std::string text;
};

struct Paragraph {
  Paragraph() = default;

  Paragraph(StyleWithParagraphParameters style_with_alignment)
      : font(style_with_alignment.font_opt.value_or("")),
        paragraph_parameters(
            style_with_alignment.align_opt.value_or(HorizontalAlignment::kLeft),
            style_with_alignment.wrapping_opt.value_or(Wrapping::kClip)) {
    style_runs.push_back(StyleRun());
    style_runs.back().style =
        StyleFromStyleWithParagraphParameters(style_with_alignment);
  }

  std::string font;
  ParagraphParameters paragraph_parameters;
  // TODO(truvorskameikin): Switch to using lists.
  std::vector<StyleRun> style_runs;
};

struct FormattedText {
  // TODO(truvorskameikin): Switch to using lists.
  std::vector<Paragraph> paragraphs;
};

std::optional<FormattedText> FormatText(
    const std::string& input, const Style& default_style,
    const ParagraphParameters& default_paragraph_parameters,
    const std::map<std::string, std::string>& variables) {
  FormattedText result;

  StyleWithParagraphParameters default_style_with_aligment =
      StyleWithParagraphParametersFromStyleAndParagraphParameters(
          default_style, default_paragraph_parameters);

  result.paragraphs.push_back(Paragraph(default_style_with_aligment));

  std::stack<StyleWithParagraphParameters> styles_stack;
  styles_stack.push(default_style_with_aligment);

  std::istringstream input_stream(input);
  while (!input_stream.eof()) {
    int next_char = input_stream.peek();

    if (next_char == '<') {
      input_stream.get();

      next_char = input_stream.peek();
      if (next_char == '<') {
        input_stream.get();

        result.paragraphs.back().style_runs.back().text.push_back('<');
        continue;
      } else if (next_char == '/') {
        input_stream.get();
        next_char = input_stream.get();
        if (next_char != '>') {
          PrintParseError(input_stream,
                          "[Symphony::Text::FormattedText] Bad closing tag:");
          return std::nullopt;
        } else {
          // Closing tag.
          // Note: styles_stack has also default style.
          if (styles_stack.size() == 1) {
            PrintParseError(input_stream,
                            "[Symphony::Text::FormattedText] Bad closing tag"
                            " (only <style> tags have closing tag option):");
            return std::nullopt;
          }

          // Note: styles_stack has also default style.
          if (styles_stack.size() > 1) {
            styles_stack.pop();

            if (!result.paragraphs.back().style_runs.back().text.empty()) {
              result.paragraphs.back().style_runs.push_back(StyleRun());
              result.paragraphs.back().style_runs.back().style =
                  StyleFromStyleWithParagraphParameters(styles_stack.top());
            }
          }
        }

        continue;
      }

      // Opening tag.

      std::string tag_name;
      std::getline(input_stream, tag_name, ' ');

      Tag tag;
      if (tag_name == "style") {
        tag = kTagStyle;
      } else if (tag_name == "sub") {
        tag = kTagSub;
      } else {
        input_stream.seekg(-tag_name.size(), std::ios::cur);

        PrintParseError(input_stream,
                        "[Symphony::Text::FormattedText] Unknown tag:");
        return std::nullopt;
      }

      StyleWithParagraphParameters style_with_alignment = styles_stack.top();
      std::string variable_value;

      while (input_stream.peek() != '>') {
        while (input_stream.peek() == ' ') {
          input_stream.get();
        }

        std::string key;
        std::getline(input_stream, key, '=');

        next_char = input_stream.get();
        if (next_char != '\"') {
          PrintParseError(
              input_stream,
              "[Symphony::Text::FormattedText] Bad tag's parameters "
              "formatting (requires \"\" for values):");
          return std::nullopt;
        }

        std::string value;
        std::getline(input_stream, value, '\"');

        if (tag == kTagStyle) {
          if (key == "font") {
            style_with_alignment.font_opt = value;
          } else if (key == "color") {
            auto color_opt = ColorFromString(value);
            if (!color_opt) {
              PrintParseError(input_stream,
                              "[Symphony::Text::FormattedText] Can't read "
                              "color parameter:");
              return std::nullopt;
            }
            style_with_alignment.color_opt = color_opt.value();
          } else if (key == "align") {
            auto align_opt = HorizontalAlignmentFromString(value);
            if (!align_opt) {
              PrintParseError(input_stream,
                              "[Symphony::Text::FormattedText] Can't read "
                              "align parameter:");
              return std::nullopt;
            }
            style_with_alignment.align_opt = align_opt.value();
          } else if (key == "wrapping") {
            auto wrapping_opt = WrappingFromString(value);
            if (!wrapping_opt) {
              PrintParseError(input_stream,
                              "[Symphony::Text::FormattedText] Can't read "
                              "wrapping parameter:");
              return std::nullopt;
            }
            style_with_alignment.wrapping_opt = wrapping_opt.value();
          }
        } else if (tag == kTagSub) {
          if (key == "variable") {
            if (value.empty() || value[0] != '$') {
              PrintParseError(input_stream,
                              "[Symphony::Text::FormattedText] Variables "
                              "should start with $:");
              return std::nullopt;
            }

            std::string variable_name = value.substr(1);

            auto variable_it = variables.find(variable_name);
            if (variable_it == variables.end()) {
              PrintParseError(
                  input_stream,
                  "[Symphony::Text::FormattedText] Variable is not specified:");
              return std::nullopt;
            }

            variable_value = variable_it->second;
          } else {
            PrintParseError(input_stream,
                            "[Symphony::Text::FormattedText] Unknown parameter "
                            "for tag <sub> (should be 'variable'):");
            return std::nullopt;
          }
        }
      }

      // Get '>'.
      input_stream.get();

      if (tag == kTagStyle) {
        // Latest align is used as paragraph align.
        if (style_with_alignment.align_opt) {
          result.paragraphs.back().paragraph_parameters.align =
              style_with_alignment.align_opt.value();
        }

        // Latest wrapping is used as paragraph wrapping.
        if (style_with_alignment.wrapping_opt) {
          result.paragraphs.back().paragraph_parameters.wrapping =
              style_with_alignment.wrapping_opt.value();
        }

        if (!result.paragraphs.back().style_runs.back().text.empty()) {
          result.paragraphs.back().style_runs.push_back(StyleRun());
        }

        result.paragraphs.back().style_runs.back().style =
            StyleFromStyleWithParagraphParameters(style_with_alignment);

        styles_stack.push(style_with_alignment);
      } else if (tag == kTagSub) {
        result.paragraphs.back().style_runs.back().text.insert(
            result.paragraphs.back().style_runs.back().text.end(),
            variable_value.begin(), variable_value.end());
      }
    } else {
      char c = 0;
      input_stream.get(c);
      if (c == 0) {
        continue;
      }

      if (c == '\n') {
        result.paragraphs.push_back(Paragraph(styles_stack.top()));
      } else {
        result.paragraphs.back().style_runs.back().text.push_back(c);
      }
    }
  }

  // We don't need empty style runs in the end of paragraph. But let's keep
  // empty paragraphs.
  for (auto& paragraph : result.paragraphs) {
    while (!paragraph.style_runs.empty() &&
           paragraph.style_runs.back().text.empty()) {
      paragraph.style_runs.pop_back();
    }
  }

  return result;
}

}  // namespace Text
}  // namespace Symphony
