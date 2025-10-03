#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <symphony_lite/all_symphony.hpp>

#include "consts.hpp"

namespace gameLD58 {
class Demo {
 public:
  Demo(std::shared_ptr<SDL_Renderer> renderer,
       std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer), audio_(audio) {}

  void Load();
  void Draw();

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
};

void Demo::Load() {}

void Demo::Draw() {
  SDL_FRect screen_rect = {0, 0, kScreenWidth, kScreenHeight};
  SDL_SetRenderDrawColor(renderer_.get(), 255, 128, 128, 128);
  SDL_RenderFillRect(renderer_.get(), &screen_rect);
}

}  // namespace gameLD58
