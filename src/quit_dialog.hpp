#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "consts.hpp"
#include "draw_texture.hpp"
#include "fade_image.hpp"
#include "keyboard.hpp"

namespace gameLD58 {
class QuitDialog : public Keyboard::Callback {
 public:
  class Callback {
   public:
    virtual void BackToGame() = 0;
    virtual void QuitGame() = 0;
  };

  QuitDialog(std::shared_ptr<SDL_Renderer> renderer,
             std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer), audio_(audio), image_(renderer, audio, "") {}

  void Load();

  void Show();
  void Update(float dt);
  void Draw();

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

  void RegisterCallback(Callback* callback);

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  FadeImage image_;
  bool fadening_out_{false};
  Callback* callback_{nullptr};
};

void QuitDialog::Load() { image_.Load("assets/quit_dialog.png"); }

void QuitDialog::Show() {
  image_.MakeSolid();
  fadening_out_ = false;
}

void QuitDialog::Update(float dt) {
  image_.Update(dt);

  if (fadening_out_) {
    if (image_.IsIdle()) {
      if (callback_) {
        callback_->BackToGame();
      }
    }
  }
}

void QuitDialog::Draw() { image_.Draw(); }

void QuitDialog::OnKeyDown(Keyboard::Key /*key*/) {}

void QuitDialog::OnKeyUp(Keyboard::Key key) {
  if (!fadening_out_) {
    if (key == Keyboard::Key::kX) {
      if (callback_) {
        callback_->QuitGame();
      }
    } else if (key == Keyboard::Key::kCircle || key == Keyboard::Key::kSelect) {
      image_.StartFadeOut(0.5f);
      fadening_out_ = true;
    }
  }
}

void QuitDialog::RegisterCallback(Callback* callback) { callback_ = callback; }
}  // namespace gameLD58
