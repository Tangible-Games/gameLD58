#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "consts.hpp"

namespace gameLD58 {

struct ParallaxLayerDesc {
  SDL_Texture* texture = nullptr;
  float factor_x = 0.0f;  // 0 — не двигается, 1 — как мир, >1 — быстрее мира
  float scale = 1.0f;
  float world_y = 0.0f;
};

class ParallaxRenderer {
 public:
  ParallaxRenderer(std::shared_ptr<SDL_Renderer> renderer)
      : renderer_(renderer) {}

  void Load(float world_length, std::string backgrounds_path);
  void Draw(float cam_x, float cam_y);

 private:
  struct layer {
    ParallaxLayerDesc desc;
    int tile_w = 0;
    int tile_h = 0;
    mutable float phase_x = 0.0f;
  };

  std::shared_ptr<SDL_Renderer> renderer_;
  std::vector<layer> layers_;
  std::string backgrounds_path_;
  float world_length_ = 0.f;
  mutable bool has_prev_cam_ = false;
  mutable float prev_cam_x_ = 0.0f;
  mutable double cam_x_unwrapped_ = 0.0;

  int AddLayer(const ParallaxLayerDesc& d);
  static float Wrapf(float x, float w);

  static std::string readFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
      return "";
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }

  static inline float shortestDelta(float a, float b, float L) {
    float d = a - b;
    d -= L * std::floor((d + L * 0.5f) / L);
    return d;
  }
};

int ParallaxRenderer::AddLayer(const ParallaxLayerDesc& d) {
  layer l;
  l.desc = d;
  if (d.texture) {
    float tw = 0, th = 0;
    SDL_GetTextureSize(d.texture, &tw, &th);
    l.tile_w = tw;
    l.tile_h = th;
  }
  layers_.push_back(l);
  return (int)layers_.size() - 1;
}

float ParallaxRenderer::Wrapf(float x, float w) {
  return x - w * std::floor(x / w);
}

void ParallaxRenderer::Load(float world_length, std::string backgrounds_path) {
  layers_.clear();
  this->world_length_ = world_length;

  const auto text = readFile(backgrounds_path);
  if (text.empty()) {
    LOGE("[gameLD58:ParallaxRenderer]: cannot read file '{}'",
         backgrounds_path);
    return;
  }

  nlohmann::json backgrounds_json = nlohmann::json::parse(text, nullptr, false);
  if (backgrounds_json.is_discarded()) {
    LOGE("[gameLD58:ParallaxRenderer]: cannot parse JSON from file '{}'",
         backgrounds_path);
    return;
  }
  if (!backgrounds_json.is_array()) {
    LOGE("[gameLD58:ParallaxRenderer]: expected top-level array in '{}'",
         backgrounds_path);
    return;
  }

  for (const auto& item : backgrounds_json) {
    ParallaxLayerDesc desc{};

    const std::string tex_path = item.value("texture", std::string{});
    if (tex_path.empty()) {
      LOGE("[gameLD58:ParallaxRenderer]: layer missing 'texture' field");
      continue;
    }

    desc.texture = IMG_LoadTexture(renderer_.get(), tex_path.c_str());
    if (!desc.texture) {
      LOGE("[gameLD58:ParallaxRenderer]: failed to load texture '{}'",
           tex_path);
      continue;
    }

    desc.factor_x = item.value("factor_x", 1.0f);
    desc.scale = item.value("scale", 1.0f);
    desc.world_y = item.value("world_y", 0.0f);

    AddLayer(desc);
  }
}

void ParallaxRenderer::Draw(float cam_x, float cam_y) {
  if (!has_prev_cam_ || world_length_ <= 0.0f) {
    cam_x_unwrapped_ = cam_x;
    prev_cam_x_ = cam_x;
    has_prev_cam_ = true;
  } else {
    float dx = shortestDelta(cam_x, prev_cam_x_, world_length_);
    cam_x_unwrapped_ += (double)dx;
    prev_cam_x_ = cam_x;
  }

  float cam_left = (float)cam_x_unwrapped_ - kScreenWidth * 0.5f;
  float cam_top = cam_y - kScreenHeight * 0.5f;

  for (const auto& l : layers_) {
    if (!l.desc.texture) continue;

    float tile_w_px = l.tile_w * l.desc.scale;
    float tile_h_px = l.tile_h * l.desc.scale;
    if (tile_w_px <= 0.0f || tile_h_px <= 0.0f) continue;

    float shift = Wrapf(cam_left * l.desc.factor_x, tile_w_px);
    float start_x = -shift;
    float y = l.desc.world_y - cam_top;

    float first_x = start_x - tile_w_px;
    int tiles = (int)std::ceil((kScreenWidth + tile_w_px) / tile_w_px) + 1;

    for (int i = 0; i < tiles; ++i) {
      float x = first_x + i * tile_w_px;
      SDL_FRect dst{x, y, tile_w_px, tile_h_px};
      SDL_RenderTexture(renderer_.get(), l.desc.texture, nullptr, &dst);
    }
  }
}

}  // namespace gameLD58
