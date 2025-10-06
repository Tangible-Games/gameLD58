#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "consts.hpp"
#include "draw_texture.hpp"
#include "fade_image.hpp"
#include "keyboard.hpp"
#include "market_rules.hpp"

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

  void Show(const PlayerStatus* player_status);

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
  const PlayerStatus* player_status_{nullptr};
  Callback* callback_{nullptr};
};

void MarketScreen::Load(
    std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
    const std::string& default_font) {
  known_fonts_ = known_fonts;
  default_font_ = default_font;

  image_.reset(IMG_LoadTexture(renderer_.get(), "assets/market.png"),
               &SDL_DestroyTexture);
}

void MarketScreen::Show(const PlayerStatus* player_status) {
  player_status_ = player_status;
}

void MarketScreen::Update(float /*dt*/) {}

void MarketScreen::Draw() {
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
