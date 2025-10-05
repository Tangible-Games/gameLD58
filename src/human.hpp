#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <symphony_lite/all_symphony.hpp>

#include "symphony_lite/vector2d.hpp"

namespace gameLD58 {

constexpr auto kHumanConfigPath{"assets/human.json"};
struct HumanConfiguration {
  static HumanConfiguration load() {
    std::ifstream file;
    HumanConfiguration ret;

    file.open(kHumanConfigPath);
    if (file.is_open()) {
      // Parse JSON and extract values
      nlohmann::json config = nlohmann::json::parse(file);
      file.close();

      ret.accelerationCapturing =
          config["accelerationY"]["capturing"].get<float>();
      ret.accelerationFalling = config["accelerationY"]["falling"].get<float>();
      ret.velocityDeadly = config["velocityY"]["deadly"].get<float>();
    } else {
      LOGE("Failed to load {}", kHumanConfigPath);
    }

    return ret;
  }

  static HumanConfiguration& configuration() {
    static HumanConfiguration config{HumanConfiguration::load()};
    return config;
  }

  float accelerationCapturing{0};
  float accelerationFalling{0};
  float velocityDeadly{0};
};

struct HumanTexture {
  static std::shared_ptr<SDL_Texture> texture(
      std::shared_ptr<SDL_Renderer> renderer) {
    static std::shared_ptr<SDL_Texture> _ = std::shared_ptr<SDL_Texture>(
        IMG_LoadTexture(renderer.get(), "assets/human.png"),
        &SDL_DestroyTexture);
    return _;
  }
};

class Human {
 public:
  Human(std::shared_ptr<SDL_Renderer> renderer,
        const Symphony::Math::AARect2d& r)
      : rect(r),
        groundY_(r.center.y),
        renderer_(renderer),
        texture_(HumanTexture::texture(renderer)),
        configuration_(HumanConfiguration::configuration()) {}

  bool Update(float dt) {
    if (captured_) {
      acc_.y -= configuration_.accelerationCapturing * dt;
    } else if (rect.center.y < groundY_) {
      acc_.y += configuration_.accelerationFalling * dt;
    }

    auto v = acc_ * dt;
    rect.center.y += v.y * dt;
    if (rect.center.y > groundY_) {
      rect.center.y = groundY_;
      acc_.y = 0;
      if (v.y > configuration_.velocityDeadly) {
        // TODO: Do something
        return false;
      }
    }
    return true;
  }

  void DrawTo(const SDL_FRect& r) {
    SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, &r);
  }

  Symphony::Math::AARect2d rect;

  float groundY_;

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<SDL_Texture> texture_;
  HumanConfiguration configuration_;
  Symphony::Math::Vector2d acc_;
  bool captured_{false};
};

}  // namespace gameLD58