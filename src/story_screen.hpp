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
  class Callback {
   public:
    virtual void ContinueFromStoryScreen() = 0;
    virtual void TryExitFromStoryScreen() = 0;
  };

  StoryScreen(std::shared_ptr<SDL_Renderer> renderer,
              std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer), audio_(audio) {}

  void Load(
      std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
      const std::string& default_font);

  void Update(float dt);
  void Draw();

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

  void RegisterCallback(Callback* callback);

 private:
  struct Story {
    Symphony::Text::TextRenderer text_renderer;
  };

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts_;
  std::string default_font_;
  std::shared_ptr<SDL_Texture> image_;
  std::vector<Story> stories_;
  Callback* callback_{nullptr};
};

void StoryScreen::Load(
    std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
    const std::string& default_font) {
  known_fonts_ = known_fonts;
  default_font_ = default_font;

  image_.reset(IMG_LoadTexture(renderer_.get(), "assets/story_screen.png"),
               &SDL_DestroyTexture);

  std::ifstream file;

  file.open("assets/known_fonts.json");
  if (!file.is_open()) {
    LOGE("Failed to load {}", "assets/known_fonts.json");
    return;
  }

  nlohmann::json story_screen_json = nlohmann::json::parse(file);
  file.close();

  stories_.resize(story_screen_json["stories"].size());
  for (size_t index = 0;
       const auto& story_json : story_screen_json["stories"]) {
    stories_[index].text_renderer.InitRenderer(renderer_);
    stories_[index].text_renderer.LoadFromFile(story_json["file_path"]);
    // stories_[index].text_renderer.SetPosition(story_json["x"].get<int>(),
    //                                           story_json["y"].get<int>());
    // stories_[index].text_renderer.SetSizes(story_json["width"].get<int>(),
    //                                        story_json["height"].get<int>());
    stories_[index].text_renderer.ReFormat({}, default_font_, known_fonts_);

    ++index;
  }
}

void StoryScreen::Update(float /*dt*/) {}
void StoryScreen::Draw() {}

void StoryScreen::OnKeyDown(Keyboard::Key /*key*/) {}

void StoryScreen::OnKeyUp(Keyboard::Key key) {
  if (key == Keyboard::Key::kX) {
    if (callback_) {
      callback_->ContinueFromStoryScreen();
    }
  } else if (key == Keyboard::Key::kSelect) {
    if (callback_) {
      callback_->TryExitFromStoryScreen();
    }
  }
}

void StoryScreen::RegisterCallback(Callback* callback) { callback_ = callback; }
}  // namespace gameLD58
