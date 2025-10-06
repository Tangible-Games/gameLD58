#pragma once

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <nlohmann/json.hpp>
#include <symphony_lite/all_symphony.hpp>

#include "symphony_lite/vector2d.hpp"
#include "utils.hpp"

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
          config["acceleration"]["y"]["capturing"].get<float>();
      ret.accelerationFalling =
          config["acceleration"]["y"]["falling"].get<float>();
      ret.accelerationWalking =
          config["acceleration"]["x"]["walking"].get<float>();
      ret.accelerationRunning =
          config["acceleration"]["x"]["running"].get<float>();
      ret.velocityDeadly = config["velocity"]["y"]["deadly"].get<float>();
      ret.velocityXMax = config["velocity"]["x"]["max"].get<float>();
      ret.captureDelay = config["capture_delay"].get<float>();
    } else {
      LOGE("Failed to load {}", kHumanConfigPath);
    }

    return ret;
  }

  static HumanConfiguration& configuration() {
    static HumanConfiguration config{HumanConfiguration::load()};
    return config;
  }

  float accelerationWalking{0};
  float accelerationRunning{0};
  float accelerationCapturing{0};
  float accelerationFalling{0};
  float velocityDeadly{0};
  float velocityXMax{0};
  float captureDelay{0};
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
        const Symphony::Math::AARect2d& r, float maxX)
      : rect(r),
        groundY_(r.center.y),
        renderer_(renderer),
        texture_(HumanTexture::texture(renderer)),
        configuration_(HumanConfiguration::configuration()),
        maxX_{maxX} {}

  bool Update(float dt) {
    if (captured_) {
      if (!prevCaptured_) {
        LOGD("Capturing started");
        capturedDelay_ = configuration_.captureDelay;
        acc_.x = -std::copysign(configuration_.accelerationRunning, acc_.x);
        prevCaptured_ = true;
      }
    } else if (rect.center.y < groundY_) {
      LOGD("Run away");
      acc_.y += configuration_.accelerationFalling * dt;
      if (prevCaptured_) {
        // Reset direction and acceleration
        LOGD("Forget target");
        prevCaptured_ = false;
        targetX_ = -1;
        acc_.x = 0;
      }
    }
    if (captured_) {
      if (capturedDelay_ > 0) {
        capturedDelay_ -= dt;
        LOGD_IF(capturedDelay_ <= 0, "Start tracking up");
      }
      if (capturedDelay_ <= 0) {
        acc_.y -= configuration_.accelerationCapturing * dt;
      }
    }

    if (targetX_ < 0.0) {
      auto newDirection = randMinusOneToOne() * maxX_;
      acc_.x = std::copysign(configuration_.accelerationWalking, newDirection);
      targetX_ = rect.center.x + newDirection;
      if (targetX_ < 0) {
        targetX_ = maxX_ + targetX_;
      } else if (targetX_ > maxX_) {
        targetX_ = targetX_ - maxX_;
      }
      LOGD("New target: {}, {}", newDirection, targetX_);
    } else if (std::abs(targetX_ - rect.center.x) < 0.1) {
      LOGD("Reset target");
      targetX_ = -1;
      acc_.x = 0;
    }

    auto v = acc_ * dt;
    v.x = std::clamp(v.x, -configuration_.velocityXMax,
                     configuration_.velocityXMax);

    rect.center.y += v.y * dt;
    if (rect.center.y > groundY_) {
      rect.center.y = groundY_;
      acc_.y = 0;
      if (v.y > configuration_.velocityDeadly) {
        // TODO: Do something to handle death
        return false;
      }
    }
    rect.center.x += v.x * dt;
    if (rect.center.x < 0) {
      rect.center.x = maxX_ + rect.center.x;
    } else if (rect.center.x > maxX_) {
      rect.center.x = rect.center.x - maxX_;
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
  bool prevCaptured_{false};
  float maxX_;
  float targetX_{-1};
  float capturedDelay_;
};

}  // namespace gameLD58