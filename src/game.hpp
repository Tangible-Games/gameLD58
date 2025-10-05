#pragma once

#include <SDL3/SDL.h>

#include <symphony_lite/all_symphony.hpp>

#include "fade_image.hpp"
#include "keyboard.hpp"
#include "level.hpp"
#include "quit_dialog.hpp"
#include "title_screen.hpp"

namespace gameLD58 {
class Game : public TitleScreen::Callback, public QuitDialog::Callback {
 public:
  Game(std::shared_ptr<SDL_Renderer> renderer,
       std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer),
        audio_(audio),
        loading_(renderer, audio, "assets/loading.png"),
        title_screen_(renderer, audio),
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

  void ToGame() override;
  void TryExitFromTitleScreen() override;

  void BackToGame() override;
  void QuitGame() override;

 private:
  enum class State {
    kJustStarted,
    kFirstLoading,
    kFadeToTitleScreen,
    kTitleScreen,
    kToGameFadeIn,
    kToGameFadeOut,
    kGame,
  };

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  bool is_running_{true};
  bool ready_for_loading_{false};
  FadeImage loading_;
  TitleScreen title_screen_;
  FadeImage fade_in_out_;
  Level level_;
  QuitDialog quit_dialog_;
  bool show_quit_dialog_{false};
  Keyboard::Callback* prev_keyboard_callback_{nullptr};
  State state_{State::kJustStarted};
  int just_started_updates_{30};
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
        state_ = State::kTitleScreen;
        title_screen_.RegisterCallback(this);
        Keyboard::Instance().RegisterCallback(&title_screen_);
        LOGD("Game switches to state 'State::kTitleScreen'.");
      }
      break;

    case State::kTitleScreen:
      break;

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
    level_.Load();
    title_screen_.Load();
    fade_in_out_.Load("assets/fade_in_out.png");
    quit_dialog_.Load();

    // std::this_thread::sleep_for(std::chrono::seconds(5));

    ready_for_loading_ = false;

    loading_.StartFadeOut(1.0f);
    state_ = State::kFadeToTitleScreen;
    LOGD("Game switches to state 'State::kFadeToTitleScreen'.");
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
    case State::kToGameFadeIn:
      title_screen_.Draw();
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

void Game::ToGame() {
  fade_in_out_.StartFadeIn(0.5f);
  state_ = State::kToGameFadeIn;
  LOGD("Game switches to state 'State::kToGameFadeIn'.");
}

void Game::TryExitFromTitleScreen() {
  show_quit_dialog_ = true;
  quit_dialog_.Show();
  quit_dialog_.RegisterCallback(this);
  prev_keyboard_callback_ =
      Keyboard::Instance().RegisterCallback(&quit_dialog_);

  LOGD("Game shows Quit dialog from Title screen.");
}

void Game::BackToGame() {
  show_quit_dialog_ = false;
  Keyboard::Instance().RegisterCallback(prev_keyboard_callback_);
  LOGD("Quit dialog requests going back to game.");
}

void Game::QuitGame() {
  is_running_ = false;
  LOGD("Quit dialog requests quitting.");
}
}  // namespace gameLD58
