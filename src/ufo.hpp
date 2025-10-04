#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <symphony_lite/all_symphony.hpp>

#include "consts.hpp"
#include "keyboard.hpp"

namespace gameLD58 {
class Ufo : public Keyboard::Callback {
 public:
  Ufo(std::shared_ptr<SDL_Renderer> renderer,
      std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer), audio_(audio) {
    Keyboard::Instance().RegisterCallback(this);
  }

  void Load();
  void Draw();

  void Update(float dt);

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;

  float x_{kScreenWidth / 2.0};
  float y_{kScreenHeight / 2.0};
};

void Ufo::OnKeyDown(Keyboard::Key /*key*/) {}
void Ufo::OnKeyUp(Keyboard::Key /*key*/) {}

void Ufo::Update(float dt) {
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadLeft).has_value()) {
    x_ -= 10.0 * dt;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadRight).has_value()) {
    x_ += 10.0 * dt;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadUp).has_value()) {
    y_ -= 10.0 * dt;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadDown).has_value()) {
    y_ += 10.0 * dt;
  }
}

void Ufo::Load() {}

void Ufo::Draw() {
  SDL_FRect screen_rect = {x_, y_, 50, 20};
  SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
  SDL_RenderFillRect(renderer_.get(), &screen_rect);
}

}  // namespace gameLD58