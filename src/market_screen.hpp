#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "all_audio.hpp"
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
               std::shared_ptr<Symphony::Audio::Device> audio,
               AllAudio* all_audio)
      : renderer_(renderer), audio_(audio), all_audio_(all_audio) {}

  void Load(
      const MarketRules* market_rules,
      std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
      const std::string& default_font);

  void Show(PlayerStatus* player_status, size_t cur_alien_index);

  void Update(float dt);
  void Draw();

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

  void RegisterCallback(Callback* callback);

 private:
  void reFormatHumanoid();
  void reFormatAlien();
  void reFormatCredits();
  void reFormatAlienReply();
  void reFormatReceipt();

  struct Alien {
    std::shared_ptr<SDL_Texture> portrait;
  };

  enum class State {
    kShowWare,
    kAlienReply,
    kReceipt,
    kAllSold,
  };

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  AllAudio* all_audio_{nullptr};
  const MarketRules* market_rules_{nullptr};
  std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts_;
  std::string default_font_;
  State state_{State::kShowWare};
  float no_button_time_{1.0f};
  const float no_button_timeout_{0.25f};
  std::shared_ptr<SDL_Texture> image_;
  Symphony::Text::TextRenderer humanoid_text_;
  Symphony::Text::TextRenderer alien_text_;
  Symphony::Text::TextRenderer credits_text_;
  std::shared_ptr<SDL_Texture> alien_reply_image_;
  Symphony::Text::TextRenderer alien_reply_text_;
  std::shared_ptr<SDL_Texture> receipt_image_;
  Symphony::Text::TextRenderer receipt_text_;
  int alien_reply_image_x_{0};
  int alien_reply_image_y_{0};
  std::vector<Alien> aliens_;
  // Can modify when selling:
  PlayerStatus* player_status_{nullptr};
  bool is_alien_happy_{false};
  int alien_pays_{0};
  int alien_pays_after_vat_{0};
  size_t cur_alien_index_{0};
  int alien_x_{0};
  int alien_y_{0};
  int alien_width_{0};
  int alien_height_{0};
  int receipt_y_{0};
  std::list<KnownHumanoid>::const_iterator cur_humanoid_it_;
  int cur_humanoid_index_{0};
  Callback* callback_{nullptr};
};

void MarketScreen::Load(
    const MarketRules* market_rules,
    std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
    const std::string& default_font) {
  market_rules_ = market_rules;
  known_fonts_ = known_fonts;
  default_font_ = default_font;

  image_.reset(IMG_LoadTexture(renderer_.get(), "assets/market.png"),
               &SDL_DestroyTexture);

  alien_reply_image_.reset(
      IMG_LoadTexture(renderer_.get(), "assets/talking_bubble.png"),
      &SDL_DestroyTexture);

  receipt_image_.reset(IMG_LoadTexture(renderer_.get(), "assets/receipt.png"),
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

  const auto& alien_json = market_screen_json["market_screen"]["alien"];
  alien_x_ = alien_json.value("x", 0);
  alien_y_ = alien_json.value("y", 0);
  alien_width_ = alien_json.value("width", 0);
  alien_height_ = alien_json.value("height", 0);

  const auto& alien_text_json =
      market_screen_json["market_screen"]["alien_text"];
  alien_text_.InitRenderer(renderer_);
  alien_text_.LoadFromFile(alien_text_json["file_path"]);
  alien_text_.SetPosition(alien_text_json.value("x", 0),
                          alien_text_json.value("y", 0));
  alien_text_.SetSizes(alien_text_json.value("width", 0),
                       alien_text_json.value("height", 0));

  const auto& credits_json = market_screen_json["market_screen"]["credits"];
  credits_text_.InitRenderer(renderer_);
  credits_text_.LoadFromFile(credits_json["file_path"]);
  credits_text_.SetPosition(credits_json.value("x", 0),
                            credits_json.value("y", 0));
  credits_text_.SetSizes(credits_json.value("width", 0),
                         credits_json.value("height", 0));

  const auto& alien_reply_image_json =
      market_screen_json["market_screen"]["alien_reply_image"];
  alien_reply_image_x_ = alien_reply_image_json.value("x", 0);
  alien_reply_image_y_ = alien_reply_image_json.value("y", 0);

  const auto& alien_reply_json =
      market_screen_json["market_screen"]["alien_reply"];
  alien_reply_text_.InitRenderer(renderer_);
  alien_reply_text_.LoadFromFile(alien_reply_json["file_path"]);
  alien_reply_text_.SetPosition(alien_reply_json.value("x", 0),
                                alien_reply_json.value("y", 0));
  alien_reply_text_.SetSizes(alien_reply_json.value("width", 0),
                             alien_reply_json.value("height", 0));

  const auto& receipt_image_json =
      market_screen_json["market_screen"]["receipt_image"];
  receipt_y_ = receipt_image_json.value("y", 0);

  const auto& receipt_json = market_screen_json["market_screen"]["receipt"];
  receipt_text_.InitRenderer(renderer_);
  receipt_text_.LoadFromFile(receipt_json["file_path"]);
  receipt_text_.SetPosition(receipt_json.value("x", 0),
                            receipt_json.value("y", 0));
  receipt_text_.SetSizes(receipt_json.value("width", 0),
                         receipt_json.value("height", 0));

  aliens_.resize(market_rules_->known_aliens.size());
  for (size_t i = 0; i < aliens_.size(); ++i) {
    aliens_[i].portrait.reset(
        IMG_LoadTexture(
            renderer_.get(),
            market_rules->known_aliens[i].portrait_file_path.c_str()),
        &SDL_DestroyTexture);
  }
}

void MarketScreen::Show(PlayerStatus* player_status, size_t cur_alien_index) {
  player_status_ = player_status;
  cur_humanoid_it_ = player_status_->cur_captured_humanoids.begin();
  cur_humanoid_index_ = 0;

  cur_alien_index_ = cur_alien_index;

  state_ = State::kShowWare;
  no_button_time_ = no_button_timeout_;

  reFormatHumanoid();
  reFormatAlien();
  reFormatCredits();
}

void MarketScreen::Update(float dt) {
  no_button_time_ -= dt;
  if (no_button_time_ < 0.0f) {
    no_button_time_ = 0.0f;
  }
}

void MarketScreen::Draw() {
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

  {
    float texture_width = 0;
    float texture_height = 0;
    SDL_GetTextureSize(aliens_[cur_alien_index_].portrait.get(), &texture_width,
                       &texture_height);

    SDL_FRect texture_rect = {0, 0, texture_width, texture_height};
    SDL_FRect screen_rect = {alien_x_ + (alien_width_ - texture_width) / 2.0f,
                             alien_y_ + (alien_height_ - texture_height) / 2.0f,
                             texture_width, texture_height};
    SDL_FColor color;
    color.a = 1.0f;
    color.r = 1.0f;
    color.g = 1.0f;
    color.b = 1.0f;
    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(aliens_[cur_alien_index_].portrait.get(),
                            SDL_BLENDMODE_BLEND);
    RenderTexture(renderer_, aliens_[cur_alien_index_].portrait, &texture_rect,
                  &screen_rect, &color);
  }

  if (state_ == State::kShowWare) {
    humanoid_text_.Render(0);
  }

  if (state_ == State::kAlienReply) {
    float texture_width = 0;
    float texture_height = 0;
    SDL_GetTextureSize(alien_reply_image_.get(), &texture_width,
                       &texture_height);

    SDL_FRect texture_rect = {0, 0, texture_width, texture_height};
    SDL_FRect screen_rect = {(float)alien_reply_image_x_,
                             (float)alien_reply_image_y_, texture_width,
                             texture_height};
    SDL_FColor color;
    color.a = 1.0f;
    color.r = 1.0f;
    color.g = 1.0f;
    color.b = 1.0f;
    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(alien_reply_image_.get(), SDL_BLENDMODE_BLEND);
    RenderTexture(renderer_, alien_reply_image_, &texture_rect, &screen_rect,
                  &color);

    alien_reply_text_.Render(0);
  }

  alien_text_.Render(0);

  if (state_ == State::kReceipt) {
    float texture_width = 0;
    float texture_height = 0;
    SDL_GetTextureSize(receipt_image_.get(), &texture_width, &texture_height);

    SDL_FRect texture_rect = {0, 0, texture_width, texture_height};
    SDL_FRect screen_rect = {(float)(kScreenWidth - texture_width) / 2.0f,
                             (float)receipt_y_, texture_width, texture_height};
    SDL_FColor color;
    color.a = 1.0f;
    color.r = 1.0f;
    color.g = 1.0f;
    color.b = 1.0f;
    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(receipt_image_.get(), SDL_BLENDMODE_BLEND);
    RenderTexture(renderer_, receipt_image_, &texture_rect, &screen_rect,
                  &color);

    receipt_text_.Render(0);
  }

  credits_text_.Render(0);
}

void MarketScreen::OnKeyDown(Keyboard::Key /*key*/) {}

void MarketScreen::OnKeyUp(Keyboard::Key key) {
  if (no_button_time_ > 0.0f) {
    return;
  }

  bool need_humanoid_re_format = false;
  switch (state_) {
    case State::kShowWare:
      if (key == Keyboard::Key::kCircle) {
        if (callback_) {
          audio_->Play(all_audio_->audio[Sound::kButtonClick],
                       Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);

          callback_->BackFromMarketScreen();
        }
      } else if (key == Keyboard::Key::kDpadLeft) {
        if (cur_humanoid_index_ > 0) {
          --cur_humanoid_it_;
          --cur_humanoid_index_;
          need_humanoid_re_format = true;

          audio_->Play(all_audio_->audio[Sound::kHumanoidSelect],
                       Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);
        }
      } else if (key == Keyboard::Key::kDpadRight) {
        auto next_it = cur_humanoid_it_;
        ++next_it;

        if (next_it != player_status_->cur_captured_humanoids.end()) {
          cur_humanoid_it_ = next_it;
          ++cur_humanoid_index_;
          need_humanoid_re_format = true;

          audio_->Play(all_audio_->audio[Sound::kHumanoidSelect],
                       Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);
        }
      } else if (key == Keyboard::Key::kX) {
        size_t match_result = MatchHumanoidWithAlien(
            *market_rules_, *cur_humanoid_it_,
            market_rules_->known_aliens[cur_alien_index_]);
        // Matching all known traits:
        if (match_result == market_rules_->known_traits.size()) {
          is_alien_happy_ = true;
          alien_pays_ =
              market_rules_->known_aliens[cur_alien_index_].pays_when_likes;
        } else {
          is_alien_happy_ = false;
          alien_pays_ =
              market_rules_->known_aliens[cur_alien_index_].pays_when_dislikes;
        }

        alien_pays_after_vat_ =
            (int)((1.0f - market_rules_->vat) * alien_pays_);

        reFormatAlienReply();

        audio_->Play(all_audio_->audio[Sound::kButtonClick],
                     Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);

        state_ = State::kAlienReply;
        no_button_time_ = no_button_timeout_;
      } else if (key == Keyboard::Key::kSelect) {
        if (callback_) {
          callback_->TryExitFromMarketScreen();

          audio_->Play(all_audio_->audio[Sound::kButtonClick],
                       Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);
        }
      }
      break;

    case State::kAlienReply:
      if (key == Keyboard::Key::kX) {
        player_status_->credits_earned += alien_pays_after_vat_;
        reFormatCredits();

        reFormatReceipt();

        audio_->Play(all_audio_->audio[Sound::kKaChing],
                     Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);

        state_ = State::kReceipt;
        no_button_time_ = no_button_timeout_;
      }
      break;

    case State::kReceipt:
      if (key == Keyboard::Key::kX) {
        player_status_->cur_captured_humanoids.erase(cur_humanoid_it_);

        cur_humanoid_it_ = player_status_->cur_captured_humanoids.begin();
        if (cur_humanoid_it_ == player_status_->cur_captured_humanoids.end()) {
          state_ = State::kAllSold;
        } else {
          cur_humanoid_index_ = 0;

          need_humanoid_re_format = true;

          state_ = State::kShowWare;
          no_button_time_ = no_button_timeout_;
        }

        audio_->Play(all_audio_->audio[Sound::kButtonClick],
                     Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);
      }
      break;

    case State::kAllSold:
      if (key == Keyboard::Key::kCircle) {
        if (callback_) {
          audio_->Play(all_audio_->audio[Sound::kButtonClick],
                       Symphony::Audio::PlayTimes(1), Symphony::Audio::kNoFade);

          callback_->BackFromMarketScreen();
        }
      }
      break;
  }

  if (need_humanoid_re_format) {
    reFormatHumanoid();
  }
}

void MarketScreen::RegisterCallback(Callback* callback) {
  callback_ = callback;
}

void MarketScreen::reFormatHumanoid() {
  std::map<std::string, std::string> humanoid_variables;
  for (const auto& [trait, trait_value] : cur_humanoid_it_->traits) {
    humanoid_variables[trait] = trait;
    humanoid_variables[trait + "_value"] = trait_value;
  }

  humanoid_variables["index_plus_1"] = std::to_string(cur_humanoid_index_ + 1);

  humanoid_text_.ReFormat(humanoid_variables, default_font_, known_fonts_);
}

void MarketScreen::reFormatAlien() {
  alien_text_.ReFormat(
      {{"name", market_rules_->known_aliens[cur_alien_index_].name}},
      default_font_, known_fonts_);
}

void MarketScreen::reFormatCredits() {
  credits_text_.ReFormat(
      {{"credits", std::to_string(player_status_->credits_earned)}},
      default_font_, known_fonts_);
}

void MarketScreen::reFormatAlienReply() {
  std::map<std::string, std::string> variables;

  if (is_alien_happy_) {
    variables["reply"] =
        market_rules_->known_aliens[cur_alien_index_].says_when_likes;
  } else {
    variables["reply"] =
        market_rules_->known_aliens[cur_alien_index_].says_when_dislikes;
  }

  variables["credits"] = std::to_string(alien_pays_);

  alien_reply_text_.ReFormat(variables, default_font_, known_fonts_);
}

void MarketScreen::reFormatReceipt() {
  std::map<std::string, std::string> variables;

  variables["credits"] = std::to_string(alien_pays_);

  receipt_text_.ReFormat(
      {{"date", ""},
       {"credits", std::to_string(alien_pays_)},
       {"vat", std::to_string(alien_pays_ - alien_pays_after_vat_)},
       {"credits_after_vat", std::to_string(alien_pays_after_vat_)}},
      default_font_, known_fonts_);
}
}  // namespace gameLD58
