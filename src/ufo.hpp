#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
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

  std::shared_ptr<SDL_Texture> texture_{};

  struct Configuration {
    Symphony::Math::AARect2d rect{};
    Symphony::Math::Vector2d maxVelocity{0, 0};
    Symphony::Math::Vector2d moveAcceleration{0, 0};
    Symphony::Math::Vector2d dragForce{0, 0};
    Symphony::Math::Vector2d driftAcceleration{0, 0};
    Symphony::Math::Vector2d driftThreshold{0, 0};
  } configuration_;

  Symphony::Math::Vector2d velocity_{0, 0};
  Symphony::Math::Vector2d acceleration_{0, 0};

  float prevDt_{0};
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

  // Parse configuration
  configuration_.rect = Symphony::Math::AARect2d{
      Symphony::Math::Point2d{kScreenWidth / 2.0, kScreenHeight / 2.0},
      Symphony::Math::Vector2d{config["size"]["width"].get<float>() / 2.0f,
                               config["size"]["height"].get<float>() / 2.0f}};

  configuration_.moveAcceleration =
      Symphony::Math::Vector2d{config["acceleration"]["x"].get<float>(),
                               config["acceleration"]["y"].get<float>()};

  configuration_.maxVelocity =
      Symphony::Math::Vector2d{config["max_velocity"]["x"].get<float>(),
                               config["max_velocity"]["y"].get<float>()};

  configuration_.dragForce =
      Symphony::Math::Vector2d{config["drag_force"]["x"].get<float>(),
                               config["drag_force"]["y"].get<float>()};

  configuration_.driftAcceleration = Symphony::Math::Vector2d{
      config["drift"]["acceleration"]["x"].get<float>(),
      config["drift"]["acceleration"]["y"].get<float>()};

  configuration_.driftThreshold =
      Symphony::Math::Vector2d{config["drift"]["threshold"]["x"].get<float>(),
                               config["drift"]["threshold"]["y"].get<float>()};

  const std::string texturePath = config["texture"].get<std::string>();
  texture_ = std::shared_ptr<SDL_Texture>(
      IMG_LoadTexture(renderer_.get(), texturePath.c_str()),
      &SDL_DestroyTexture);
}

void Ufo::Update(float dt) {
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadLeft).has_value()) {
    acceleration_.x -= configuration_.moveAcceleration.x;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadRight).has_value()) {
    acceleration_.x += configuration_.moveAcceleration.x;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadUp).has_value()) {
    acceleration_.y -= configuration_.moveAcceleration.y;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadDown).has_value()) {
    acceleration_.y += configuration_.moveAcceleration.y;
  }

  velocity_.x = acceleration_.x * dt;
  velocity_.y = acceleration_.y * dt;

  if (std::floor(prevDt_ + dt) > std::floor(prevDt_)) {
    LOGD("acc: x: {}, y: {}", acceleration_.x, acceleration_.y);
  }
  prevDt_ += dt;

  velocity_.x = std::clamp<float>(velocity_.x, -configuration_.maxVelocity.x,
                                  configuration_.maxVelocity.x);
  velocity_.y = std::clamp<float>(velocity_.y, -configuration_.maxVelocity.y,
                                  configuration_.maxVelocity.y);

  configuration_.rect.center.x += velocity_.x * dt;
  configuration_.rect.center.y += velocity_.y * dt;

  // Add some drag, acceleration is decreasing
  float accXAbs = std::abs(acceleration_.x);
  accXAbs -= configuration_.dragForce.x * std::abs(velocity_.x);
  accXAbs = std::max(accXAbs, 0.0f);
  acceleration_.x = std::copysign(accXAbs, acceleration_.x);

  float accYAbs = std::abs(acceleration_.y);
  accYAbs -= configuration_.dragForce.y * std::abs(velocity_.y);
  accYAbs = std::max(accYAbs, 0.0f);
  acceleration_.y = std::copysign(accYAbs, acceleration_.y);

  if (std::abs(acceleration_.x) < configuration_.driftThreshold.x &&
      std::abs(acceleration_.y) < configuration_.driftThreshold.y) {
    // Add random drift when UFO is stopped
    float rx = ((float)std::rand() - RAND_MAX / 2.0) / ((float)RAND_MAX / 2.0);
    float ry = ((float)std::rand() - RAND_MAX / 2.0) / ((float)RAND_MAX / 2.0);
    acceleration_.x += configuration_.driftAcceleration.x * rx;
    acceleration_.y += configuration_.driftAcceleration.y * ry;
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
  SDL_FRect rect = AARectToSdlFRect(configuration_.rect);
  SDL_RenderTexture(renderer_.get(), texture_.get(), NULL, &rect);
}

}  // namespace gameLD58