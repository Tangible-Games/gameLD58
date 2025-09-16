#pragma once

#include <fstream>
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

enum Tag {
  kTagP,
  kTagStyle,
  kTagSub,
};

void PrintParseError(std::istringstream& input_stream,
                     const std::string& error_message) {
  std::cerr << error_message << std::endl;
  auto parse_error_source = MakeParseErrorSource(input_stream);
  std::cerr << parse_error_source.source << std::endl;
  std::cerr << parse_error_source.marker << std::endl;
}
}  // namespace

namespace Text {
// There are two tags:
// 1. <style> tag has the following parameters:
//   - font="font_name.fnt|$variable_name"
//   - color="AARRGGBB|$variable_name"
//   - align="left|right|center|$variable_name"
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

struct StyleWithAlignment {
  std::string font;
  uint32_t color;
  HorizontalAlignment align;
};

struct Style {
  std::string font;
  uint32_t color;
};

struct StyleRun {
  Style style;
  std::string text;
};

struct Paragraph {
  Paragraph(StyleWithAlignment style_with_alignment)
      : align(style_with_alignment.align) {
    style_runs.push_back(StyleRun());
    style_runs.back().style.font = style_with_alignment.font;
    style_runs.back().style.color = style_with_alignment.color;
  }

  HorizontalAlignment align{HorizontalAlignment::kLeft};
  std::vector<StyleRun> style_runs;
};

bool IsEmpty(const Paragraph& paragraph) {
  if (paragraph.style_runs.size() == 0) {
    return true;
  }
  if (paragraph.style_runs.size() == 1 &&
      paragraph.style_runs[0].text.empty()) {
    return true;
  }
  return false;
}

struct FormattedText {
  std::vector<Paragraph> paragraphs;
};

std::optional<FormattedText> LoadFormattedText(
    const std::string& input, const StyleWithAlignment& default_style,
    const std::map<std::string, std::string>& variables) {
  (void)variables;
  FormattedText result;

  result.paragraphs.push_back(Paragraph(default_style));

  std::stack<StyleWithAlignment> styles_stack;
  styles_stack.push(default_style);

  std::istringstream input_stream(input);
  while (!input_stream.eof()) {
    int next_char = input_stream.peek();

    if (next_char == '<') {
      input_stream.get();

      next_char = input_stream.peek();
      if (next_char == '<') {
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

      StyleWithAlignment style = styles_stack.top();
      std::string sub_value;

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

        std::cout << key << " -> " << value << std::endl;

        if (tag == kTagStyle) {
          if (key == "font") {
            std::swap(style.font, value);
          }
        } else if (tag == kTagSub) {
          if (key == "variable") {
            std::swap(sub_value, value);
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
        styles_stack.push(style);
      }
    } else {
      char c = 0;
      input_stream.get(c);

      result.paragraphs.back().style_runs.back().text.push_back(c);
    }
  }

  return result;
}

std::optional<FormattedText> LoadFormattedTextFromFile(
    const std::string& file_path, const StyleWithAlignment& default_style,
    const std::map<std::string, std::string>& variables) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    std::cerr << "[Symphony::Text::FormattedText] Can't open file, file_path: "
              << file_path << std::endl;
    return std::nullopt;
  }

  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::string file_content;
  file_content.resize(file_size);

  file.read(&file_content[0], file_size);

  return LoadFormattedText(file_content, default_style, variables);
}
}  // namespace Text
}  // namespace Symphony
