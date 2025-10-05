#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "consts.hpp"
#include "draw_texture.hpp"
#include "fade_image.hpp"
#include "keyboard.hpp"

namespace gameLD58 {
class StoryScreen : public Keyboard::Callback {
 public:
  StoryScreen(std::shared_ptr<SDL_Renderer> renderer,
              std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer), audio_(audio) {}

  void Load();

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  std::shared_ptr<SDL_Texture> image_;
};

void StoryScreen::Load() {}

void StoryScreen::OnKeyDown(Keyboard::Key /*key*/) {}

void StoryScreen::OnKeyUp(Keyboard::Key /*key*/) {}
}  // namespace gameLD58
