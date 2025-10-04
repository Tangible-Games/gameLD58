#pragma once

#include <SDL3/SDL.h>

#include <symphony_lite/all_symphony.hpp>

#include "demo.hpp"
#include "fade_image.hpp"
#include "keyboard.hpp"

namespace gameLD58 {
class Game : public Keyboard::Callback {
 public:
  Game(std::shared_ptr<SDL_Renderer> renderer,
       std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer),
        audio_(audio),
        loading_(renderer, audio, "assets/loading.png"),
        demo_(renderer, audio),
        quit_dialog_(renderer, audio, "assets/quit_dialog.png") {
    LOGD("Game is in state 'State::kJustStarted'.");
  }

  bool IsRunning() const { return is_running_; }

  void Quit() { is_running_ = false; }

  void Update(float dt);
  bool ReadyForLoading() const { return ready_for_loading_; }
  void Load();
  void Draw();

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

 private:
  enum class State {
    kJustStarted,
    kFirstLoading,
    kFadeToDemo,
    kDemo,
    kQuitDialog,
    kQuitDialogFadeOut,
  };

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  bool is_running_{true};
  bool ready_for_loading_{false};
  FadeImage loading_;
  Demo demo_;  // Delete in real game.
  FadeImage quit_dialog_;
  bool show_quit_dialog_{false};
  State state_{State::kJustStarted};
  int just_started_updates_{60};
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

    case State::kFadeToDemo:
      if (loading_.IsIdle()) {
        state_ = State::kDemo;
        Keyboard::Instance().RegisterCallback(this);
        LOGD("Game switches to state 'State::kDemo'.");
      }
      break;

    case State::kDemo:
      break;

    case State::kQuitDialog:
      quit_dialog_.Update(dt);
      break;

    case State::kQuitDialogFadeOut:
      quit_dialog_.Update(dt);
      if (quit_dialog_.IsIdle()) {
        state_ = State::kDemo;
        LOGD("Game switches to state 'State::kDemo'.");
      }
      break;
  }
}

void Game::Load() {
  if (state_ == State::kFirstLoading) {
    demo_.Load();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    ready_for_loading_ = false;

    loading_.StartFadeOut(1.0f);
    state_ = State::kFadeToDemo;
    LOGD("Game switches to state 'State::kFadeToDemo'.");
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
    case State::kFadeToDemo:
      demo_.Draw();
      loading_.Draw();
      break;
    case State::kDemo:
    case State::kQuitDialog:
    case State::kQuitDialogFadeOut:
      demo_.Draw();
      if (show_quit_dialog_) {
        quit_dialog_.Draw();
      }
      break;
  }
}

void Game::OnKeyDown(Keyboard::Key /*key*/) {}

void Game::OnKeyUp(Keyboard::Key key) {
  if (state_ == State::kDemo) {
    if (key == Keyboard::Key::kSelect) {
      show_quit_dialog_ = true;
      quit_dialog_.MakeSolid();

      state_ = State::kQuitDialog;
      LOGD("Game switches to state 'State::kQuitDialog'.");
    }
  } else if (state_ == State::kQuitDialog) {
    if (key == Keyboard::Key::kSelect) {
      is_running_ = false;
      LOGD("Game quits.");
    } else if (key == Keyboard::Key::kCircle) {
      quit_dialog_.StartFadeOut(0.5f);
      state_ = State::kQuitDialogFadeOut;
      LOGD("Game switches to state 'State::kQuitDialogFadeOut'.");
    }
  }
}
}  // namespace gameLD58
