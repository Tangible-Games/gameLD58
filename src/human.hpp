#pragma once

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <nlohmann/json.hpp>
#include <symphony_lite/all_symphony.hpp>
#include <symphony_lite/animated_sprite.hpp>
#include <symphony_lite/sprite_sheet.hpp>

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
      ret.at_capture_change_dir_time_min =
          config["at_capture_change_dir_time_min"].get<float>();
      ret.at_capture_change_dir_time_max =
          config["at_capture_change_dir_time_max"].get<float>();
      ret.half_width = config["half_width"].get<float>();
      ret.half_height = config["half_height"].get<float>();

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
  float at_capture_change_dir_time_min{0.0f};
  float at_capture_change_dir_time_max{0.0f};
  float half_width{0};
  float half_height{0};
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

static float RandF(float min_value, float max_value) {
  return (max_value - min_value) * ((float)rand() / (float)RAND_MAX) +
         min_value;
}

class Human {
 public:
  Human(std::shared_ptr<SDL_Renderer> renderer, float posX, float posY,
        float maxX, std::shared_ptr<Symphony::Sprite::SpriteSheet> animation_sp)
      : configuration_(HumanConfiguration::configuration()),
        rect{Symphony::Math::AARect2d{
            {posX, posY - configuration_.half_height},
            {configuration_.half_width, configuration_.half_height}}},
        groundY_(rect.center.y),
        renderer_(renderer),
        texture_(HumanTexture::texture(renderer)),
        maxX_{maxX},
        animations_{animation_sp} {}

  enum class AnimState { Idle, WalkLeft, WalkRight, Capture, Fall, Dead };

  bool Update(float dt) {
    if (captured_) {
      if (!prevCaptured_) {
        LOGD("Capturing started");
        capturedDelay_ = configuration_.captureDelay;
        at_capture_change_direction_delay_ =
            RandF(configuration_.at_capture_change_dir_time_min,
                  configuration_.at_capture_change_dir_time_max);
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
        if (at_capture_change_direction_delay_ > 0.0f) {
          at_capture_change_direction_delay_ -= dt;
          if (at_capture_change_direction_delay_ < 0.0f) {
            at_capture_change_direction_delay_ = 0.0f;
            acc_.x = -acc_.x;

            LOGD("Change direction.");
          }
        }

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
        dead_ = true;
        return true;
      }
    }
    rect.center.x += v.x * dt;
    if (rect.center.x < 0) {
      rect.center.x = maxX_ + rect.center.x;
    } else if (rect.center.x > maxX_) {
      rect.center.x = rect.center.x - maxX_;
    }

    UpdateAnimationState(dt);
    animations_.Update(dt);

    return true;
  }

  void DrawTo(const SDL_FRect& r) { animations_.Draw(renderer_, r); }

  void UpdateAnimationState(float dt) {
    AnimState next = state_;

    bool is_falling =
        !captured_ && rect.center.y < groundY_ - 1e-4f && acc_.y > 1e-4f;
    bool is_flying = captured_ && rect.center.y < groundY_ - 10.f;

    if (dead_) {
      next = AnimState::Dead;
      death_timer_ += dt;
      if (death_timer_ > 1.f) {
        next = AnimState::Idle;
      }
    } else if (!captured_) {
      next = (acc_.x >= 0) ? AnimState::WalkRight : AnimState::WalkLeft;
    } else if (is_falling) {
      next = AnimState::Fall;
    } else if (is_flying) {
      next = AnimState::Capture;
    }

    if (next != state_) {
      state_ = next;
      switch (state_) {
        case AnimState::Idle:
          animations_.Stop();
          break;
        case AnimState::WalkLeft:
          animations_.Play("walk_left", 50, true);
          break;
        case AnimState::WalkRight:
          animations_.Play("walk", 50, true);
          break;
        case AnimState::Capture:
          animations_.Play("capture", 50, true);
          break;
        case AnimState::Fall:
          animations_.Play("fall", 50, true);
          break;
        case AnimState::Dead:
          animations_.Play("crash", 50, false);
          break;
      }
    }
  }

  HumanConfiguration configuration_;
  Symphony::Math::AARect2d rect;
  float groundY_;
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<SDL_Texture> texture_;
  Symphony::Math::Vector2d acc_;
  bool captured_{false};
  bool prevCaptured_{false};
  float maxX_;
  float targetX_{-1};
  float capturedDelay_;
  float at_capture_change_direction_delay_{0.0f};
  Symphony::Sprite::AnimatedSprite animations_;
  AnimState state_{AnimState::Idle};
  bool dead_{false};
  float death_timer_{0.0f};
};

}  // namespace gameLD58
