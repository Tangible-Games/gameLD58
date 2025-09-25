#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "font.hpp"

namespace Symphony {
namespace Text {
class BmFont : public Font {
 public:
  struct Info {
    std::string face;
    int size{0};
    int bold{0};
    int italic{0};
    std::string charset;
    int unicode{0};
    int stretch_h{0};
    int smooth{0};
    int aa{0};
    int padding[4];
    int spacing[2];
    int outline{0};
  };

  struct Common {
    int line_height{0};
    int base{0};
    int scale_w{0};
    int scale_h{0};
    int pages{0};
    int packed{0};
    int alpha_chnl{0};
    int red_chnl{0};
    int green_chnl{0};
    int blue_chnl{0};
  };

  struct Page {
    int id{0};
    std::string file;
  };

  struct Char {
    int id{0};
    int x{0};
    int y{0};
    int width{0};
    int height{0};
    int x_offset{0};
    int y_offset{0};
    int x_advance{0};
    int page{0};
    int chnl{0};
  };

  struct Kerning {
    int first{0};
    int second{0};
    int amount{0};
  };

  enum CharChannel {
    kCharChannelBlue = 1,
    kCharChannelGreen = 2,
    kCharChannelRed = 4,
    kCharChannelAlpha = 8,
    kCharChannelAll = 15,
  };

  static std::string InfoToString(const Info& info);
  static std::string CommonToString(const Common& common);
  static std::string PageToString(const Page& page);
  static std::string CharToString(const Char& c);
  static std::string KerningToString(const Kerning& kerning);

  BmFont() = default;
  ~BmFont() = default;

  bool Load(const std::string& file_path);
  bool LoadTexture(std::shared_ptr<SDL_Renderer> renderer);

  const Info GetInfo() const { return info_; }

  const Common GetCommon() const { return common_; }

  const std::vector<Page> GetPages() const { return pages_; }

  const std::vector<Char> GetChars() const { return chars_; }

  const std::vector<Kerning> GetKernings() const { return kernings_; }

  FontMeasurements GetFontMeasurements() const override {
    FontMeasurements result;
    result.line_height = common_.line_height;
    result.base = common_.base;
    return result;
  }

  Glyph GetGlyph(uint32_t code_position) const override;

  void* GetTexture() override { return sdl_texture_.get(); }

 private:
  enum BlockType {
    kBlockInfo,
    kBlockCommon,
    kBlockPage,
    kBlockChars,
    kBlockChar,
    kBlockKerning,
    kBlockUnknown,
  };

  static void deleteTexture(SDL_Texture* sdl_texture) {
    SDL_DestroyTexture(sdl_texture);
  }

  std::string file_path_;
  Info info_;
  Common common_;
  std::vector<Page> pages_;
  std::vector<Char> chars_;
  std::vector<Kerning> kernings_;
  std::unordered_map<uint32_t, size_t> code_position_to_char_;
  std::shared_ptr<SDL_Texture> sdl_texture_;
};

std::string BmFont::InfoToString(const Info& info) {
  std::ostringstream result_stream;

  result_stream << "info"
                << " face=\"" << info.face << "\""
                << " size=" << info.size << " bold=" << info.bold
                << " italic=" << info.italic << " charset=\"" << info.charset
                << "\""
                << " unicode=" << info.unicode << " stretchH=" << info.stretch_h
                << " smooth=" << info.smooth << " aa=" << info.aa
                << " padding=" << info.padding[0] << "," << info.padding[1]
                << "," << info.padding[2] << "," << info.padding[3] << ","
                << " spacing=" << info.spacing[0] << "," << info.spacing[0];

  return result_stream.str();
}

std::string BmFont::CommonToString(const Common& common) {
  std::ostringstream result_stream;

  result_stream << "common"
                << " lineHeight=" << common.line_height
                << " base=" << common.base << " scaleW=" << common.scale_w
                << " scaleH=" << common.scale_h << " pages=" << common.pages
                << " packed=" << common.packed
                << " alphaChnl=" << common.alpha_chnl
                << " redChnl=" << common.red_chnl
                << " greenChnl=" << common.green_chnl
                << " blueChnl=" << common.blue_chnl;

  return result_stream.str();
}

std::string BmFont::PageToString(const Page& page) {
  std::ostringstream result_stream;

  result_stream << "page"
                << " id=" << page.id << " file=" << page.file;

  return result_stream.str();
}

std::string BmFont::CharToString(const Char& c) {
  std::ostringstream result_stream;

  result_stream << "char"
                << " id=" << c.id << " x=" << c.x << " y=" << c.y
                << " width=" << c.width << " height=" << c.height
                << " xoffset=" << c.x_offset << " yoffset=" << c.y_offset
                << " xadvance=" << c.x_advance << " page=" << c.page
                << " chnl=" << c.chnl;

  return result_stream.str();
}

std::string BmFont::KerningToString(const Kerning& kerning) {
  std::ostringstream result_stream;

  result_stream << "kerning"
                << " first=" << kerning.first << " second=" << kerning.second
                << " amount=" << kerning.amount;

  return result_stream.str();
}

bool BmFont::Load(const std::string& file_path) {
  std::ifstream file;

  file.open(file_path);
  if (!file.is_open()) {
    std::cerr << "[Symphony::Text::BmFont] Can't open file, file_path: "
              << file_path << std::endl;
    return false;
  }

  size_t num_chars = 0;

  std::string block_content;
  while (std::getline(file, block_content)) {
    std::istringstream block_stream(block_content);

    std::string block_type_string;
    block_stream >> block_type_string;

    BlockType block_type = kBlockUnknown;
    if (block_type_string == "info") {
      block_type = kBlockInfo;
    } else if (block_type_string == "common") {
      block_type = kBlockCommon;
    } else if (block_type_string == "page") {
      block_type = kBlockPage;
      pages_.push_back(Page());
    } else if (block_type_string == "chars") {
      block_type = kBlockChars;
    } else if (block_type_string == "char") {
      block_type = kBlockChar;
      chars_.push_back(Char());
    } else if (block_type_string == "kerning") {
      block_type = kBlockKerning;
      kernings_.push_back(Kerning());
    } else if (block_type == kBlockUnknown) {
      continue;
    }

    while (!block_stream.eof()) {
      while (block_stream.peek() == ' ') {
        block_stream.get();
      }

      std::string key;
      std::getline(block_stream, key, '=');

      std::string value;
      if (block_stream.peek() == '\"') {
        block_stream.get();
        std::getline(block_stream, value, '\"');
      } else {
        block_stream >> value;
      }

      if (block_type == kBlockInfo) {
        if (key == "face") {
          info_.face = value;
        } else if (key == "size") {
          info_.size = std::stoi(value);
        } else if (key == "bold") {
          info_.bold = std::stoi(value);
        } else if (key == "italic") {
          info_.italic = std::stoi(value);
        } else if (key == "charset") {
          info_.charset = value;
        } else if (key == "unicode") {
          info_.unicode = std::stoi(value);
        } else if (key == "stretchH") {
          info_.stretch_h = std::stoi(value);
        } else if (key == "smooth") {
          info_.smooth = std::stoi(value);
        } else if (key == "aa") {
          info_.aa = std::stoi(value);
        } else if (key == "padding") {
          std::istringstream value_stream(value);
          value_stream >> info_.padding[0];
          value_stream >> info_.padding[1];
          value_stream >> info_.padding[2];
          value_stream >> info_.padding[3];
        } else if (key == "spacing") {
          std::istringstream value_stream(value);
          value_stream >> info_.spacing[0];
          value_stream >> info_.spacing[1];
          ;
        }
      } else if (block_type == kBlockCommon) {
        if (key == "lineHeight") {
          common_.line_height = std::stoi(value);
        } else if (key == "base") {
          common_.base = std::stoi(value);
        } else if (key == "scaleW") {
          common_.scale_w = std::stoi(value);
        } else if (key == "scaleH") {
          common_.scale_h = std::stoi(value);
        } else if (key == "pages") {
          common_.pages = std::stoi(value);
        } else if (key == "packed") {
          common_.packed = std::stoi(value);
        } else if (key == "alphaChnl") {
          common_.alpha_chnl = std::stoi(value);
        } else if (key == "redChnl") {
          common_.red_chnl = std::stoi(value);
        } else if (key == "greenChnl") {
          common_.green_chnl = std::stoi(value);
        } else if (key == "blueChnl") {
          common_.blue_chnl = std::stoi(value);
        }
      } else if (block_type == kBlockPage) {
        if (key == "id") {
          pages_.back().id = std::stoi(value);
        } else if (key == "file") {
          pages_.back().file = value;
        }
      } else if (block_type == kBlockChars) {
        if (key == "count") {
          num_chars = (size_t)std::stoi(value);
        }
      } else if (block_type == kBlockChar) {
        if (key == "id") {
          chars_.back().id = std::stoi(value);
        } else if (key == "x") {
          chars_.back().x = std::stoi(value);
        } else if (key == "y") {
          chars_.back().y = std::stoi(value);
        } else if (key == "width") {
          chars_.back().width = std::stoi(value);
        } else if (key == "height") {
          chars_.back().height = std::stoi(value);
        } else if (key == "xoffset") {
          chars_.back().x_offset = std::stoi(value);
        } else if (key == "yoffset") {
          chars_.back().y_offset = std::stoi(value);
        } else if (key == "xadvance") {
          chars_.back().x_advance = std::stoi(value);
        } else if (key == "page") {
          chars_.back().page = std::stoi(value);
        } else if (key == "chnl") {
          chars_.back().chnl = std::stoi(value);
        }
      } else if (block_type == kBlockKerning) {
        if (key == "first") {
          kernings_.back().first = std::stoi(value);
        } else if (key == "second") {
          kernings_.back().second = std::stoi(value);
        } else if (key == "amount") {
          kernings_.back().amount = std::stoi(value);
        }
      }
    }
  }

  if (pages_.size() != 1) {
    std::cerr << "[Symphony::Text::BmFont] Only fonts with single page are "
                 "supported, file_path: "
              << file_path << std::endl;
    return false;
  }

  if (common_.packed != 0) {
    std::cerr << "[Symphony::Text::BmFont] Characters shouldn't be packed in "
                 "separate color channels, file_path: "
              << file_path << std::endl;
    return false;
  }

  if (num_chars != chars_.size()) {
    std::cerr << "[Symphony::Text::BmFont] Wrong count in 'chars' block and "
                 "number of 'char' blocks, file_path: "
              << file_path << std::endl;
    return false;
  }

  for (const auto& c : chars_) {
    if (c.chnl != kCharChannelAll) {
      std::cerr << "[Symphony::Text::BmFont] Only characters across all color "
                   "channels are supported, file_path: "
                << file_path << ", char: " << c.id << std::endl;
      return false;
    }
  }

  for (size_t index = 0; const auto& c : chars_) {
    code_position_to_char_[(uint32_t)c.id] = index;
    ++index;
  }

  file_path_ = file_path;

  return true;
}

bool BmFont::LoadTexture(std::shared_ptr<SDL_Renderer> sdl_renderer) {
  auto font_path = std::filesystem::path(file_path_);
  auto texture_path = font_path.parent_path() / pages_[0].file;

  sdl_texture_.reset(IMG_LoadTexture(sdl_renderer.get(), texture_path.c_str()),
                     &deleteTexture);
  if (!sdl_texture_) {
    std::cerr
        << "[Symphony::Text::BmFont] Failed to create texture, file_path: "
        << texture_path << ", font_file_path: " << file_path_ << std::endl;
    return false;
  }

  return true;
}

Glyph BmFont::GetGlyph(uint32_t code_position) const {
  Glyph result;

  auto it = code_position_to_char_.find(code_position);
  if (it == code_position_to_char_.end()) {
    return result;
  }

  const auto& c = chars_[it->second];
  result.texture_x = c.x;
  result.texture_y = c.y;
  result.texture_width = c.width;
  result.texture_height = c.height;
  result.x_offset = c.x_offset;
  result.y_offset = c.y_offset;
  result.x_advance = c.x_advance;
  result.code_position = code_position;

  return result;
}

std::shared_ptr<BmFont> LoadBmFont(const std::string& file_path) {
  std::shared_ptr<BmFont> result(new BmFont());
  if (!result->Load(file_path)) {
    result.reset();
  }
  return result;
}

}  // namespace Text
}  // namespace Symphony
