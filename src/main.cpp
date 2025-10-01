#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <spine/spine.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <symphony_lite/all_symphony.hpp>
#include <thread>
#include <vector>

#include "keyboard.hpp"
#include "spine_sdl3.h"

using namespace Symphony::Math;
using namespace Symphony::Collision;
using namespace gameLD58;

extern spine::SkeletonRenderer* skeletonRenderer;

namespace {

constexpr bool kShowSystemCounters = true;

constexpr float kGravity = -980.0f;
constexpr float kCharacter_velocity = 150.0f;
constexpr float kCharacter_jump_velocity = 400.0f;

struct Sprite {
  Point2d pos;
  Point2d cur_pos;
  Vector2d half_sizes;
  Vector2d dir;
  float velocity{};
  float max_travel_dist{};
  bool collides{false};
};

bool characterTouchesFloor(const Vector2d& next_pos, const Vector2d& half_sizes,
                           float floor_y, Vector2d& new_pos_out) {
  new_pos_out = next_pos;

  float character_down = next_pos.y + half_sizes.y;
  if (character_down >= floor_y) {
    new_pos_out.y = floor_y - half_sizes.y;
    return true;
  }

  return false;
}

}  // namespace

int main(int /* argc */, char* /* argv */[]) {
  std::vector<Sprite> all_sprites;

  Vector2d character_pos;
  Vector2d character_half_sizes;
  Vector2d character_cur_velocity;

  float music_timeout = 0.0f;

  Symphony::Log::Logger::init()
      .set_verbosity(Symphony::Log::Logger::Verbosity::DEBUG)
      .add_sink(Symphony::Log::FileSink::create(LOG_FILE));

  LOGI("Game starting...");

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);

  LOGI("SDL initialized.");

  SDL_Window* window = SDL_CreateWindow("window", 480, 272, 0);
  std::shared_ptr<SDL_Renderer> renderer{SDL_CreateRenderer(window, nullptr),
                                         &SDL_DestroyRenderer};
  if (!renderer) {
    LOGI("Creating renderer failed, error: {}", SDL_GetError());
  }
  LOGI("Window is created.");

  srand(time(nullptr));

  LOGI("Loading resources...");

  // Load sprites
  SDL_Texture* sprite_black =
      IMG_LoadTexture(renderer.get(), "assets/dummy_50x50.png");
  if (!sprite_black) {
    LOGI("Loading 'assets/dummy_50x50.png' failed, error: {}", SDL_GetError());
  }

  LOGI("Path: {}", SDL_GetBasePath());

  LOGI("Loading resources 1...");

  SDL_Texture* sprite_red =
      IMG_LoadTexture(renderer.get(), "assets/dummy_50x50.png");
  if (!sprite_red) {
    LOGI("Loading 'assets/dummy_50x50.png' failed.");
  }

  LOGI("Loading resources 2...");

  SDL_Texture* sprite_character =
      IMG_LoadTexture(renderer.get(), "assets/character.png");
  if (!sprite_character) {
    LOGI("Loading 'assets/character.png' failed.");
  }
  character_half_sizes.x = sprite_character->w / 2.0f;
  character_half_sizes.y = sprite_character->h / 2.0f;

  LOGI("Loading resources 3...");

  character_pos = Vector2d(480.0f / 2, 272.0f / 2);

  auto audio_device = std::make_unique<Symphony::Audio::Device>();
  auto music = Symphony::Audio::LoadWave(
      "assets/chrispy_mint_22k.wav",
      Symphony::Audio::WaveFile::kModeStreamingFromFile);
  auto jump = Symphony::Audio::LoadWave(
      "assets/dummy_22k.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  audio_device->Init();

  std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts;
  if (kShowSystemCounters) {
    auto system_font_20 = Symphony::Text::LoadBmFont("assets/system_20.fnt");
    system_font_20->LoadTexture(renderer);
    auto system_font_30 = Symphony::Text::LoadBmFont("assets/system_30.fnt");
    system_font_30->LoadTexture(renderer);
    auto system_font_50 = Symphony::Text::LoadBmFont("assets/system_50.fnt");
    system_font_50->LoadTexture(renderer);
    known_fonts.insert(std::make_pair("system_20.fnt", system_font_20));
    known_fonts.insert(std::make_pair("system_30.fnt", system_font_30));
    known_fonts.insert(std::make_pair("system_50.fnt", system_font_50));
  }
  auto multi_paragraph =
      Symphony::Text::LoadBmFont("assets/multi_paragraph.fnt");
  multi_paragraph->LoadTexture(renderer);
  auto multi_paragraph_25 =
      Symphony::Text::LoadBmFont("assets/multi_paragraph_25.fnt");
  multi_paragraph_25->LoadTexture(renderer);
  known_fonts.insert(std::make_pair("multi_paragraph.fnt", multi_paragraph));
  known_fonts.insert(
      std::make_pair("multi_paragraph_25.fnt", multi_paragraph_25));

  Symphony::Text::TextRenderer system_info_renderer(renderer);
  if (kShowSystemCounters) {
    system_info_renderer.LoadFromFile("assets/system_counters.txt");
    system_info_renderer.SetPosition(5, 5);
    system_info_renderer.SetSizes(480 - 10, 272);
  }

  Symphony::Text::TextRenderer multi_paragraph_demo_renderer(renderer);
  multi_paragraph_demo_renderer.LoadFromFile("assets/multi_paragraph.txt");
  multi_paragraph_demo_renderer.SetPosition(100, 5);
  multi_paragraph_demo_renderer.SetSizes(480 - 110, 272 - 10);
  multi_paragraph_demo_renderer.ReFormat({}, "multi_paragraph.fnt",
                                         known_fonts);

  music_timeout = (float)(rand() % 3) + 3;
  std::shared_ptr<Symphony::Audio::PlayingStream> music_stream;

  for (int i = 0; i < 100; ++i) {
    Sprite sprite;
    sprite.pos = Point2d(rand() % 480, rand() % 272);
    sprite.cur_pos = sprite.pos;
    sprite.half_sizes = Vector2d((rand() % 10) + 5, (rand() % 20) + 5);
    float angle = DegToRad(rand() % 180);
    sprite.dir = Vector2d(cosf(angle), sinf(angle));
    sprite.dir.MakeNormalized();
    sprite.velocity = rand() % 10 + 20;
    sprite.max_travel_dist = rand() % 50 + 20;
    all_sprites.emplace_back(sprite);
  }

  // Spine example from spine sdl to test port on PSP
  spine::SDLTextureLoader textureLoader(renderer.get());
  spine::Atlas atlas("assets/spineboy-pma.atlas", &textureLoader);
  spine::AtlasAttachmentLoader attachmentLoader(&atlas);
  spine::SkeletonJson json(&attachmentLoader);
  json.setScale(0.2f);
  spine::SkeletonData* skeletonData =
      json.readSkeletonDataFile("assets/spineboy-pro.json");
  spine::SkeletonDrawable drawable(skeletonData);
  drawable.usePremultipliedAlpha = true;
  drawable.animationState->getData()->setDefaultMix(0.2f);
  drawable.skeleton->setPosition(200, 200);
  drawable.skeleton->setToSetupPose();
  drawable.animationState->setAnimation(0, "portal", true);
  drawable.animationState->addAnimation(0, "run", true, 0);
  drawable.update(0, spine::Physics_Update);

  bool running = true;

  LOGI("Starting main loop...");

  float fps = 0.0f;
  auto prev_frame_start_time{std::chrono::steady_clock::now()};

  float multi_paragraph_demo_scroll_y = 0.0f;
  float multi_paragraph_demo_no_scroll_timeout = 3.0f;

  std::shared_ptr<Symphony::Audio::PlayingStream> jump_stream;
  uint64_t lastFrameTime = SDL_GetPerformanceCounter();
  while (running) {
    auto frame_start_time{std::chrono::steady_clock::now()};
    std::chrono::duration<float> dt_period_seconds{frame_start_time -
                                                   prev_frame_start_time};
    float dt = dt_period_seconds.count();
    prev_frame_start_time = frame_start_time;

    bool is_music_playing = audio_device->IsPlaying(music_stream);
    if (!is_music_playing) {
      music_timeout -= dt;
      if (music_timeout < 0.0f) {
        music_timeout = 0.0f;
        music_stream =
            audio_device->Play(music, Symphony::Audio::PlayTimes(2),
                               Symphony::Audio::FadeInOut(5.0f, 5.0f));
        music_timeout = (float)(rand() % 5) + 5;
      }
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT:
          running = false;
          break;
      }

      Keyboard::Instance().OnEvent(&event);
    }

    Keyboard::Instance().Update(dt);

    Vector2d character_new_pos;
    bool character_touches_floor = characterTouchesFloor(
        character_pos, character_half_sizes, 272.0f, character_new_pos);

    if (character_touches_floor) {
      character_cur_velocity.x = 0.0f;
      if (Keyboard::Instance()
              .IsKeyDown(Keyboard::Key::kDpadLeft)
              .has_value()) {
        character_cur_velocity.x = -kCharacter_velocity;
      }
      if (Keyboard::Instance()
              .IsKeyDown(Keyboard::Key::kDpadRight)
              .has_value()) {
        character_cur_velocity.x = kCharacter_velocity;
      }
    }
    if (Keyboard::Instance().IsKeyDown(Keyboard::Key::kDpadUp).has_value() ||
        Keyboard::Instance().IsKeyDown(Keyboard::Key::kX).has_value()) {
      Vector2d new_pos;
      if (characterTouchesFloor(character_pos, character_half_sizes, 272.0f,
                                new_pos)) {
        jump_stream = audio_device->Play(jump, Symphony::Audio::PlayTimes(1));
        character_cur_velocity.y = -kCharacter_jump_velocity;

        bool is_music_playing = audio_device->IsPlaying(music_stream);
        if (is_music_playing) {
          audio_device->Stop(music_stream, Symphony::Audio::StopFade(1.0f));
        }
      }
    }

    character_cur_velocity.y -= kGravity * dt;
    character_pos = character_pos + character_cur_velocity * dt;

    if (characterTouchesFloor(character_pos, character_half_sizes, 272.0f,
                              character_new_pos)) {
      character_pos = character_new_pos;
    }

    for (auto& sprite : all_sprites) {
      sprite.cur_pos = sprite.cur_pos + sprite.dir * dt * sprite.velocity;
      float d = (sprite.cur_pos - sprite.pos).GetLength();
      if (d > sprite.max_travel_dist) {
        sprite.dir = -sprite.dir;
      }
    }

    SDL_SetRenderDrawColor(renderer.get(), 255, 128, 128, 128);
    SDL_RenderClear(renderer.get());

    for (auto& sprite : all_sprites) {
      SDL_Texture* sprite_img = nullptr;
      if (sprite.collides) {
        sprite_img = sprite_red;
      } else {
        sprite_img = sprite_black;
      }

      SDL_FRect square = {sprite.cur_pos.x - sprite.half_sizes.x,
                          sprite.cur_pos.y - sprite.half_sizes.y,
                          sprite.half_sizes.x * 2, sprite.half_sizes.y * 2};
      SDL_RenderTexture(renderer.get(), sprite_img, NULL, &square);
    }

    SDL_FRect square = {character_pos.x - character_half_sizes.x,
                        character_pos.y - character_half_sizes.y,
                        character_half_sizes.x * 2, character_half_sizes.y * 2};
    SDL_RenderTexture(renderer.get(), sprite_character, NULL, &square);

    if (kShowSystemCounters) {
      fps = (1.0f / dt) * 0.1f + fps * 0.9f;
      size_t num_playing_audio_streams = audio_device->GetNumPlaying();
      system_info_renderer.ReFormat(
          {{"fps_count", std::format("{:.1f}", fps)},
           {"audio_streams_playing", std::to_string(num_playing_audio_streams)},
           {"down_keys", Keyboard::Instance().GetDownKeysListString()}},
          "system_20.fnt", known_fonts);
      system_info_renderer.Render(0);
    }
    multi_paragraph_demo_renderer.Render((int)multi_paragraph_demo_scroll_y);
    if (multi_paragraph_demo_no_scroll_timeout == 0.0f) {
      multi_paragraph_demo_scroll_y -= dt * 10.0f;
      if (multi_paragraph_demo_scroll_y <
          -(multi_paragraph_demo_renderer.GetContentHeight() - 140)) {
        multi_paragraph_demo_scroll_y =
            -(float)(multi_paragraph_demo_renderer.GetContentHeight() - 140);
      }
    } else {
      multi_paragraph_demo_no_scroll_timeout -= dt;
      if (multi_paragraph_demo_no_scroll_timeout < 0.0f) {
        multi_paragraph_demo_no_scroll_timeout = 0.0f;
      }
    }
    // Test spine
    SDL_SetRenderDrawColor(renderer.get(), 94, 93, 96, 255);
    uint64_t now = SDL_GetPerformanceCounter();
    double deltaTime =
        (now - lastFrameTime) / (double)SDL_GetPerformanceFrequency();
    lastFrameTime = now;

    drawable.update(deltaTime, spine::Physics_Update);
    drawable.draw(renderer.get());

    SDL_RenderPresent(renderer.get());
  }

  delete skeletonData;
  delete skeletonRenderer;

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
