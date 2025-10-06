#pragma once

#include <SDL3/SDL.h>

#include <nlohmann/json.hpp>
#include <symphony_lite/all_symphony.hpp>

#include "base_screen.hpp"
#include "fade_image.hpp"
#include "keyboard.hpp"
#include "level.hpp"
#include "market_rules.hpp"
#include "market_screen.hpp"
#include "quit_dialog.hpp"
#include "story_screen.hpp"
#include "title_screen.hpp"

namespace gameLD58 {
class Game : public TitleScreen::Callback,
             public StoryScreen::Callback,
             public BaseScreen::Callback,
             public MarketScreen::Callback,
             public Level::Callback,
             public QuitDialog::Callback {
 public:
  Game(std::shared_ptr<SDL_Renderer> renderer,
       std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer),
        audio_(audio),
        loading_(renderer, audio, "assets/loading.png"),
        title_screen_(renderer, audio),
        story_screen_(renderer, audio),
        base_screen_(renderer, audio),
        market_screen_(renderer, audio),
        fade_in_out_(renderer, audio, ""),
        level_(renderer, audio, "assets/level.json"),
        quit_dialog_(renderer, audio) {
    LOGD("Game is in state 'State::kJustStarted'.");
  }

  bool IsRunning() const { return is_running_; }

  void Quit() { is_running_ = false; }

  void Update(float dt);
  bool ReadyForLoading() const { return ready_for_loading_; }
  void Load();
  void Draw();

  void ContinueFromTitleScreen() override;
  void TryExitFromTitleScreen() override;

  void ContinueFromStoryScreen() override;
  void TryExitFromStoryScreen() override;

  void ToMarketFromBaseScreen() override;
  void ContinueFromBaseScreen() override;
  void TryExitFromBaseScreen() override;

  void BackFromMarketScreen() override;
  void TryExitFromMarketScreen() override;

  void TryExitFromLevel() override;

  void BackToGame() override;
  void QuitGame() override;

 private:
  enum class State {
    kJustStarted,
    kFirstLoading,
    kFadeToTitleScreen,
    kTitleScreen,
    kToStoryScreenFadeIn,
    kToStoryScreenFadeOut,
    kStoryScreen,
    kToBaseScreenFadeIn,
    kToBaseScreenFromMarketFadeIn,
    kToBaseScreenFadeOut,
    kBaseScreen,
    kToMarketScreenFadeIn,
    kToMarketScreenFadeOut,
    kMarketScreen,
    kToGameFadeIn,
    kToGameFadeOut,
    kGame,
  };

  void loadFonts();
  void loadRules();

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  bool is_running_{true};
  bool ready_for_loading_{false};
  FadeImage loading_;
  TitleScreen title_screen_;
  StoryScreen story_screen_;
  MarketRules market_rules_;
  PlayerStatus player_status_;
  size_t cur_alien_index_{0};
  BaseScreen base_screen_;
  MarketScreen market_screen_;
  float market_before_next_music_timeout_{0.0f};
  FadeImage fade_in_out_;
  Level level_;
  QuitDialog quit_dialog_;
  bool show_quit_dialog_{false};
  Keyboard::Callback* prev_keyboard_callback_{nullptr};
  State state_{State::kJustStarted};
  int just_started_updates_{30};
  std::shared_ptr<Symphony::Audio::WaveFile> menu_audio_;
  std::shared_ptr<Symphony::Audio::WaveFile> market_audio_;
  std::shared_ptr<Symphony::Audio::PlayingStream> menu_audio_stream_;
  std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts_;
  std::string default_font_;
};

void Game::Update(float dt) {
  loading_.Update(dt);

  switch (state_) {
    case State::kJustStarted:
      --just_started_updates_;
      if (just_started_updates_ == 0) {
        ready_for_loading_ = true;
        state_ = State::kFirstLoading;
        LOGD("Game switches to state 'State::kFirstLoading'.");
      }
      break;

    case State::kFirstLoading:
      break;

    case State::kFadeToTitleScreen:
      if (loading_.IsIdle()) {
        menu_audio_stream_ =
            audio_->Play(menu_audio_, Symphony::Audio::kPlayLooped,
                         Symphony::Audio::FadeInOut(2.0f, 1.0f));

        state_ = State::kTitleScreen;
        title_screen_.RegisterCallback(this);
        Keyboard::Instance().RegisterCallback(&title_screen_);
        LOGD("Game switches to state 'State::kTitleScreen'.");
      }
      break;

    case State::kTitleScreen:
      break;

    case State::kToStoryScreenFadeIn:
      fade_in_out_.Update(dt);
      if (fade_in_out_.IsIdle()) {
        fade_in_out_.StartFadeOut(0.5f);
        state_ = State::kToStoryScreenFadeOut;
        LOGD("Game switches to state 'State::kToStoryScreenFadeOut'.");
      }
      break;

    case State::kToStoryScreenFadeOut:
      fade_in_out_.Update(dt);
      if (fade_in_out_.IsIdle()) {
        state_ = State::kStoryScreen;
        story_screen_.RegisterCallback(this);
        Keyboard::Instance().RegisterCallback(&story_screen_);
        LOGD("Game switches to state 'State::kStoryScreen'.");
      }
      break;

    case State::kStoryScreen:
      break;

    case State::kToBaseScreenFadeIn:
    case State::kToBaseScreenFromMarketFadeIn:
      fade_in_out_.Update(dt);
      if (fade_in_out_.IsIdle()) {
        base_screen_.Show(&player_status_);
        fade_in_out_.StartFadeOut(0.5f);
        state_ = State::kToBaseScreenFadeOut;
        LOGD("Game switches to state 'State::kToBaseScreenFadeOut'.");
      }
      break;

    case State::kToBaseScreenFadeOut:
      fade_in_out_.Update(dt);
      if (fade_in_out_.IsIdle()) {
        state_ = State::kBaseScreen;
        base_screen_.RegisterCallback(this);
        Keyboard::Instance().RegisterCallback(&base_screen_);
        LOGD("Game switches to state 'State::kBaseScreen'.");
      }
      break;

    case State::kBaseScreen:
      break;

    case State::kToMarketScreenFadeIn:
      fade_in_out_.Update(dt);
      if (fade_in_out_.IsIdle()) {
        cur_alien_index_ = rand() % market_rules_.known_aliens.size();

        // Can modify player_status_ when selling:
        market_screen_.Show(&player_status_, cur_alien_index_);
        fade_in_out_.StartFadeOut(0.5f);
        state_ = State::kToMarketScreenFadeOut;
        LOGD("Game switches to state 'State::kToMarketScreenFadeOut'.");
      }
      break;

    case State::kToMarketScreenFadeOut:
      fade_in_out_.Update(dt);
      if (fade_in_out_.IsIdle()) {
        state_ = State::kMarketScreen;
        market_screen_.RegisterCallback(this);
        Keyboard::Instance().RegisterCallback(&market_screen_);
        LOGD("Game switches to state 'State::kMarketScreen'.");
      }
      break;

    case State::kMarketScreen: {
      market_screen_.Update(dt);
      if (market_before_next_music_timeout_ > 0.0f) {
        market_before_next_music_timeout_ -= dt;
        if (market_before_next_music_timeout_ < 0.0f) {
          menu_audio_stream_ =
              audio_->Play(market_audio_, Symphony::Audio::PlayTimes(2),
                           Symphony::Audio::FadeInOut(5.0f, 5.0f));
          market_before_next_music_timeout_ =
              market_audio_->GetLengthSec() * 2.0f + 2.0f + (float)(rand() % 4);
        }
      }
      break;
    }

    case State::kToGameFadeIn:
      fade_in_out_.Update(dt);
      if (fade_in_out_.IsIdle()) {
        fade_in_out_.StartFadeOut(0.5f);
        state_ = State::kToGameFadeOut;
        LOGD("Game switches to state 'State::kToGameFadeOut'.");
      }
      break;

    case State::kToGameFadeOut:
      fade_in_out_.Update(dt);
      if (fade_in_out_.IsIdle()) {
        audio_->Stop(menu_audio_stream_, Symphony::Audio::StopFade(0.5f));

        level_.RegisterCallback(this);
        Keyboard::Instance().RegisterCallback(&level_);

        level_.SetIsPaused(false);

        state_ = State::kGame;
        LOGD("Game switches to state 'State::kGame'.");
      }
      break;

    case State::kGame:
      level_.Update(dt);
      break;
  }

  if (show_quit_dialog_) {
    quit_dialog_.Update(dt);
  }
}

void Game::Load() {
  if (state_ == State::kFirstLoading) {
    loadFonts();
    loadRules();
    market_rules_ = LoadMarketRules();

    level_.Load();
    title_screen_.Load();
    story_screen_.Load(known_fonts_, default_font_);
    base_screen_.Load(known_fonts_, default_font_);
    market_screen_.Load(&market_rules_, known_fonts_, default_font_);
    fade_in_out_.Load("assets/fade_in_out.png");
    quit_dialog_.Load();

    menu_audio_ = Symphony::Audio::LoadWave(
        "assets/05_22k.wav", Symphony::Audio::WaveFile::kModeStreamingFromFile);
    market_audio_ = Symphony::Audio::LoadWave(
        "assets/14_22k.wav", Symphony::Audio::WaveFile::kModeStreamingFromFile);

    ready_for_loading_ = false;

    loading_.StartFadeOut(1.0f);
    state_ = State::kFadeToTitleScreen;
    LOGD("Game switches to state 'State::kFadeToTitleScreen'.");

    // Should be after level:
    std::list<KnownHumanoid> new_captured_humanoids =
        CaptureRandomHumanoids(market_rules_, 5);
    player_status_.cur_captured_humanoids.insert(
        player_status_.cur_captured_humanoids.end(),
        new_captured_humanoids.begin(), new_captured_humanoids.end());
  }
}

void Game::Draw() {
  switch (state_) {
    case State::kJustStarted:
      loading_.Draw();
      break;
    case State::kFirstLoading:
      loading_.Draw();
      break;
    case State::kFadeToTitleScreen:
      title_screen_.Draw();
      loading_.Draw();
      break;
    case State::kTitleScreen:
      title_screen_.Draw();
      break;
    case State::kToStoryScreenFadeIn:
      title_screen_.Draw();
      fade_in_out_.Draw();
      break;
    case State::kToStoryScreenFadeOut:
      story_screen_.Draw();
      fade_in_out_.Draw();
      break;
    case State::kStoryScreen:
      story_screen_.Draw();
      break;
    case State::kToBaseScreenFadeIn:
      story_screen_.Draw();
      fade_in_out_.Draw();
      break;
    case State::kToBaseScreenFromMarketFadeIn:
      market_screen_.Draw();
      fade_in_out_.Draw();
      break;
    case State::kToBaseScreenFadeOut:
      base_screen_.Draw();
      fade_in_out_.Draw();
      break;
    case State::kBaseScreen:
      base_screen_.Draw();
      break;
    case State::kToMarketScreenFadeIn:
      base_screen_.Draw();
      fade_in_out_.Draw();
      break;
    case State::kToMarketScreenFadeOut:
      market_screen_.Draw();
      fade_in_out_.Draw();
      break;
    case State::kMarketScreen:
      market_screen_.Draw();
      break;
    case State::kToGameFadeIn:
      base_screen_.Draw();
      fade_in_out_.Draw();
      break;
    case State::kToGameFadeOut:
      level_.Draw();
      fade_in_out_.Draw();
      break;
    case State::kGame:
      level_.Draw();
      break;
  }

  if (show_quit_dialog_) {
    quit_dialog_.Draw();
  }
}

void Game::ContinueFromTitleScreen() {
  if (state_ != State::kTitleScreen) {
    return;
  }

  fade_in_out_.StartFadeIn(0.5f);
  state_ = State::kToStoryScreenFadeIn;
  level_.Start(/*is_paused*/ true);
  LOGD("Game switches to state 'State::kToStoryScreenFadeIn'.");
}

void Game::TryExitFromTitleScreen() {
  if (state_ != State::kTitleScreen) {
    return;
  }

  show_quit_dialog_ = true;
  quit_dialog_.Show();
  quit_dialog_.RegisterCallback(this);
  prev_keyboard_callback_ =
      Keyboard::Instance().RegisterCallback(&quit_dialog_);

  LOGD("Game shows Quit dialog from Title screen.");
}

void Game::ContinueFromStoryScreen() {
  if (state_ != State::kStoryScreen) {
    return;
  }

  fade_in_out_.StartFadeIn(0.5f);
  state_ = State::kToBaseScreenFadeIn;
  LOGD("Game switches to state 'State::kToBaseScreenFadeIn'.");
}

void Game::TryExitFromStoryScreen() {
  if (state_ != State::kStoryScreen) {
    return;
  }

  show_quit_dialog_ = true;
  quit_dialog_.Show();
  quit_dialog_.RegisterCallback(this);
  prev_keyboard_callback_ =
      Keyboard::Instance().RegisterCallback(&quit_dialog_);

  LOGD("Game shows Quit dialog from Story screen.");
}

void Game::ToMarketFromBaseScreen() {
  if (state_ != State::kBaseScreen) {
    return;
  }

  audio_->Stop(menu_audio_stream_, Symphony::Audio::StopFade(0.5f));
  menu_audio_stream_ =
      audio_->Play(market_audio_, Symphony::Audio::PlayTimes(2),
                   Symphony::Audio::FadeInOut(5.0f, 5.0f));
  market_before_next_music_timeout_ =
      market_audio_->GetLengthSec() * 2.0f + 2.0f + (float)(rand() % 4);

  fade_in_out_.StartFadeIn(0.5f);
  state_ = State::kToMarketScreenFadeIn;
  LOGD("Game switches to state 'State::kToMarketScreenFadeIn'.");
}

void Game::ContinueFromBaseScreen() {
  if (state_ != State::kBaseScreen) {
    return;
  }

  fade_in_out_.StartFadeIn(0.5f);
  state_ = State::kToGameFadeIn;
  level_.Start(/*is_paused*/ true);
  LOGD("Game switches to state 'State::kToGameFadeIn'.");
}

void Game::TryExitFromBaseScreen() {
  if (state_ != State::kBaseScreen) {
    return;
  }

  show_quit_dialog_ = true;
  quit_dialog_.Show();
  quit_dialog_.RegisterCallback(this);
  prev_keyboard_callback_ =
      Keyboard::Instance().RegisterCallback(&quit_dialog_);

  LOGD("Game shows Quit dialog from Base screen.");
}

void Game::BackFromMarketScreen() {
  if (state_ != State::kMarketScreen) {
    return;
  }

  audio_->Stop(menu_audio_stream_, Symphony::Audio::StopFade(0.5f));
  menu_audio_stream_ = audio_->Play(menu_audio_, Symphony::Audio::kPlayLooped,
                                    Symphony::Audio::FadeInOut(2.0f, 1.0f));

  fade_in_out_.StartFadeIn(0.5f);
  state_ = State::kToBaseScreenFromMarketFadeIn;
  LOGD("Game switches to state 'State::kToBaseScreenFromMarketFadeIn'.");
}

void Game::TryExitFromMarketScreen() {
  if (state_ != State::kMarketScreen) {
    return;
  }

  show_quit_dialog_ = true;
  quit_dialog_.Show();
  quit_dialog_.RegisterCallback(this);
  prev_keyboard_callback_ =
      Keyboard::Instance().RegisterCallback(&quit_dialog_);

  LOGD("Game shows Quit dialog from Base screen.");
}

void Game::TryExitFromLevel() {
  if (state_ != State::kGame) {
    return;
  }

  show_quit_dialog_ = true;
  quit_dialog_.Show();
  quit_dialog_.RegisterCallback(this);
  prev_keyboard_callback_ =
      Keyboard::Instance().RegisterCallback(&quit_dialog_);

  level_.SetIsPaused(true);

  LOGD("Game shows Quit dialog from Level.");
}

void Game::BackToGame() {
  level_.SetIsPaused(false);

  show_quit_dialog_ = false;
  Keyboard::Instance().RegisterCallback(prev_keyboard_callback_);

  if (state_ == State::kGame) {
    level_.SetIsPaused(false);
  }

  LOGD("Quit dialog requests going back to game.");
}

void Game::QuitGame() {
  is_running_ = false;

  audio_->StopImmediately(menu_audio_stream_);

  LOGD("Quit dialog requests quitting.");
}

void Game::loadFonts() {
  std::ifstream file;

  file.open("assets/known_fonts.json");
  if (!file.is_open()) {
    LOGE("Failed to load {}", "assets/known_fonts.json");
    return;
  }

  nlohmann::json known_fonts_json = nlohmann::json::parse(file);
  file.close();

  for (const auto& font_json : known_fonts_json["known_fonts"]) {
    auto font = Symphony::Text::LoadBmFont(font_json["file_path"]);
    if (!font) {
      LOGE("Failed to load font {}", font_json["file_path"].get<std::string>());
      continue;
    }
    font->LoadTexture(renderer_);

    known_fonts_.insert(std::make_pair(font_json["style_name"], font));
  }

  default_font_ = known_fonts_json["default_font"];
}

void Game::loadRules() {
  std::ifstream file;

  file.open("assets/game.json");
  if (!file.is_open()) {
    LOGE("Failed to load {}", "assets/game.json");
    return;
  }

  nlohmann::json game_json = nlohmann::json::parse(file);
  file.close();

  player_status_.credits_earned_of =
      game_json["game"].value("credits_earned_of", 0);
}
}  // namespace gameLD58
