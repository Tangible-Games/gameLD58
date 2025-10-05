#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "consts.hpp"
#include "draw_texture.hpp"
#include "fade_image.hpp"
#include "keyboard.hpp"

namespace gameLD58 {
class BaseScreen : public Keyboard::Callback {
 public:
  class Callback {
   public:
    virtual void ContinueFromBaseScreen() = 0;
    virtual void TryExitFromBaseScreen() = 0;
  };

  BaseScreen(std::shared_ptr<SDL_Renderer> renderer,
             std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer), audio_(audio) {}

  void Load();

  void Update(float dt);
  void Draw();

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

  void RegisterCallback(Callback* callback);

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  std::shared_ptr<SDL_Texture> image_;
  Callback* callback_{nullptr};
};

void BaseScreen::Load() {
  image_.reset(IMG_LoadTexture(renderer_.get(), "assets/base.png"),
               &SDL_DestroyTexture);
}

void BaseScreen::Update(float /*dt*/) {}

void BaseScreen::Draw() {
  SDL_FRect screen_rect = {0, 0, kScreenWidth, kScreenHeight};
  SDL_FColor color;
  color.a = 1.0f;
  color.r = 1.0f;
  color.g = 1.0f;
  color.b = 1.0f;
  SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
  SDL_SetTextureBlendMode(image_.get(), SDL_BLENDMODE_BLEND);
  RenderTexture(renderer_, image_, &screen_rect, &screen_rect, &color);
}

void BaseScreen::OnKeyDown(Keyboard::Key /*key*/) {}

void BaseScreen::OnKeyUp(Keyboard::Key key) {
  if (key == Keyboard::Key::kX) {
    if (callback_) {
      callback_->ContinueFromBaseScreen();
    }
  } else if (key == Keyboard::Key::kSquare) {
    // Repair shop:
  } else if (key == Keyboard::Key::kTriangle) {
    // Info:
  } else if (key == Keyboard::Key::kSelect) {
    if (callback_) {
      callback_->TryExitFromBaseScreen();
    }
  }
}

void BaseScreen::RegisterCallback(Callback* callback) { callback_ = callback; }
}  // namespace gameLD58
