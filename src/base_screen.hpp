#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "consts.hpp"
#include "draw_texture.hpp"
#include "fade_image.hpp"
#include "keyboard.hpp"
#include "market_rules.hpp"

namespace gameLD58 {
class BaseScreen : public Keyboard::Callback {
 public:
  class Callback {
   public:
    virtual void ToMarketFromBaseScreen() = 0;
    virtual void ContinueFromBaseScreen() = 0;
    virtual void TryExitFromBaseScreen() = 0;
  };

  BaseScreen(std::shared_ptr<SDL_Renderer> renderer,
             std::shared_ptr<Symphony::Audio::Device> audio,
             AllAudio* all_audio)
      : renderer_(renderer), audio_(audio), all_audio_(all_audio) {}

  void Load(
      std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
      const std::string& default_font);

  void Show(const PlayerStatus* player_status_);

  void Update(float dt);
  void Draw();

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

  void RegisterCallback(Callback* callback);

 private:
  struct StatusItem {
    Symphony::Text::TextRenderer text_renderer;
  };

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  AllAudio* all_audio_{nullptr};
  std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts_;
  std::string default_font_;
  std::shared_ptr<SDL_Texture> image_;
  std::shared_ptr<SDL_Texture> market_button_image_;
  const PlayerStatus* player_status_{nullptr};
  StatusItem credits_earned;
  StatusItem humans_captured;
  StatusItem levels_completed;
  StatusItem best_price;
  Callback* callback_{nullptr};
};

void BaseScreen::Load(
    std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
    const std::string& default_font) {
  known_fonts_ = known_fonts;
  default_font_ = default_font;

  image_.reset(IMG_LoadTexture(renderer_.get(), "assets/base.png"),
               &SDL_DestroyTexture);

  market_button_image_.reset(
      IMG_LoadTexture(renderer_.get(), "assets/market_button.png"),
      &SDL_DestroyTexture);

  std::ifstream file;

  file.open("assets/base.json");
  if (!file.is_open()) {
    LOGE("Failed to load {}", "assets/base.json");
    return;
  }

  nlohmann::json base_screen_json = nlohmann::json::parse(file);
  file.close();

  const auto& credits_earned_json =
      base_screen_json["base_screen"]["credits_earned"];
  credits_earned.text_renderer.InitRenderer(renderer_);
  credits_earned.text_renderer.LoadFromFile(credits_earned_json["file_path"]);
  credits_earned.text_renderer.SetPosition(credits_earned_json.value("x", 0),
                                           credits_earned_json.value("y", 0));
  credits_earned.text_renderer.SetSizes(credits_earned_json.value("width", 0),
                                        credits_earned_json.value("height", 0));

  const auto& humans_captured_json =
      base_screen_json["base_screen"]["humans_captured"];
  humans_captured.text_renderer.InitRenderer(renderer_);
  humans_captured.text_renderer.LoadFromFile(humans_captured_json["file_path"]);
  humans_captured.text_renderer.SetPosition(humans_captured_json.value("x", 0),
                                            humans_captured_json.value("y", 0));
  humans_captured.text_renderer.SetSizes(
      humans_captured_json.value("width", 0),
      humans_captured_json.value("height", 0));

  const auto& levels_completed_json =
      base_screen_json["base_screen"]["levels_completed"];
  levels_completed.text_renderer.InitRenderer(renderer_);
  levels_completed.text_renderer.LoadFromFile(
      levels_completed_json["file_path"]);
  levels_completed.text_renderer.SetPosition(
      levels_completed_json.value("x", 0), levels_completed_json.value("y", 0));
  levels_completed.text_renderer.SetSizes(
      levels_completed_json.value("width", 0),
      levels_completed_json.value("height", 0));

  const auto& best_price_json = base_screen_json["base_screen"]["best_price"];
  best_price.text_renderer.InitRenderer(renderer_);
  best_price.text_renderer.LoadFromFile(best_price_json["file_path"]);
  best_price.text_renderer.SetPosition(best_price_json.value("x", 0),
                                       best_price_json.value("y", 0));
  best_price.text_renderer.SetSizes(best_price_json.value("width", 0),
                                    best_price_json.value("height", 0));
}

void BaseScreen::Show(const PlayerStatus* player_status) {
  player_status_ = player_status;

  std::string credits_earned_str =
      std::to_string(player_status_->credits_earned);
  std::string credits_earned_of_str =
      std::to_string(player_status_->credits_earned_of);
  credits_earned.text_renderer.ReFormat(
      {{"credits", credits_earned_str}, {"credits_of", credits_earned_of_str}},
      default_font_, known_fonts_);

  std::string humans_captured_str =
      std::to_string(player_status_->humans_captured);
  humans_captured.text_renderer.ReFormat(
      {{"humans_captured", humans_captured_str}}, default_font_, known_fonts_);

  std::string levels_completed_str =
      std::to_string(player_status_->levels_completed);
  std::string levels_completed_of_str =
      std::to_string(player_status_->levels_completed_of);
  levels_completed.text_renderer.ReFormat(
      {{"levels_completed", levels_completed_str},
       {"levels_completed_of", levels_completed_of_str}},
      default_font_, known_fonts_);

  std::string best_price_str = std::to_string(player_status_->best_price);
  best_price.text_renderer.ReFormat({{"best_price", best_price_str}},
                                    default_font_, known_fonts_);
}

void BaseScreen::Update(float /*dt*/) {}

void BaseScreen::Draw() {
  {
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

  if (!player_status_->cur_captured_humanoids.empty()) {
    float texture_width = 0;
    float texture_height = 0;
    SDL_GetTextureSize(market_button_image_.get(), &texture_width,
                       &texture_height);

    SDL_FRect texture_rect = {0, 0, texture_width, texture_height};
    SDL_FRect screen_rect = {(kScreenWidth - texture_width) / 2.0f,
                             kScreenHeight - texture_height, texture_width,
                             texture_height};
    SDL_FColor color;
    color.a = 1.0f;
    color.r = 1.0f;
    color.g = 1.0f;
    color.b = 1.0f;
    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(market_button_image_.get(), SDL_BLENDMODE_BLEND);
    RenderTexture(renderer_, market_button_image_, &texture_rect, &screen_rect,
                  &color);
  }

  credits_earned.text_renderer.Render(0);
  humans_captured.text_renderer.Render(0);
  levels_completed.text_renderer.Render(0);
  best_price.text_renderer.Render(0);
}

void BaseScreen::OnKeyDown(Keyboard::Key /*key*/) {}

void BaseScreen::OnKeyUp(Keyboard::Key key) {
  if (key == Keyboard::Key::kX) {
    if (callback_) {
      audio_->Play(all_audio_->audio[Sound::kButtonClick],
                   Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);

      callback_->ContinueFromBaseScreen();
    }
  } else if (key == Keyboard::Key::kSquare) {
    // Repair shop:
  } else if (key == Keyboard::Key::kTriangle) {
    if (!player_status_->cur_captured_humanoids.empty()) {
      if (callback_) {
        audio_->Play(all_audio_->audio[Sound::kButtonClick],
                     Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);

        callback_->ToMarketFromBaseScreen();
      }
    }
  } else if (key == Keyboard::Key::kSelect) {
    if (callback_) {
      audio_->Play(all_audio_->audio[Sound::kButtonClick],
                   Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);

      callback_->TryExitFromBaseScreen();
    }
  }
}

void BaseScreen::RegisterCallback(Callback* callback) { callback_ = callback; }
}  // namespace gameLD58
