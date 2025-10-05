#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "consts.hpp"
#include "draw_texture.hpp"
#include "fade_image.hpp"
#include "keyboard.hpp"

namespace gameLD58 {
class MarketScreen : public Keyboard::Callback {
 public:
  class Callback {
   public:
    virtual void BackFromMarketScreen() = 0;
    virtual void TryExitFromMarketScreen() = 0;
  };

  MarketScreen(std::shared_ptr<SDL_Renderer> renderer,
               std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer), audio_(audio) {}

  void Load(
      std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
      const std::string& default_font);

  void Show();

  void Update(float dt);
  void Draw();

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

  void RegisterCallback(Callback* callback);

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts_;
  std::string default_font_;
  std::shared_ptr<SDL_Texture> image_;
  Callback* callback_{nullptr};
};

void MarketScreen::Load(
    std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
    const std::string& default_font) {
  known_fonts_ = known_fonts;
  default_font_ = default_font;

  image_.reset(IMG_LoadTexture(renderer_.get(), "assets/base.png"),
               &SDL_DestroyTexture);
}

void MarketScreen::Show() {}

void MarketScreen::Update(float /*dt*/) {}

void MarketScreen::Draw() {}

void MarketScreen::OnKeyDown(Keyboard::Key /*key*/) {}

void MarketScreen::OnKeyUp(Keyboard::Key key) {
  if (key == Keyboard::Key::kCircle) {
    if (callback_) {
      callback_->BackFromMarketScreen();
    }
  } else if (key == Keyboard::Key::kX) {
    // Sell:
  } else if (key == Keyboard::Key::kSelect) {
    if (callback_) {
      callback_->TryExitFromMarketScreen();
    }
  }
}

void MarketScreen::RegisterCallback(Callback* callback) {
  callback_ = callback;
}
}  // namespace gameLD58
