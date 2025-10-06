#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "all_audio.hpp"
#include "consts.hpp"
#include "draw_texture.hpp"
#include "keyboard.hpp"

namespace gameLD58 {
class TitleScreen : public Keyboard::Callback {
 public:
  class Callback {
   public:
    virtual void ContinueFromTitleScreen() = 0;
    virtual void TryExitFromTitleScreen() = 0;
  };

  TitleScreen(std::shared_ptr<SDL_Renderer> renderer,
              std::shared_ptr<Symphony::Audio::Device> audio,
              AllAudio* all_audio)
      : renderer_(renderer), audio_(audio), all_audio_(all_audio) {}

  void Load();

  void Update(float dt);
  void Draw();

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

  void RegisterCallback(Callback* callback);

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  AllAudio* all_audio_{nullptr};
  std::shared_ptr<SDL_Texture> image_;
  Callback* callback_{nullptr};
};

void TitleScreen::Load() {
  image_.reset(IMG_LoadTexture(renderer_.get(), "assets/title_screen.png"),
               &SDL_DestroyTexture);
}

void TitleScreen::Update(float /*dt*/) {}

void TitleScreen::Draw() {
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

void TitleScreen::OnKeyDown(Keyboard::Key /*key*/) {}

void TitleScreen::OnKeyUp(Keyboard::Key key) {
  if (key == Keyboard::Key::kX) {
    if (callback_) {
      audio_->Play(all_audio_->audio[Sound::kButtonClick],
                   Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);

      callback_->ContinueFromTitleScreen();
    }
  } else if (key == Keyboard::Key::kSelect) {
    if (callback_) {
      audio_->Play(all_audio_->audio[Sound::kButtonClick],
                   Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);

      callback_->TryExitFromTitleScreen();
    }
  }
}

void TitleScreen::RegisterCallback(Callback* callback) { callback_ = callback; }
}  // namespace gameLD58
