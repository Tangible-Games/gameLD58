#pragma once

#include <SDL3/SDL.h>

#include <symphony_lite/all_symphony.hpp>

#include "demo.hpp"
#include "fade_image.hpp"

namespace gameLD58 {
class Game {
 public:
  Game(std::shared_ptr<SDL_Renderer> renderer,
       std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer),
        audio_(audio),
        loading_(renderer, audio, "assets/loading.png"),
        demo_(renderer, audio) {
    LOGD("Game is in state 'State::kJustStarted'.");
  }

  bool IsRunning() const { return is_running_; }

  void Quit() { is_running_ = false; }

  void Update(float dt);
  bool ReadyForLoading() const { return ready_for_loading_; }
  void Load();
  void Draw();

 private:
  enum class State {
    kJustStarted,
    kFirstLoading,
    kFadeToDemo,
    kDemo,
  };

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  bool is_running_{true};
  bool ready_for_loading_{false};
  FadeImage loading_;
  Demo demo_;  // Delete in real game.
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
        LOGD("Game switches to state 'State::kDemo'.");
      }
      break;

    case State::kDemo:
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
      demo_.Draw();
      break;
  }
  loading_.Draw();
}
}  // namespace gameLD58
