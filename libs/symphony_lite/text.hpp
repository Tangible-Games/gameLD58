#pragma once

#include <SDL3/SDL.h>
#include <vlog/vlog.h>

#include <fstream>
#include <unordered_map>

#include "formatted_text.hpp"
#include "measured_text.hpp"

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

  int GetContentHeight() const { return content_height_; }

  void Render(int scroll_y);

 private:
  struct RenderBuffers {
    std::vector<float> original_ys;
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
  };

  struct Line {
    int min_y{0};
    int max_y{0};
    std::unordered_map<Font*, RenderBuffers> font_to_buffers;
  };

  static SDL_FColor SdlColorFromUInt32(uint32_t color) {
    SDL_FColor sdl_color;
    sdl_color.a = (float)((color >> 24) & 0xFF) / 255.0f;
    sdl_color.r = (float)((color >> 16) & 0xFF) / 255.0f;
    sdl_color.g = (float)((color >> 8) & 0xFF) / 255.0f;
    sdl_color.b = (float)(color & 0xFF) / 255.0f;
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
  int content_height_{0};
  int prev_scroll_y_{0};
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

  int line_y = 0;
  float line_y_f = 0;
  for (size_t i = 0; i < lines_.size(); ++i) {
    Line& line = lines_[i];
    const MeasuredTextLine& measured_line = measured_text.measured_lines[i];
    float align_offset = static_cast<float>(measured_line.align_offset);

    for (const auto& [font, glyph_indices] :
         measured_line.font_to_glyph_index) {
      auto p =
          line.font_to_buffers.insert(std::make_pair(font, RenderBuffers()));
      auto& render_buffers = p.first->second;

      SDL_Texture* sdl_texture = (SDL_Texture*)font->GetTexture();
      if (!sdl_texture) {
        continue;
      }

      int texture_width = sdl_texture->w;
      int texture_height = sdl_texture->h;
      float texture_width_scale = 1.0f / (float)texture_width;
      float texture_height_scale = 1.0f / (float)texture_height;

      (void)texture_width_scale;
      (void)texture_height_scale;

      render_buffers.original_ys.resize(glyph_indices.size());
      render_buffers.vertices.resize(glyph_indices.size() * 4);
      render_buffers.indices.resize(glyph_indices.size() * 6);
      for (size_t num_glyphs_processed = 0; auto glyph_index : glyph_indices) {
        const auto& measured_glyph = measured_line.glyphs[glyph_index];
        const auto& glyph = measured_glyph.glyph;

        SDL_FColor sdl_color = SdlColorFromUInt32(measured_glyph.color);

        float glyph_x = x_ + align_offset + (float)measured_glyph.x;
        float glyph_y = y_ + line_y_f + (float)measured_glyph.y;
        render_buffers.original_ys[num_glyphs_processed] = glyph_y;

        SDL_Vertex* vertex =
            &render_buffers.vertices[num_glyphs_processed * 4 + 0];
        vertex->position.x = glyph_x;
        vertex->position.y = glyph_y;
        vertex->color = sdl_color;
        vertex->tex_coord.x = (float)glyph.texture_x * texture_width_scale;
        vertex->tex_coord.y = (float)glyph.texture_y * texture_height_scale;

        vertex = &render_buffers.vertices[num_glyphs_processed * 4 + 1];
        vertex->position.x = glyph_x;
        vertex->position.y = glyph_y + (float)glyph.texture_height;
        vertex->color = sdl_color;
        vertex->tex_coord.x = (float)glyph.texture_x * texture_width_scale;
        vertex->tex_coord.y = (float)(glyph.texture_y + glyph.texture_height) *
                              texture_height_scale;

        vertex = &render_buffers.vertices[num_glyphs_processed * 4 + 2];
        vertex->position.x = glyph_x + (float)glyph.texture_width;
        vertex->position.y = glyph_y + (float)glyph.texture_height;
        vertex->color = sdl_color;
        vertex->tex_coord.x = (float)(glyph.texture_x + glyph.texture_width) *
                              texture_width_scale;
        vertex->tex_coord.y = (float)(glyph.texture_y + glyph.texture_height) *
                              texture_height_scale;

        vertex = &render_buffers.vertices[num_glyphs_processed * 4 + 3];
        vertex->position.x = glyph_x + (float)glyph.texture_width;
        vertex->position.y = glyph_y;
        vertex->color = sdl_color;
        vertex->tex_coord.x = (float)(glyph.texture_x + glyph.texture_width) *
                              texture_width_scale;
        vertex->tex_coord.y = (float)(glyph.texture_y) * texture_height_scale;

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

    line.min_y = line_y;
    line.max_y = line_y + measured_line.line_height;
    line_y += measured_line.line_height;
    line_y_f += (float)measured_line.line_height;
  }

  content_height_ = 0;
  if (!lines_.empty()) {
    content_height_ = lines_.back().max_y;
  }
}

void TextRenderer::Render(int scroll_y) {
  if (!formatted_text_.has_value()) {
    return;
  }

  if (!measured_text_.has_value()) {
    return;
  }

  SDL_Rect prev_clip_rect;
  SDL_GetRenderClipRect(sdl_renderer_.get(), &prev_clip_rect);

  SDL_Rect clip_rect(x_, y_, width_, height_);
  SDL_SetRenderClipRect(sdl_renderer_.get(), &clip_rect);

  SDL_SetRenderDrawBlendMode(sdl_renderer_.get(), SDL_BLENDMODE_BLEND);

  for (const auto& line : lines_) {
    if ((scroll_y + line.max_y) < y_) {
      continue;
    }
    if ((scroll_y + line.min_y) > y_ + height_) {
      break;
    }

    for (auto& [font, buffers] : line.font_to_buffers) {
      if (prev_scroll_y_ != scroll_y) {
        for (size_t i = 0; i < buffers.vertices.size() / 4; ++i) {
          float y = buffers.original_ys[i];
          float height = buffers.vertices[i * 4 + 1].position.y -
                         buffers.vertices[i * 4 + 0].position.y;
          SDL_Vertex* vertex =
              const_cast<SDL_Vertex*>(&buffers.vertices[i * 4 + 0]);
          vertex->position.y = y + scroll_y;

          vertex = const_cast<SDL_Vertex*>(&buffers.vertices[i * 4 + 1]);
          vertex->position.y = y + height + scroll_y;

          vertex = const_cast<SDL_Vertex*>(&buffers.vertices[i * 4 + 2]);
          vertex->position.y = y + height + scroll_y;

          vertex = const_cast<SDL_Vertex*>(&buffers.vertices[i * 4 + 3]);
          vertex->position.y = y + scroll_y;
        }
      }

      SDL_Texture* sdl_texture = (SDL_Texture*)font->GetTexture();
      SDL_SetTextureBlendMode(sdl_texture, SDL_BLENDMODE_BLEND);

      SDL_RenderGeometry(sdl_renderer_.get(), sdl_texture, &buffers.vertices[0],
                         buffers.vertices.size(), &buffers.indices[0],
                         buffers.indices.size());
    }
  }

  SDL_SetRenderClipRect(sdl_renderer_.get(), &prev_clip_rect);

  prev_scroll_y_ = scroll_y;
}

}  // namespace Text
}  // namespace Symphony
