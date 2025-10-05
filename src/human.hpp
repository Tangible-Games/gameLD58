#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <symphony_lite/all_symphony.hpp>

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
      : rect(r), renderer_(renderer), texture_(HumanTexture::get(renderer)) {}

  void Update(float dt) { (void)dt; }

  void DrawTo(const SDL_FRect& r) {
    SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, &r);
  }

  Symphony::Math::AARect2d rect;

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<SDL_Texture> texture_;
};

}  // namespace gameLD58