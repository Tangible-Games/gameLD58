#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>

#include <nlohmann/json.hpp>
#include <symphony_lite/all_symphony.hpp>

#include "consts.hpp"
#include "keyboard.hpp"
#include "symphony_lite/vector2d.hpp"

namespace gameLD58 {
class Ufo {
  static constexpr auto kUfoConfigPath{"assets/ufo.json"};

 public:
  Ufo(std::shared_ptr<SDL_Renderer> renderer,
      std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer), audio_(audio) {}
  virtual ~Ufo() = default;

  void Load();
  void Draw();

  void Update(float dt);

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;

  Symphony::Math::AARect2d rect_{};
  std::shared_ptr<SDL_Texture> texture_{};

  Symphony::Math::Vector2d velocity_{};
};

void Ufo::Load() {
  // Load UFO configuration from json
  std::ifstream file;

  file.open(kUfoConfigPath);
  if (!file.is_open()) {
    LOGE("Failed to load {}", kUfoConfigPath);
    return;
  }

  // Parse JSON and extract values
  nlohmann::json config = nlohmann::json::parse(file);
  file.close();

  // Extract size
  rect_ = Symphony::Math::AARect2d{
      Symphony::Math::Point2d{kScreenWidth / 2.0, kScreenHeight / 2.0},
      Symphony::Math::Vector2d{config["size"]["width"].get<float>() / 2.0f,
                               config["size"]["height"].get<float>() / 2.0f}};

  velocity_ = Symphony::Math::Vector2d{config["velocity"]["x"].get<float>(),
                                       config["velocity"]["y"].get<float>()};

  const std::string texturePath = config["texture"].get<std::string>();
  texture_ = std::shared_ptr<SDL_Texture>(
      IMG_LoadTexture(renderer_.get(), texturePath.c_str()),
      &SDL_DestroyTexture);
}

void Ufo::Update(float dt) {
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadLeft).has_value()) {
    rect_.center.x -= velocity_.x * dt;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadRight).has_value()) {
    rect_.center.x += velocity_.x * dt;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadUp).has_value()) {
    rect_.center.y -= velocity_.y * dt;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadDown).has_value()) {
    rect_.center.y += velocity_.y * dt;
  }
}

inline SDL_FRect AARectToSdlFRect(Symphony::Math::AARect2d inp) {
  SDL_FRect out;
  out.x = inp.center.x - inp.half_size.x;
  out.y = inp.center.y - inp.half_size.y;
  out.w = inp.half_size.x * 2;
  out.h = inp.half_size.y * 2;
  return out;
}

void Ufo::Draw() {
  SDL_FRect rect = AARectToSdlFRect(rect_);
  SDL_RenderTexture(renderer_.get(), texture_.get(), NULL, &rect);
}

}  // namespace gameLD58