#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <symphony_lite/all_symphony.hpp>

#include "consts.hpp"
#include "human.hpp"
#include "keyboard.hpp"
#include "symphony_lite/angle.hpp"
#include "symphony_lite/point2d.hpp"
#include "utils.hpp"

namespace gameLD58 {
class Ufo {
  static constexpr auto kUfoConfigPath{"assets/ufo.json"};
  static constexpr bool kDebugDump = true;

 public:
  Ufo(std::shared_ptr<SDL_Renderer> renderer,
      std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer), audio_(audio) {}
  virtual ~Ufo() = default;

  void Load();
  void DrawTo(const SDL_FRect& dst);

  void Update(float dt);

  void FinishLevel();

  const Symphony::Math::Vector2d& GetVelocity() const { return velocity_; }

  void SetVelocity(const Symphony::Math::Vector2d& new_velocity) {
    velocity_ = new_velocity;
  }

  const Symphony::Math::Vector2d& GetAcceleration() const {
    return acceleration_;
  }

  void SetAcceleration(const Symphony::Math::Vector2d& new_acceleration) {
    acceleration_ = new_acceleration;
  }

  const Symphony::Math::AARect2d& GetBounds() const { return rect_; }

  void SetPosition(const Symphony::Math::Point2d& new_position) {
    rect_.center = new_position;
  }

  bool MaybeCatchHuman(Human& h);

  bool IsPointInBeam(const Symphony::Math::Point2d& p);

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;

  std::shared_ptr<SDL_Texture> texture_{};

  struct Configuration {
    Symphony::Math::Vector2d maxVelocity{0, 0};
    Symphony::Math::Vector2d moveAcceleration{0, 0};
    Symphony::Math::Vector2d dragCoef{0, 0};
    Symphony::Math::Vector2d driftAcceleration{0, 0};
    Symphony::Math::Vector2d driftThreshold{0, 0};
    struct TractorBeam {
      float latency{0};
      float initialWidth{0};
      float angularWidth{0};
    } tractorBeam;
  } configuration_;

  Symphony::Math::AARect2d rect_{};
  Symphony::Math::Vector2d velocity_{0, 0};
  Symphony::Math::Vector2d acceleration_{0, 0};

  float tractorBeamTimeout_{0};

  float prevTime_{0};
};

void Ufo::Load() {
  // Load UFO configuration from json
  std::ifstream file;

  file.open(kUfoConfigPath);
  if (!file.is_open()) {
    LOGE("Failed to load {}", kUfoConfigPath);
    return;
  }

  // Parse JSON and extract values
  nlohmann::json config = nlohmann::json::parse(file);
  file.close();

  // Parse configuration
  rect_ = Symphony::Math::AARect2d{
      Symphony::Math::Point2d{kScreenWidth / 2.0, kScreenHeight / 2.0},
      Symphony::Math::Vector2d{config["size"]["width"].get<float>() / 2.0f,
                               config["size"]["height"].get<float>() / 2.0f}};

  configuration_.moveAcceleration =
      Symphony::Math::Vector2d{config["acceleration"]["x"].get<float>(),
                               config["acceleration"]["y"].get<float>()};

  configuration_.maxVelocity =
      Symphony::Math::Vector2d{config["max_velocity"]["x"].get<float>(),
                               config["max_velocity"]["y"].get<float>()};

  configuration_.dragCoef =
      Symphony::Math::Vector2d{config["drag_sec"]["x"].get<float>(),
                               config["drag_sec"]["y"].get<float>()};

  configuration_.driftAcceleration = Symphony::Math::Vector2d{
      config["drift"]["acceleration"]["x"].get<float>(),
      config["drift"]["acceleration"]["y"].get<float>()};

  configuration_.driftThreshold = Symphony::Math::Vector2d{
      config["drift"]["acceleration_threshold"]["x"].get<float>(),
      config["drift"]["acceleration_threshold"]["y"].get<float>()};

  configuration_.tractorBeam.latency =
      config["tractor_beam"]["latency"].get<float>();
  configuration_.tractorBeam.initialWidth =
      config["tractor_beam"]["initial_width"].get<float>();
  configuration_.tractorBeam.angularWidth = Symphony::Math::DegToRad(
      config["tractor_beam"]["angular_width"].get<float>());

  const std::string texturePath = config["texture"].get<std::string>();
  texture_ = std::shared_ptr<SDL_Texture>(
      IMG_LoadTexture(renderer_.get(), texturePath.c_str()),
      &SDL_DestroyTexture);

  LOGD(
      "Config:"
      "\n\t"
      "rect: {},"
      "\n\t"
      "moveAcceleration: {},"
      "\n\t"
      "maxVelocity: {},"
      "\n\t"
      "dragCoef: {}"
      "\n\t"
      "driftAcceleration: {}"
      "\n\t"
      "driftThreshold: {}",
      rect_, configuration_.moveAcceleration, configuration_.maxVelocity,
      configuration_.dragCoef, configuration_.driftAcceleration,
      configuration_.driftThreshold);
}

void Ufo::Update(float dt) {
  // Handle keys
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadLeft).has_value()) {
    acceleration_.x -= configuration_.moveAcceleration.x * dt;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadRight).has_value()) {
    acceleration_.x += configuration_.moveAcceleration.x * dt;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadUp).has_value()) {
    acceleration_.y -= configuration_.moveAcceleration.y * dt;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadDown).has_value()) {
    acceleration_.y += configuration_.moveAcceleration.y * dt;
  }
  if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kSquare).has_value()) {
    tractorBeamTimeout_ = configuration_.tractorBeam.latency;
  }

  // Calculate velocity
  velocity_ = acceleration_ * dt;

  // Limit max speed
  velocity_.x = std::clamp<float>(velocity_.x, -configuration_.maxVelocity.x,
                                  configuration_.maxVelocity.x);
  velocity_.y = std::clamp<float>(velocity_.y, -configuration_.maxVelocity.y,
                                  configuration_.maxVelocity.y);

  // Add some drag, acceleration is decreasing
  acceleration_.x *= std::pow(configuration_.dragCoef.x, dt);
  acceleration_.y *= std::pow(configuration_.dragCoef.y, dt);

  // Add drift, on small acceleration add random
  if (std::abs(acceleration_.x) < configuration_.driftThreshold.x &&
      std::abs(acceleration_.y) < configuration_.driftThreshold.y) {
    // Add random drift when UFO is stopped
    float rx = randMinusOneToOne() * configuration_.driftAcceleration.x;
    float ry = randMinusOneToOne() * configuration_.driftAcceleration.y;
    acceleration_.x += rx * dt;
    acceleration_.y += ry * dt;
  }

  if (kDebugDump) {
    // Dump acceleration once a second for debug
    auto newTime = prevTime_ + dt;
    if (std::floor(newTime) > std::floor(prevTime_)) {
      LOGD("acc: {}, velocity: {}, pos: {}", acceleration_, velocity_, rect_);
    }
    prevTime_ = newTime;
  }

  tractorBeamTimeout_ -= dt;
  tractorBeamTimeout_ =
      std::clamp(tractorBeamTimeout_, 0.0f, configuration_.tractorBeam.latency);
}

void Ufo::FinishLevel() { tractorBeamTimeout_ = 0.0f; }

static SDL_FColor SdlColorFromUInt32(uint32_t color) {
  SDL_FColor sdl_color;
  sdl_color.a = (float)((color >> 24) & 0xFF) / 255.0f;
  sdl_color.r = (float)((color >> 16) & 0xFF) / 255.0f;
  sdl_color.g = (float)((color >> 8) & 0xFF) / 255.0f;
  sdl_color.b = (float)(color & 0xFF) / 255.0f;
  return sdl_color;
}

void Ufo::DrawTo(const SDL_FRect& dst) {
  SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, &dst);

  // TODO: DrawTo might be not convenient for cases like this. Factor it out.
  if (tractorBeamTimeout_ > 0.0) {
    float x = dst.x + rect_.half_size.x;
    float y = dst.y + (rect_.half_size.y * 2);

    float beamTopHalfWidth = configuration_.tractorBeam.initialWidth / 2;
    float beamBottomHalfWidth =
        beamTopHalfWidth +
        ((kScreenHeight - y) *
         std::tan(configuration_.tractorBeam.angularWidth / 2.0));

    const std::array<SDL_Vertex, 4> vert = {
        SDL_Vertex{
            {x + beamTopHalfWidth, y}, SdlColorFromUInt32(0x887DE974), {}},
        SDL_Vertex{
            {x - beamTopHalfWidth, y}, SdlColorFromUInt32(0x887DE974), {}},
        SDL_Vertex{{x - beamBottomHalfWidth, kScreenHeight},
                   SdlColorFromUInt32(0x887DE974),
                   {}},
        SDL_Vertex{{x + beamBottomHalfWidth, kScreenHeight},
                   SdlColorFromUInt32(0x887DE974),
                   {}},
    };

    const std::array<int, 6> indices = {0, 1, 2, 2, 3, 0};

    SDL_RenderGeometry(renderer_.get(), nullptr, vert.data(), vert.size(),
                       indices.data(), indices.size());
  }
}

bool Ufo::IsPointInBeam(const Symphony::Math::Point2d& p) {
  float beamTopHalfWidth = configuration_.tractorBeam.initialWidth / 2;
  float beamOnPointWidth =
      beamTopHalfWidth +
      ((p.y - rect_.center.y) *
       std::tan(configuration_.tractorBeam.angularWidth / 2.0));
  return rect_.center.x - beamOnPointWidth <= p.x &&
         p.x <= rect_.center.x + beamOnPointWidth;
}

bool Ufo::MaybeCatchHuman(Human& h) {
  (void)h;
  if (tractorBeamTimeout_ > 0.0 && IsPointInBeam(h.rect.center)) {
    h.captured_ = true;

    if (h.rect.center.y < (rect_.center.y + rect_.half_size.y)) {
      LOGD("Capture human");
      return true;
    }
  } else {
    h.captured_ = false;
  }

  return false;
}

}  // namespace gameLD58
