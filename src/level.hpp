#pragma once

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <string>
#include <symphony_lite/aa_rect2d.hpp>
#include <symphony_lite/animated_sprite.hpp>
#include <symphony_lite/log.hpp>
#include <symphony_lite/sprite_sheet.hpp>
#include <vector>

#include "consts.hpp"
#include "human.hpp"
#include "paralax_renderer.hpp"
#include "ufo.hpp"

namespace gameLD58 {

struct Object {
  std::string layer;
  Symphony::Math::AARect2d rect;
};

struct Config {
  float length;
  float height;
  float ufo_min_height{0.0f};
  float ufo_max_height{0.0f};
  float human_y{0.0f};
  Symphony::Math::Point2d ufo_spawn;
  int humansMin_;
  int humansMax_;
  std::vector<float> humanRespawnX_;
  size_t to_capture{0};
  int time_on_level{0};
  float ufo_velocity_at_end{0.0f};
};

class Level : public Keyboard::Callback {
 public:
  class Callback {
   public:
    virtual void FinishLevel(size_t captured_humans) = 0;
    virtual void TryExitFromLevel() = 0;
  };

  Level(std::shared_ptr<SDL_Renderer> renderer,
        std::shared_ptr<Symphony::Audio::Device> audio, std::string path)
      : renderer_(renderer),
        audio_(audio),
        level_path_(std::move(path)),
        paralax_renderer_(renderer),
        ufo_(renderer, audio) {
    human_sprite_sheet_ = std::make_shared<Symphony::Sprite::SpriteSheet>(
        renderer.get(), "assets", "humanoid.json");
  }

  void Load(
      std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
      const std::string& default_font);
  void Draw();
  void Update(float dt);

  void Start(bool is_paused);

  void SetIsPaused(bool is_paused);

  void OnKeyDown(Keyboard::Key key) override;
  void OnKeyUp(Keyboard::Key key) override;

  void RegisterCallback(Callback* callback_);

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
  std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts_;
  std::string default_font_;
  Symphony::Text::TextRenderer captured_text_;
  Symphony::Text::TextRenderer time_text_;
  std::string level_path_;
  Config level_config_;
  bool is_paused_{false};
  Callback* callback_{nullptr};
  std::vector<Object> objects_;
  std::list<Human> humans_;
  ParallaxRenderer paralax_renderer_;
  std::shared_ptr<Symphony::Sprite::SpriteSheet> human_sprite_sheet_;
  Ufo ufo_;
  float cam_x_;
  float cam_y_;

  size_t capturedHumans_{0};
  float time_left_{0.0f};

  bool is_ending_{false};
  float ending_timeout_{0.0f};

 private:
  static std::string readFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
      return "";
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }

  static inline float shortest_delta(float a, float b, float L) {
    float d = a - b;
    d -= L * std::floor((d + L * 0.5f) / L);
    return d;
  }

  template <typename T>
  void DrawObject(const T& obj, auto DrawToFn);

  void reFormatCapturedText();
  void reFormatTimeText();
};

void Level::Load(
    std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts,
    const std::string& default_font) {
  known_fonts_ = known_fonts;
  default_font_ = default_font;

  const auto text = readFile(level_path_);
  if (text.empty()) {
    LOGE("[gameLD58:Level]: cannot read file '{}'", level_path_);
    return;
  }

  nlohmann::json level_json = nlohmann::json::parse(text, nullptr, false);
  if (level_json.is_discarded()) {
    LOGE("[gameLD58:Level]: cannot parse JSON from file '{}'", level_path_);
    return;
  }

  Config config;
  config.length = level_json.value("length", (float)kScreenWidth);
  config.height = level_json.value("height", (float)kScreenHeight);
  config.ufo_min_height = level_json.value("ufo_min_height", 0.0f);
  config.ufo_max_height =
      level_json.value("ufo_max_height", (float)kScreenHeight);
  config.human_y = level_json.value("human_y", (float)kScreenHeight);

  if (config.length < kScreenWidth) {
    LOGW("level.length {} < screen {}, clamping", config.length, kScreenWidth);
    config.length = (float)kScreenWidth;
  }
  if (config.height < kScreenHeight) {
    LOGW("level.height {} < screen {}, clamping", config.height, kScreenHeight);
    config.height = (float)kScreenHeight;
  }
  config.ufo_spawn.x = level_json.value("ufo_spawn_x", kScreenWidth / 2.f);
  config.ufo_spawn.y = level_json.value("ufo_spawn_y", kScreenHeight / 2.f);

  cam_x_ = config.ufo_spawn.x;
  cam_x_ -= config.length * std::floor(cam_x_ / config.length);
  float min_center = 0.5f * kScreenHeight;
  float max_center = std::max(min_center, config.height - 0.5f * kScreenHeight);
  cam_y_ = std::clamp(config.ufo_spawn.y, min_center, max_center);

  for (const auto& item : level_json["items"]) {
    Object obj;
    float center_x = item.value("center_x", 0);
    float center_y = item.value("center_y", 0);
    float half_width = item.value("half_width", 0);
    float half_height = item.value("half_height", 0);
    Symphony::Math::AARect2d rect{{center_x, center_y},
                                  {half_width, half_height}};
    obj.rect = rect;
    obj.layer = item.value("layer", "default");
    objects_.push_back(obj);
  }

  config.humansMin_ = level_json["humans"]["min"].get<int>();
  config.humansMax_ = level_json["humans"]["max"].get<int>();
  for (const auto& h : level_json["humans"]["respawn_points"]) {
    config.humanRespawnX_.push_back(h.get<float>());
  }

  config.to_capture = level_json["to_capture"].get<size_t>();
  config.time_on_level = level_json["time_on_level"].get<size_t>();
  config.ufo_velocity_at_end = level_json["ufo_velocity_at_end"].get<float>();

  LOGD("Level loaded: length={}, spawn=({}, {}), obstacles={}", config.length,
       config.ufo_spawn.x, config.ufo_spawn.y, objects_.size());

  level_config_ = std::move(config);

  paralax_renderer_.Load(level_config_.length, "assets/backgrounds.json");

  captured_text_.InitRenderer(renderer_);
  captured_text_.LoadFromFile("assets/level_captured.txt");
  captured_text_.SetPosition(10, 10);
  captured_text_.SetSizes(kScreenWidth - 20, kScreenHeight - 10);

  time_text_.InitRenderer(renderer_);
  time_text_.LoadFromFile("assets/level_time.txt");
  time_text_.SetPosition(10, 10);
  time_text_.SetSizes(kScreenWidth - 20, kScreenHeight - 10);

  ufo_.Load();
}

template <typename T>
void Level::DrawObject(const T& obj, auto DrawToFn) {
  const auto& b = obj.rect;
  float cx = b.center.x;
  float cy = b.center.y;
  float hx = b.half_size.x;
  float hy = b.half_size.y;

  // TODO: rework it. Before it was outside the loop
  float l = level_config_.length;
  float cam_left = cam_x_ - (kScreenWidth * 0.5f);
  float cam_top = cam_y_ - (kScreenHeight * 0.5f);
  bool crosses_left = cam_left < 0.0f;
  bool crosses_right = cam_left + kScreenWidth > l;

  bool need_wrap = l > kScreenWidth;

  auto draw_one = [&](float draw_cx) {
    float x = (draw_cx - hx) - cam_left;
    float y = (cy - hy) - cam_top;
    float rw = 2.f * hx;
    float rh = 2.f * hy;

    if (x + rw <= 0.f || x >= kScreenWidth) return;
    if (y + rh <= 0.f || y >= kScreenHeight) return;

    SDL_FRect r{x, y, rw, rh};
    DrawToFn(r);
  };

  draw_one(cx);
  if (need_wrap && crosses_left) {
    draw_one(cx - l);
  }
  if (need_wrap && crosses_right) {
    draw_one(cx + l);
  }
}

void Level::reFormatCapturedText() {
  captured_text_.ReFormat(
      {{"captured", std::to_string(capturedHumans_)},
       {"to_capture", std::to_string(level_config_.to_capture)}},
      default_font_, known_fonts_);
}

void Level::reFormatTimeText() {
  time_text_.ReFormat({{"time", std::to_string((int)time_left_)}},
                      default_font_, known_fonts_);
}

void Level::Draw() {
  paralax_renderer_.Draw(cam_x_, cam_y_);

  SDL_SetRenderDrawColor(renderer_.get(), 0, 255, 0, 255);

  // for (const auto& obj : objects_) {
  //   DrawObject(obj, [&](SDL_FRect r) { SDL_RenderRect(renderer_.get(), &r);
  //   });
  // };

  for (auto& obj : humans_) {
    DrawObject(obj, [&](SDL_FRect r) { obj.DrawTo(r); });
  };

  auto ub = ufo_.GetBounds();
  auto dx = shortest_delta(ub.center.x, cam_x_, level_config_.length);

  float x = (kScreenWidth * 0.5f) + dx - ub.half_size.x;
  float y = (kScreenHeight * 0.5f) + (ub.center.y - cam_y_) - ub.half_size.y;
  SDL_FRect dst{x, y, 2.f * ub.half_size.x, 2.f * ub.half_size.y};

  if (dst.x + dst.w > 0.f && dst.x < kScreenWidth && dst.y + dst.h > 0.f &&
      dst.y < kScreenHeight) {
    ufo_.DrawTo(dst);
  }

  captured_text_.Render(0);

  reFormatTimeText();
  time_text_.Render(0);
}

void Level::Update(float dt) {
  if (is_paused_) {
    return;
  }

  if (is_ending_) {
    auto ufo_center =
        ufo_.GetBounds().center +
        Symphony::Math::Vector2d(0.0f, level_config_.ufo_velocity_at_end) * dt;
    ufo_.SetPosition(ufo_center);

    if (ending_timeout_ > 0.0f) {
      ending_timeout_ -= dt;
      if (ending_timeout_ < 0.0f) {
        ending_timeout_ = 0.0;

        if (callback_) {
          callback_->FinishLevel(capturedHumans_);
        }
      }
    }

    return;
  }

  time_left_ -= dt;
  if (time_left_ < 0.0f) {
    time_left_ = 0.0f;

    is_ending_ = true;
    ending_timeout_ = 1.0f;
    ufo_.FinishLevel();

    LOGD("Ending level, time's up.");
  }

  // Respawn humans if needed
  if ((int)humans_.size() < level_config_.humansMin_) {
    int max = level_config_.humansMax_ - (int)humans_.size();
    int min = level_config_.humansMin_ - (int)humans_.size();
    int randNum = min + ((1ULL * (max - min) * std::rand()) / RAND_MAX);
    LOGD("Repawn {} humans", randNum);
    for (int i = 0; i < randNum; i++) {
      int randPointNum =
          ((1ULL * level_config_.humanRespawnX_.size() * std::rand()) /
           RAND_MAX);
      humans_.emplace_back(
          renderer_, level_config_.humanRespawnX_[randPointNum],
          level_config_.height, level_config_.length, human_sprite_sheet_);
      LOGD("Create a human at {}x{}",
           level_config_.humanRespawnX_[randPointNum], level_config_.human_y);
    }
  }

  ufo_.Update(dt);
  // Add collisions here:
  auto ufo_center = ufo_.GetBounds().center + ufo_.GetVelocity() * dt;
  if (ufo_center.x > level_config_.length) {
    ufo_center.x -= level_config_.length;
  }
  if (ufo_center.x < 0.f) {
    ufo_center.x = level_config_.length - ufo_center.x;
  }

  if (ufo_center.y < level_config_.ufo_min_height) {
    ufo_center.y = level_config_.ufo_min_height;
    if (ufo_.GetVelocity().y < 0.0f) {
      auto v = ufo_.GetVelocity();
      ufo_.SetVelocity({v.x, 0.0f});

      auto a = ufo_.GetAcceleration();
      ufo_.SetAcceleration({a.x, 0.0f});
    }
  } else if (ufo_center.y > level_config_.ufo_max_height) {
    ufo_center.y = level_config_.ufo_max_height;
    if (ufo_.GetVelocity().y > 0.0f) {
      auto v = ufo_.GetVelocity();
      ufo_.SetVelocity({v.x, 0.0f});

      auto a = ufo_.GetAcceleration();
      ufo_.SetAcceleration({a.x, 0.0f});
    }
  }
  ufo_.SetPosition(ufo_center);

  float dead_left = 100.f;
  float dead_right = 100.f;

  float dx = ufo_.GetBounds().center.x - cam_x_;
  dx -= level_config_.length *
        std::floor((dx + level_config_.length * 0.5f) / level_config_.length);

  float ufo_screen_x = kScreenWidth * 0.5f + dx;

  if (ufo_screen_x > kScreenWidth - dead_right) {
    cam_x_ += ufo_screen_x - (kScreenWidth - dead_right);
  } else if (ufo_screen_x < dead_left) {
    cam_x_ -= (dead_left - ufo_screen_x);
  }

  cam_x_ -= level_config_.length * std::floor(cam_x_ / level_config_.length);

  float min_center = 0.5f * kScreenHeight;
  float max_center =
      std::max(min_center, level_config_.height - 0.5f * kScreenHeight);
  if (level_config_.height <= kScreenHeight) {
    cam_y_ = min_center;
  } else {
    cam_y_ = std::clamp(ufo_.GetBounds().center.y, min_center, max_center);
  }

  // Get visible humans
  for (auto h = humans_.begin(); h != humans_.end();) {
    bool collected = ufo_.MaybeCatchHuman(*h);
    if (collected) {
      h = humans_.erase(h);
      capturedHumans_++;
      reFormatCapturedText();
    } else {
      h++;
    }
  }

  for (auto h = humans_.begin(); h != humans_.end();) {
    if (!h->Update(dt)) {
      LOGD("Dead");
      h = humans_.erase(h);
    } else {
      h++;
    }
  }

  if (capturedHumans_ >= level_config_.to_capture) {
    is_ending_ = true;
    ending_timeout_ = 1.0f;
    ufo_.FinishLevel();

    LOGD("Ending level, captured enough.");
  }
}

void Level::Start(bool is_paused) {
  is_paused_ = is_paused;

  ufo_.SetPosition(level_config_.ufo_spawn);
  ufo_.SetVelocity(Symphony::Math::Vector2d());
  ufo_.SetAcceleration(Symphony::Math::Vector2d());

  is_ending_ = false;
  ending_timeout_ = 0.0f;

  capturedHumans_ = 0;
  time_left_ = (float)level_config_.time_on_level + 0.5f;
  reFormatCapturedText();
}

void Level::SetIsPaused(bool is_paused) { is_paused_ = is_paused; }

void Level::OnKeyDown(Keyboard::Key /*key*/) {}

void Level::OnKeyUp(Keyboard::Key key) {
  if (key == Keyboard::Key::kSelect) {
    if (callback_) {
      callback_->TryExitFromLevel();
    }
  }
}

void Level::RegisterCallback(Callback* callback) { callback_ = callback; }

}  // namespace gameLD58
