#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace gameLD58 {
class Keyboard {
 public:
  enum class Key {
    kDpadUp,
    kDpadDown,
    kDpadLeft,
    kDpadRight,
    kTriangle,
    kX,
    kSquare,
    kCircle,
    kStart,
  };

  class Callback {
   public:
    virtual void OnKeyDown(Key key) = 0;
    virtual void OnKeyUp(Key key) = 0;
  };

  Keyboard() = default;

  static Keyboard& Instance() {
    static Keyboard instance;
    return instance;
  }

  void OnEvent(SDL_Event* sdl_event);
  void Update(float dt);

  void RegisterCallback(Callback* callback);

 private:
  struct KeyDown {
    float time_in_down_state{0.0f};
  };

  std::unordered_set<Callback*> callbacks_;
  std::unordered_map<Key, KeyDown> keys_;
};

void Keyboard::OnEvent(SDL_Event* sdl_event) {
  switch (sdl_event->type) {
    case SDL_EVENT_GAMEPAD_ADDED:
      SDL_OpenGamepad(sdl_event->cdevice.which);
      break;

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      break;

    case SDL_EVENT_GAMEPAD_BUTTON_UP:
      break;

    case SDL_EVENT_KEY_DOWN:
      break;

    case SDL_EVENT_KEY_UP:
      break;
  }
}

void Keyboard::Update(float dt) { (void)dt; }

void Keyboard::RegisterCallback(Callback* callback) {
  callbacks_.insert(callback);
}
}  // namespace gameLD58
