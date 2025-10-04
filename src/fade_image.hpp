#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <symphony_lite/all_symphony.hpp>

#include "consts.hpp"
#include "draw_texture.hpp"

namespace gameLD58 {
class FadeImage {
 public:
  FadeImage(std::shared_ptr<SDL_Renderer> renderer,
            std::shared_ptr<Symphony::Audio::Device> audio,
            const std::string& static_image_path)
      : renderer_(renderer), audio_(audio) {
    if (!static_image_path.empty()) {
      image_.reset(IMG_LoadTexture(renderer.get(), static_image_path.c_str()),
                   &SDL_DestroyTexture);
    }
  }

  void Load(const std::string& static_image_path);

  void StartFadeOut(float timeout);
  void StartFadeIn(float timeout);
  void MakeSolid();
  bool IsIdle() const { return state_ == State::kIdle; }

  void Update(float dt);
  void Draw();

 private:
  enum class State { kIdle, kFadeOut, kFadeIn };

  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  std::shared_ptr<SDL_Texture> image_;
  State state_;
  float timeout_{0.0f};
  float running_time_{0.0f};
  float cur_alpha_{1.0f};
};

void FadeImage::Load(const std::string& static_image_path) {
  image_.reset(IMG_LoadTexture(renderer_.get(), static_image_path.c_str()),
               &SDL_DestroyTexture);
}

void FadeImage::StartFadeOut(float timeout) {
  state_ = State::kFadeOut;
  timeout_ = timeout;
  running_time_ = 0.0f;
}

void FadeImage::StartFadeIn(float timeout) {
  state_ = State::kFadeIn;
  timeout_ = timeout;
  running_time_ = 0.0f;
}

void FadeImage::MakeSolid() {
  state_ = State::kIdle;
  timeout_ = 0.0f;
  running_time_ = 0.0f;
  cur_alpha_ = 1.0f;
}

void FadeImage::Update(float dt) {
  switch (state_) {
    case State::kIdle:
      break;

    case State::kFadeOut:
      running_time_ += dt;
      if (running_time_ > timeout_) {
        running_time_ = timeout_;
        state_ = State::kIdle;
      }
      cur_alpha_ = (timeout_ - running_time_) / timeout_;
      break;

    case State::kFadeIn:
      running_time_ += dt;
      if (running_time_ > timeout_) {
        running_time_ = timeout_;
        state_ = State::kIdle;
      }
      cur_alpha_ = running_time_ / timeout_;
      break;
  }
}

void FadeImage::Draw() {
  SDL_FRect screen_rect = {0, 0, kScreenWidth, kScreenHeight};
  SDL_FColor color;
  color.a = cur_alpha_;
  color.r = 1.0f;
  color.g = 1.0f;
  color.b = 1.0f;
  SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
  SDL_SetTextureBlendMode(image_.get(), SDL_BLENDMODE_BLEND);
  RenderTexture(renderer_, image_, &screen_rect, &screen_rect, &color);
}
}  // namespace gameLD58
