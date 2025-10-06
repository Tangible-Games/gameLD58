#pragma once

#include <cmath>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "log.hpp"
#include "sprite_sheet.hpp"

namespace Symphony {
namespace Sprite {

class AnimatedSprite {
 public:
  explicit AnimatedSprite(std::shared_ptr<SpriteSheet> sheet)
      : sheet_(std::move(sheet)) {}

  void Play(const std::string& name, bool looped, float fps = default_fps) {
    if (name == current_name_ && playing_) return;

    current_name_ = name;
    looped_ = looped;
    fps_ = (fps > 0.0f) ? fps : default_fps;

    const auto& indices =
        sheet_ ? sheet_->GetAnimIndices(current_name_) : kEmpty_;
    if (indices.empty()) {
      playing_ = false;
      finished_ = false;
      time_in_anim_ = 0.0f;
      current_frame_idx_ = 0;
      current_global_frame_ = 0;
      LOGE("[Symphony::Sprite::AnimatedSprite] Unknown animation '{}'", name);
      return;
    }

    playing_ = true;
    finished_ = false;
    time_in_anim_ = 0.0f;
    current_frame_idx_ = 0;
    recalcCurrentGlobal_();
  }

  void Stop() {
    if (!playing_) return;

    playing_ = false;
    finished_ = false;
    time_in_anim_ = 0.0f;
    current_frame_idx_ = 0;
    recalcCurrentGlobal_();
  }

  void Update(float dt) {
    if (!playing_ || !sheet_) return;

    const auto& indices = sheet_->GetAnimIndices(current_name_);
    const int frames_count = static_cast<int>(indices.size());
    if (frames_count <= 0 || fps_ <= 0.0f) return;

    time_in_anim_ += dt;
    int new_frame = static_cast<int>(std::floor(time_in_anim_ * fps_));

    if (looped_) {
      new_frame = (frames_count > 0) ? (new_frame % frames_count) : 0;
      finished_ = false;
    } else {
      if (new_frame >= frames_count) {
        new_frame = frames_count - 1;
        finished_ = true;
        playing_ = false;
      }
    }

    if (new_frame != current_frame_idx_) {
      current_frame_idx_ = new_frame;
      recalcCurrentGlobal_();
    }
  }

  void Draw(std::shared_ptr<SDL_Renderer> renderer,
            const SDL_FRect& dst) const {
    if (!sheet_) return;
    if (!playing_) return;

    SDL_Texture* atlas = sheet_->GetAtlas();
    if (!atlas) return;
    if (dst.w <= 0 || dst.h <= 0) return;

    auto oframe = sheet_->GetFrame(current_name_,
                                   static_cast<size_t>(current_frame_idx_));
    if (!oframe.has_value() || !oframe.value()) {
      return;
    }

    const Sprite::SpriteFrame& sf = *oframe.value();
    SDL_FRect src{(float)sf.x, (float)sf.y, (float)sf.w, (float)sf.h};

    SDL_RenderTexture(renderer.get(), atlas, &src, &dst);
  }

  const Sprite::SpriteFrame* CurrentFrame() const noexcept {
    if (!sheet_) return nullptr;

    auto oframe = sheet_->GetFrame(current_name_,
                                   static_cast<size_t>(current_frame_idx_));
    return oframe ? *oframe : nullptr;
  }

  bool IsPlaying() const noexcept { return playing_; }
  bool IsFinished() const noexcept { return finished_; }

 private:
  void recalcCurrentGlobal_() {
    if (!sheet_) {
      current_global_frame_ = 0;
      return;
    }

    const auto& indices = sheet_->GetAnimIndices(current_name_);
    if (indices.empty()) {
      current_global_frame_ = 0;
      return;
    }

    int idx = current_frame_idx_;
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(indices.size()))
      idx = static_cast<int>(indices.size()) - 1;

    current_global_frame_ = static_cast<int>(indices[static_cast<size_t>(idx)]);
  }

 private:
  static constexpr float default_fps = 30.0f;

  std::shared_ptr<SpriteSheet> sheet_;

  std::string current_name_;
  bool looped_ = false;
  bool playing_ = false;
  bool finished_ = false;

  float time_in_anim_ = 0.0f;
  int current_frame_idx_ = 0;
  int current_global_frame_ = 0;
  float fps_ = default_fps;

  static const inline std::vector<size_t> kEmpty_{};
};

}  // namespace Sprite
}  // namespace Symphony
