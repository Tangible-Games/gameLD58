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
  Symphony::Text::TextRenderer humanoid_text_;
  const PlayerStatus* player_status_{nullptr};
  std::list<KnownHumanoid>::const_iterator cur_humanoid_it_;
  int cur_humanoid_index_{0};
  Callback* callback_{nullptr};
};

void MarketScreen::Load(
    std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
    const std::string& default_font) {
  known_fonts_ = known_fonts;
  default_font_ = default_font;

  image_.reset(IMG_LoadTexture(renderer_.get(), "assets/market.png"),
               &SDL_DestroyTexture);

  std::ifstream file;

  file.open("assets/market.json");
  if (!file.is_open()) {
    LOGE("Failed to load {}", "assets/market.json");
    return;
  }

  nlohmann::json market_screen_json = nlohmann::json::parse(file);
  file.close();

  const auto& humanoid_json = market_screen_json["market_screen"]["humanoid"];

  humanoid_text_.InitRenderer(renderer_);
  humanoid_text_.LoadFromFile(humanoid_json["file_path"]);
  humanoid_text_.SetPosition(humanoid_json.value("x", 0),
                             humanoid_json.value("y", 0));
  humanoid_text_.SetSizes(humanoid_json.value("width", 0),
                          humanoid_json.value("height", 0));
}

void MarketScreen::Show(const PlayerStatus* player_status) {
  player_status_ = player_status;
  cur_humanoid_it_ = player_status_->cur_captured_humanoids.begin();
  cur_humanoid_index_ = 0;

  std::map<std::string, std::string> humanoid_variables;
  for (const auto& [trait, trait_value] : cur_humanoid_it_->traits) {
    humanoid_variables[trait] = trait;
    humanoid_variables[trait + "_value"] = trait_value;
  }

  humanoid_variables["index_plus_1"] = std::to_string(cur_humanoid_index_ + 1);

  humanoid_text_.ReFormat(humanoid_variables, default_font_, known_fonts_);
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

  humanoid_text_.Render(0);
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
