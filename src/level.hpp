#pragma once

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include <string>
#include <symphony_lite/aa_rect2d.hpp>
#include <symphony_lite/log.hpp>
#include <vector>

#include "consts.hpp"
#include "ufo.hpp"

namespace gameLD58 {

struct Object {
  std::string layer;
  Symphony::Math::AARect2d rect;
};

struct Config {
  float length;
  float height;
  Symphony::Math::Point2d ufo_spawn;
};

class Level {
 public:
  Level(std::shared_ptr<SDL_Renderer> renderer,
        std::shared_ptr<Symphony::Audio::Device> audio, std::string path)
      : renderer_(renderer),
        audio_(audio),
        level_path_(std::move(path)),
        ufo_(renderer, audio) {}

  void Load();
  void Draw();
  void Update(float dt);

  void Start();

  void SetIsPaused(bool is_paused);

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  std::string level_path_;
  Config level_config_;
  bool is_paused_{false};
  std::vector<Object> objects_;
  Ufo ufo_;
  float cam_x_;
  float cam_y_;

 private:
  static std::string readFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
      return "";
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }
};

void Level::Load() {
  const auto text = readFile(level_path_);
  if (text.empty()) {
    LOGE("[gameLD58:Level]: cannot read file '{}'", level_path_);
    return;
  }

  nlohmann::json level_json = nlohmann::json::parse(text, nullptr, false);
  if (level_json.is_discarded()) {
    LOGE("[gameLD58:Level]: cannot parse JSON from file '{}'", level_path_);
    return;
  }

  Config config;
  config.length = level_json.value("length", (float)kScreenWidth);
  config.height = level_json.value("height", (float)kScreenHeight);

  if (config.length < kScreenWidth) {
    LOGW("level.length {} < screen {}, clamping", config.length, kScreenWidth);
    config.length = (float)kScreenWidth;
  }
  if (config.height < kScreenHeight) {
    LOGW("level.height {} < screen {}, clamping", config.height, kScreenHeight);
    config.height = (float)kScreenHeight;
  }
  config.ufo_spawn.x = level_json.value("ufo_spawn_x", kScreenWidth / 2.f);
  config.ufo_spawn.y = level_json.value("ufo_spawn_y", kScreenHeight / 2.f);

  cam_x_ = config.ufo_spawn.x;
  cam_x_ -= config.length * std::floor(cam_x_ / config.length);
  float min_center = 0.5f * kScreenHeight;
  float max_center = std::max(min_center, config.height - 0.5f * kScreenHeight);
  cam_y_ = std::clamp(config.ufo_spawn.y, min_center, max_center);

  for (const auto& item : level_json["items"]) {
    Object obj;
    float center_x = item.value("center_x", 0);
    float center_y = item.value("center_y", 0);
    float half_width = item.value("half_width", 0);
    float half_height = item.value("half_height", 0);
    Symphony::Math::AARect2d rect{{center_x, center_y},
                                  {half_width, half_height}};
    obj.rect = rect;
    obj.layer = item.value("layer", "default");
    objects_.push_back(obj);
  }

  LOGD("Level loaded: length={}, spawn=({}, {}), obstacles={}", config.length,
       config.ufo_spawn.x, config.ufo_spawn.y, objects_.size());

  level_config_ = std::move(config);

  ufo_.Load();
}

void Level::Draw() {
  float l = level_config_.length;
  float cam_left = cam_x_ - kScreenWidth * 0.5f;
  float cam_top = cam_y_ - kScreenHeight * 0.5f;

  SDL_SetRenderDrawColor(renderer_.get(), 0, 255, 0, 255);

  bool crosses_left = cam_left < 0.0f;
  bool crosses_right = cam_left + kScreenWidth > l;

  bool need_wrap = l > kScreenWidth;

  for (const auto& obj : objects_) {
    const auto& b = obj.rect;
    float cx = b.center.x;
    float cy = b.center.y;
    float hx = b.half_size.x;
    float hy = b.half_size.y;

    auto draw_one = [&](float draw_cx) {
      float x = (draw_cx - hx) - cam_left;
      float y = (cy - hy) - cam_top;
      float rw = 2.f * hx, rh = 2.f * hy;

      if (x + rw <= 0.f || x >= kScreenWidth) return;
      if (y + rh <= 0.f || y >= kScreenHeight) return;

      SDL_FRect r{x, y, rw, rh};
      SDL_RenderRect(renderer_.get(), &r);
    };

    draw_one(cx);
    if (need_wrap && crosses_left) {
      draw_one(cx - l);
    }
    if (need_wrap && crosses_right) {
      draw_one(cx + l);
    }
  }

  ufo_.Draw();
}

void Level::Update(float dt) {
  if (is_paused_) {
    return;
  }

  ufo_.Update(dt);
  // Add collisions here:
  ufo_.SetPosition(ufo_.GetBounds().center + ufo_.GetVelocity() * dt);

  cam_x_ = ufo_.GetBounds().center.x;
  cam_y_ = ufo_.GetBounds().center.y;
}

void Level::Start() {
  is_paused_ = false;

  ufo_.SetPosition(level_config_.ufo_spawn);
}

void Level::SetIsPaused(bool is_paused) {
  is_paused_ = is_paused;
  ufo_.SetIsPaused(is_paused);
}

}  // namespace gameLD58
