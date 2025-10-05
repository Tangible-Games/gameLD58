#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <symphony_lite/all_symphony.hpp>

#include "symphony_lite/vector2d.hpp"

namespace gameLD58 {

struct HumanTexture {
  static std::shared_ptr<SDL_Texture> get(
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
        texture_(HumanTexture::get(renderer)) {}

  void Update(float dt) {
    (void)dt;
    if (captured_) {
      acc_.y -= 2000 * dt;
    } else if (rect.center.y < groundY_) {
      acc_.y += 5000 * dt;
    }

    auto v = acc_ * dt;
    rect.center.y += v.y * dt;
    if (rect.center.y > groundY_) {
      if (v.y > 30.0) {
        LOGD("Dead");
        // TODO
      }
      rect.center.y = groundY_;
      acc_.y = 0;
    }
  }

  void DrawTo(const SDL_FRect& r) {
    SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, &r);
  }

  Symphony::Math::AARect2d rect;

  float groundY_;

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<SDL_Texture> texture_;
  std::shared_ptr<SDL_Texture> textureCaptured_;
  Symphony::Math::Vector2d acc_;
  bool captured_{false};
};

}  // namespace gameLD58