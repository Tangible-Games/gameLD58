#pragma once

#include <SDL2/SDL.h>

#include <fstream>

#include "formatted_text.hpp"
#include "measured_text.hpp"
#include "point2d.hpp"

namespace Symphony {
namespace Text {

class TextRenderer {
 public:
  TextRenderer() = default;

  explicit TextRenderer(std::shared_ptr<SDL_Renderer> sdl_renderer)
      : sdl_renderer_(sdl_renderer) {}

  void SetPosition(int x, int y) {
    x_ = x;
    y_ = y;
  }

  void SetWidth(int width) { width_ = width; }

  void SetHeight(int height) { height_ = height; }

  void SetSizes(int width, int height) {
    width_ = width;
    height_ = height;
  }

  bool LoadFromFile(const std::string& file_path);

  void ReFormat(const std::map<std::string, std::string>& variables,
                const std::string& default_font,
                const std::map<std::string, std::shared_ptr<Font>>& fonts);

  void Render();

 private:
  struct RenderBuffers {
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
  };

  struct Line {
    std::unordered_map<Font*, RenderBuffers> font_to_buffers;
  };

  static SDL_Color SdlColorFromUInt32(uint32_t /*color*/) {
    SDL_Color sdl_color;
    sdl_color.a = 255;  // ((color >> 24) & 0xFF);
    sdl_color.r = 0;    //((color >> 16) & 0xFF);
    sdl_color.g = 255;  // ((color >> 8) & 0xFF);
    sdl_color.b = 255;  // (color & 0xFF);
    return sdl_color;
  }

  std::shared_ptr<SDL_Renderer> sdl_renderer_;
  std::string raw_text_;
  std::optional<FormattedText> formatted_text_;
  std::optional<MeasuredText> measured_text_;
  std::vector<Line> lines_;
  int x_{0};
  int y_{0};
  int width_{0};
  int height_{0};
};

bool TextRenderer::LoadFromFile(const std::string& file_path) {
  std::ifstream file;

  file.open(file_path, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "[Symphony::Text::TextRenderer] Can't open file, file_path: "
              << file_path << std::endl;
    return false;
  }

  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();

  raw_text_.resize(file_size);

  file.seekg(0, std::ios::beg);
  file.read(&raw_text_[0], file_size);

  return true;
}

void TextRenderer::ReFormat(
    const std::map<std::string, std::string>& variables,
    const std::string& default_font,
    const std::map<std::string, std::shared_ptr<Font>>& fonts) {
  Style default_style(default_font, /*color*/ 0xFFFFFFFF);
  ParagraphParameters default_paragraph_parameters(HorizontalAlignment::kLeft,
                                                   Wrapping::kClip);
  formatted_text_ = FormatText(raw_text_, default_style,
                               default_paragraph_parameters, variables);
  if (!formatted_text_.has_value()) {
    return;
  }

  measured_text_ =
      MeasureText(width_, formatted_text_.value(), variables, fonts);
  if (!measured_text_.has_value()) {
    formatted_text_ = std::nullopt;
    return;
  }

  MeasuredText& measured_text = measured_text_.value();

  lines_.resize(measured_text.measured_lines.size());

  float line_y = 0.0f;
  for (size_t i = 0; i < lines_.size(); ++i) {
    Line& line = lines_[i];
    const MeasuredTextLine& measured_line = measured_text.measured_lines[i];

    for (const auto& [font, glyph_indices] :
         measured_line.font_to_glyph_index) {
      auto p = line.font_to_buffers.insert(
          std::make_pair(font, std::vector<SDL_Vertex>()));
      auto& render_buffers = p.first->second;

      SDL_Texture* sdl_texture = (SDL_Texture*)font->GetTexture();
      if (!sdl_texture) {
        continue;
      }

      int texture_width = 0;
      int texture_height = 0;
      SDL_QueryTexture(sdl_texture, nullptr, nullptr, &texture_width,
                       &texture_height);
      float texture_width_f = (float)texture_width;
      float texture_height_f = (float)texture_height;

      render_buffers.vertices.resize(glyph_indices.size() * 4);
      render_buffers.indices.resize(glyph_indices.size() * 6);
      for (size_t num_glyphs_processed = 0; auto glyph_index : glyph_indices) {
        const auto& measured_glyph = measured_line.glyphs[glyph_index];
        const auto& glyph = measured_glyph.glyph;

        SDL_Color sdl_color = SdlColorFromUInt32(measured_glyph.color);

        SDL_Vertex* vertex =
            &render_buffers.vertices[num_glyphs_processed * 4 + 0];
        vertex->position.x = (float)measured_glyph.x;
        vertex->position.y = line_y + (float)measured_glyph.y;
        vertex->color = sdl_color;
        vertex->tex_coord.x = (float)glyph.texture_x / texture_width_f;
        vertex->tex_coord.y = (float)glyph.texture_y / texture_height_f;

        vertex = &render_buffers.vertices[num_glyphs_processed * 4 + 1];
        vertex->position.x = (float)measured_glyph.x;
        vertex->position.y =
            line_y + (float)(measured_glyph.y + glyph.texture_height);
        vertex->color = sdl_color;
        vertex->tex_coord.x = (float)glyph.texture_x / texture_width_f;
        vertex->tex_coord.y =
            (float)(glyph.texture_y + glyph.texture_height) / texture_height_f;

        vertex = &render_buffers.vertices[num_glyphs_processed * 4 + 2];
        vertex->position.x = (float)(measured_glyph.x + glyph.texture_width);
        vertex->position.y =
            line_y + (float)(measured_glyph.y + glyph.texture_height);
        vertex->color = sdl_color;
        vertex->tex_coord.x =
            (float)(glyph.texture_x + glyph.texture_width) / texture_width_f;
        vertex->tex_coord.y =
            (float)(glyph.texture_y + glyph.texture_height) / texture_height_f;

        vertex = &render_buffers.vertices[num_glyphs_processed * 4 + 3];
        vertex->position.x = (float)(measured_glyph.x + glyph.texture_width);
        vertex->position.y = line_y + (float)(measured_glyph.y);
        vertex->color = sdl_color;
        vertex->tex_coord.x =
            (float)(glyph.texture_x + glyph.texture_width) / texture_width_f;
        vertex->tex_coord.y = (float)(glyph.texture_y) / texture_height_f;

        render_buffers.indices[num_glyphs_processed * 6 + 0] =
            num_glyphs_processed * 4 + 0;
        render_buffers.indices[num_glyphs_processed * 6 + 1] =
            num_glyphs_processed * 4 + 2;
        render_buffers.indices[num_glyphs_processed * 6 + 2] =
            num_glyphs_processed * 4 + 1;
        render_buffers.indices[num_glyphs_processed * 6 + 3] =
            num_glyphs_processed * 4 + 0;
        render_buffers.indices[num_glyphs_processed * 6 + 4] =
            num_glyphs_processed * 4 + 3;
        render_buffers.indices[num_glyphs_processed * 6 + 5] =
            num_glyphs_processed * 4 + 2;

        ++num_glyphs_processed;
      }
    }

    line_y += (float)measured_line.line_height;
  }
}

void TextRenderer::Render() {
  if (!formatted_text_.has_value()) {
    return;
  }

  if (!measured_text_.has_value()) {
    return;
  }

  for (const auto& line : lines_) {
    for (const auto& [font, buffers] : line.font_to_buffers) {
      SDL_Texture* sdl_texture = (SDL_Texture*)font->GetTexture();
      SDL_RenderGeometry(sdl_renderer_.get(), sdl_texture, &buffers.vertices[0],
                         buffers.vertices.size(), &buffers.indices[0],
                         buffers.indices.size());
    }
  }
}

}  // namespace Text
}  // namespace Symphony
