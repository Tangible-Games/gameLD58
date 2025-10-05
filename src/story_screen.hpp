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
  size_t cur_story_bro_{0};
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

  file.open("assets/story_screen.json");
  if (!file.is_open()) {
    LOGE("Failed to load {}", "assets/story_screen.json");
    return;
  }

  nlohmann::json story_screen_json = nlohmann::json::parse(file);
  file.close();

  stories_.resize(story_screen_json["stories"].size());
  for (size_t index = 0;
       const auto& story_json : story_screen_json["stories"]) {
    stories_[index].text_renderer.InitRenderer(renderer_);
    stories_[index].text_renderer.LoadFromFile(story_json["file_path"]);
    stories_[index].text_renderer.SetPosition(story_json.value("x", 0),
                                              story_json.value("y", 0));
    stories_[index].text_renderer.SetSizes(story_json.value("width", 0),
                                           story_json.value("height", 0));
    stories_[index].text_renderer.ReFormat({}, default_font_, known_fonts_);

    ++index;
  }
}

void StoryScreen::Update(float /*dt*/) {}
void StoryScreen::Draw() {
  SDL_FRect screen_rect = {0, 0, kScreenWidth, kScreenHeight};
  SDL_FColor color;
  color.a = 1.0f;
  color.r = 1.0f;
  color.g = 1.0f;
  color.b = 1.0f;
  SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
  SDL_SetTextureBlendMode(image_.get(), SDL_BLENDMODE_BLEND);
  RenderTexture(renderer_, image_, &screen_rect, &screen_rect, &color);

  if (cur_story_bro_ < stories_.size()) {
    stories_[cur_story_bro_].text_renderer.Render(0);
  }
}

void StoryScreen::OnKeyDown(Keyboard::Key /*key*/) {}

void StoryScreen::OnKeyUp(Keyboard::Key key) {
  if (key == Keyboard::Key::kX) {
    if (cur_story_bro_ + 1 == stories_.size()) {
      if (callback_) {
        callback_->ContinueFromStoryScreen();
      }
    } else {
      ++cur_story_bro_;
      if (cur_story_bro_ + 1 == stories_.size()) {
        cur_story_bro_ = stories_.size() - 1;
      }
    }
  } else if (key == Keyboard::Key::kCircle) {
    if (cur_story_bro_ > 0) {
      --cur_story_bro_;
    }
  } else if (key == Keyboard::Key::kSelect) {
    if (callback_) {
      callback_->TryExitFromStoryScreen();
    }
  }
}

void StoryScreen::RegisterCallback(Callback* callback) { callback_ = callback; }
}  // namespace gameLD58
