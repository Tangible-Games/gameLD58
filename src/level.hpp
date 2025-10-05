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
  float spawn_x;
  float spawn_y;
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

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  std::string level_path_;
  Config level_config_;
  std::vector<Object> objects_;
  Ufo ufo_;
  float cam_x_;
  float cam_y_;
  const float scroll_speed_ = 70.f;

 private:
  static std::string readFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return {};
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
  config.spawn_x = level_json.value("spawn_x", kScreenWidth / 2.f);
  config.spawn_y = level_json.value("spawn_y", kScreenHeight / 2.f);

  cam_x_ = config.spawn_x;
  cam_x_ -= config.length * std::floor(cam_x_ / config.length);
  float min_center = 0.5f * kScreenHeight;
  float max_center = std::max(min_center, config.height - 0.5f * kScreenHeight);
  cam_y_ = std::clamp(config.spawn_y, min_center, max_center);

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
       config.spawn_x, config.spawn_y, objects_.size());

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
    float cx = b.center.x, cy = b.center.y;
    float hx = b.half_size.x, hy = b.half_size.y;

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
    if (need_wrap && crosses_left) draw_one(cx - l);
    if (need_wrap && crosses_right) draw_one(cx + l);
  }

  ufo_.Draw();
}

void Level::Update(float dt) {
  bool left =
      Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadLeft).has_value();
  bool right =
      Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadRight).has_value();
  bool up = Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadUp).has_value();
  bool down =
      Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadDown).has_value();

  int dir_x = (int)right - (int)left;
  int dir_y = (int)down - (int)up;

  float l = level_config_.length;
  if (dir_x != 0 && dt > 0.0f) {
    cam_x_ += dir_x * scroll_speed_ * dt;
    if (cam_x_ >= l || cam_x_ < 0.0f) {
      cam_x_ -= l * std::floor(cam_x_ / l);
    }
  }

  float world_h = level_config_.height;
  float y = cam_y_;
  if (dt > 0.0f && dir_y != 0) {
    y += dir_y * scroll_speed_ * dt;
  }

  float min_center = 0.5f * kScreenHeight;
  float max_center = world_h - 0.5f * kScreenHeight;

  if (world_h <= kScreenHeight) {
    cam_y_ = min_center;
  } else {
    if (y < min_center)
      y = min_center;
    else if (y > max_center)
      y = max_center;
    cam_y_ = y;
  }

  ufo_.Update(dt);
  ufo_.SetPosition(ufo_.GetBounds().center + ufo_.GetVelocity() * dt);
}

}  // namespace gameLD58
