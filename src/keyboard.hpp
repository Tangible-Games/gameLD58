#pragma once

#include <SDL3/SDL.h>

#include <list>
#include <memory>
#include <optional>
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
    kSelect,
    kShiftLeft,
    kShiftRight,
    kUnknown,
  };

  static std::string KeyToString(Key key) {
    switch (key) {
      case Key::kDpadUp:
        return "DpadUp";
        break;
      case Key::kDpadDown:
        return "DpadDown";
        break;
      case Key::kDpadLeft:
        return "DpadLeft";
        break;
      case Key::kDpadRight:
        return "DpadRight";
        break;
      case Key::kTriangle:
        return "Triangle";
        break;
      case Key::kX:
        return "X";
        break;
      case Key::kSquare:
        return "Square";
        break;
      case Key::kCircle:
        return "Circle";
        break;
      case Key::kStart:
        return "Start";
        break;
      case Key::kSelect:
        return "Select";
        break;
      case Key::kShiftLeft:
        return "ShiftLeft";
        break;
      case Key::kShiftRight:
        return "ShiftRight";
        break;
      case Key::kUnknown:
        return "Unknown";
        break;
    }

    return "Unknown";
  }

  class Callback {
   public:
    virtual void OnKeyDown(Key key) = 0;
    virtual void OnKeyUp(Key key) = 0;
  };

  struct KeyDown {
    float time_in_down_state{0.0f};
  };

  Keyboard() = default;

  static Keyboard& Instance() {
    static Keyboard instance;
    return instance;
  }

  void OnEvent(SDL_Event* sdl_event);
  void Update(float dt);

  std::optional<KeyDown> IsKeyDown(Key key) const;
  std::list<Key> GetDownKeys() const;
  std::string GetDownKeysListString() const;

  void RegisterCallback(Callback* callback);

 private:
  std::unordered_set<Callback*> callbacks_;
  std::unordered_map<Key, KeyDown> keys_;
};

void Keyboard::OnEvent(SDL_Event* sdl_event) {
  bool is_down = false;
  Key key = Key::kUnknown;

  switch (sdl_event->type) {
    case SDL_EVENT_GAMEPAD_ADDED:
      SDL_OpenGamepad(sdl_event->cdevice.which);
      break;

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP: {
      is_down = sdl_event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
      switch (sdl_event->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
          key = Key::kDpadUp;
          break;

        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
          key = Key::kDpadDown;
          break;

        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
          key = Key::kDpadLeft;
          break;

        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
          key = Key::kDpadRight;
          break;

        case 0:
          // Z
          key = Key::kX;
          break;

        case 1:
          // X
          key = Key::kCircle;
          break;

        case 2:
          // A
          key = Key::kSquare;
          break;

        case 3:
          // S
          key = Key::kTriangle;
          break;

        case 4:
          // Enter
          key = Key::kSelect;
          break;

        case 6:
          // Space
          key = Key::kStart;
          break;

        case 9:
          // Q
          key = Key::kShiftLeft;
          break;

        case 10:
          // W
          key = Key::kShiftRight;
          break;
      }
      break;
    }

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      is_down = sdl_event->type == SDL_EVENT_KEY_DOWN;
      switch (sdl_event->key.key) {
        case SDLK_UP:
          key = Key::kDpadUp;
          break;

        case SDLK_DOWN:
          key = Key::kDpadDown;
          break;

        case SDLK_LEFT:
          key = Key::kDpadLeft;
          break;

        case SDLK_RIGHT:
          key = Key::kDpadRight;
          break;

        case SDLK_Z:
          key = Key::kX;
          break;

        case SDLK_X:
          key = Key::kCircle;
          break;

        case SDLK_A:
          key = Key::kSquare;
          break;

        case SDLK_S:
          key = Key::kTriangle;
          ;
          break;

        case SDLK_SPACE:
          key = Key::kStart;
          break;

        case SDLK_RETURN:
          key = Key::kSelect;
          break;

        case SDLK_Q:
          key = Key::kShiftLeft;
          break;

        case SDLK_W:
          key = Key::kShiftRight;
          break;
      }
      break;
    }
  }

  if (key != Key::kUnknown) {
    if (is_down) {
      for (auto* callback : callbacks_) {
        callback->OnKeyDown(key);
      }

      keys_.insert(std::make_pair(key, KeyDown()));
    } else {
      for (auto* callback : callbacks_) {
        callback->OnKeyUp(key);
      }

      keys_.erase(key);
    }
  }
}

void Keyboard::Update(float dt) {
  for (auto& [key, key_down] : keys_) {
    key_down.time_in_down_state += dt;
  }
}

std::optional<Keyboard::KeyDown> Keyboard::IsKeyDown(Key key) const {
  auto it = keys_.find(key);
  if (it == keys_.end()) {
    return std::nullopt;
  }

  return it->second;
}

std::list<Keyboard::Key> Keyboard::GetDownKeys() const {
  std::list<Keyboard::Key> result;
  for (auto& [key, _] : keys_) {
    result.push_back(key);
  }

  return result;
}

std::string Keyboard::GetDownKeysListString() const {
  std::string result;
  result.reserve(128);

  std::list<Keyboard::Key> down_keys = GetDownKeys();
  for (const auto& key : down_keys) {
    std::string key_string = KeyToString(key);
    if (!result.empty()) {
      result += ", ";
    }
    result += key_string;
  }

  return result;
}

void Keyboard::RegisterCallback(Callback* callback) {
  callbacks_.insert(callback);
}
}  // namespace gameLD58
