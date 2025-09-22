#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <thread>
#include <vector>

#include "symphony_lite/all_symphony.hpp"

using namespace Symphony::Math;
using namespace Symphony::Collision;

struct Sprite {
  Point2d pos;
  Point2d cur_pos;
  Vector2d half_sizes;
  Vector2d dir;
  float velocity;
  float max_travel_dist;
  bool collides{false};
};

std::vector<Sprite> all_sprites;

float gravity = -980.0f;

Vector2d character_pos;
Vector2d character_half_sizes;
Vector2d character_cur_velocity;
float character_velocity = 150.0f;
float character_jump_velocity = 400.0f;

bool characterTouchesFloor(const Vector2d& next_pos, float floor_y,
                           Vector2d& new_pos_out) {
  new_pos_out = next_pos;

  float character_down = next_pos.y + character_half_sizes.y;
  if (character_down >= floor_y) {
    new_pos_out.y = floor_y - character_half_sizes.y;
    return true;
  }

  return false;
}

float music_timeout = 0.0f;

int main(int /* argc */, char* /* argv */[]) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);
  IMG_Init(IMG_INIT_PNG);

  SDL_Window* window = SDL_CreateWindow("window", SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, 480, 272, 0);
  std::shared_ptr<SDL_Renderer> renderer{
      SDL_CreateRenderer(window, -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
      &SDL_DestroyRenderer};

  srand(time(0));

  // Load sprites
  SDL_Surface* pixels = IMG_Load("assets/dummy_50x50.png");
  SDL_Texture* sprite_black =
      SDL_CreateTextureFromSurface(renderer.get(), pixels);
  SDL_FreeSurface(pixels);

  pixels = IMG_Load("assets/dummy_50x50.png");
  SDL_Texture* sprite_red =
      SDL_CreateTextureFromSurface(renderer.get(), pixels);
  SDL_FreeSurface(pixels);

  pixels = IMG_Load("assets/character.png");
  SDL_Texture* sprite_character =
      SDL_CreateTextureFromSurface(renderer.get(), pixels);
  character_half_sizes.x = pixels->w / 2.0f;
  character_half_sizes.y = pixels->h / 2.0f;
  SDL_FreeSurface(pixels);

  character_pos = Vector2d(480 / 2, 272 / 2);

  auto audio_device = new Symphony::Audio::Device();
  auto music = Symphony::Audio::LoadWave(
      "assets/bioorange_22k.wav",
      Symphony::Audio::WaveFile::kModeStreamingFromFile);
  auto jump = Symphony::Audio::LoadWave(
      "assets/dummy_22k.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  audio_device->Init();

  auto system_font_30 = Symphony::Text::LoadBmFont("assets/system_30.fnt");
  system_font_30->LoadTexture(renderer);
  auto system_font_50 = Symphony::Text::LoadBmFont("assets/system_50.fnt");
  system_font_50->LoadTexture(renderer);
  std::map<std::string, std::shared_ptr<Symphony::Text::Font>> known_fonts{
      {"system_30.fnt", system_font_30}, {"system_50.fnt", system_font_50}};
  Symphony::Text::TextRenderer system_info_renderer(renderer);
  system_info_renderer.LoadFromFile("assets/system_counters.txt");
  system_info_renderer.SetPosition(5, 5);
  system_info_renderer.SetSizes(475, 267);

  music_timeout = (float)(rand() % 3) + 3;
  std::shared_ptr<Symphony::Audio::PlayingStream> music_stream;

  for (int i = 0; i < 100; ++i) {
    all_sprites.push_back(Sprite());
    all_sprites.back().pos = Point2d(rand() % 480, rand() % 272);
    all_sprites.back().cur_pos = all_sprites.back().pos;
    all_sprites.back().half_sizes = Vector2d(rand() % 10 + 5, rand() % 20 + 5);
    float angle = DegToRad(rand() % 180);
    all_sprites.back().dir = Vector2d(cosf(angle), sinf(angle));
    all_sprites.back().dir.MakeNormalized();
    all_sprites.back().velocity = rand() % 10 + 20;
    all_sprites.back().max_travel_dist = rand() % 50 + 20;
  }

  bool running = true;
  SDL_Event event;

  bool is_left = false;
  bool is_right = false;
  bool is_up = false;

  std::ofstream log;
  log.open("log.txt", std::ofstream::out);

  auto prev_frame_start_time{std::chrono::steady_clock::now()};
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

    if (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;

        case SDL_CONTROLLERDEVICEADDED:
          SDL_GameControllerOpen(event.cdevice.which);
          break;

        case SDL_CONTROLLERBUTTONDOWN:
          log << "event.cbutton.button: " << (int)event.cbutton.button
              << std::endl;
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
            running = false;
          }
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
            is_left = true;
          }
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
            is_right = true;
          }
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP ||
              event.cbutton.button == 0) {
            is_up = true;
          }
          break;

        case SDL_CONTROLLERBUTTONUP:
          log << "event.cbutton.button: " << (int)event.cbutton.button
              << std::endl;
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
            is_left = false;
          }
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
            is_right = false;
          }
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP ||
              event.cbutton.button == 0) {
            is_up = false;
          }
          break;

        case SDL_KEYDOWN:
          if (event.key.keysym.sym == SDLK_LEFT) {
            is_left = true;
          }
          if (event.key.keysym.sym == SDLK_RIGHT) {
            is_right = true;
          }
          if (event.key.keysym.sym == SDLK_UP) {
            is_up = true;
          }
          break;

        case SDL_KEYUP:
          if (event.key.keysym.sym == SDLK_LEFT) {
            is_left = false;
          }
          if (event.key.keysym.sym == SDLK_RIGHT) {
            is_right = false;
          }
          if (event.key.keysym.sym == SDLK_UP) {
            is_up = false;
          }
          break;
      }
    }

    Vector2d character_new_pos;
    bool character_touches_floor =
        characterTouchesFloor(character_pos, 272.0f, character_new_pos);

    if (character_touches_floor) {
      character_cur_velocity.x = 0.0f;
      if (is_left) {
        character_cur_velocity.x = -character_velocity;
      }
      if (is_right) {
        character_cur_velocity.x = character_velocity;
      }
    }
    if (is_up) {
      Vector2d new_pos;
      if (characterTouchesFloor(character_pos, 272.0f, new_pos)) {
        audio_device->Play(jump, Symphony::Audio::PlayTimes(1));
        character_cur_velocity.y = -character_jump_velocity;
      }
    }

    character_cur_velocity.y -= gravity * dt;
    character_pos = character_pos + character_cur_velocity * dt;

    if (characterTouchesFloor(character_pos, 272.0f, character_new_pos)) {
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

      SDL_Rect square = {(int)sprite.cur_pos.x - (int)sprite.half_sizes.x,
                         (int)sprite.cur_pos.y - (int)sprite.half_sizes.y,
                         (int)sprite.half_sizes.x * 2,
                         (int)sprite.half_sizes.y * 2};
      SDL_RenderCopy(renderer.get(), sprite_img, NULL, &square);
    }

    SDL_Rect square = {(int)character_pos.x - (int)character_half_sizes.x,
                       (int)character_pos.y - (int)character_half_sizes.y,
                       (int)character_half_sizes.x * 2,
                       (int)character_half_sizes.y * 2};
    SDL_RenderCopy(renderer.get(), sprite_character, NULL, &square);

    system_info_renderer.ReFormat(
        {{"fps_count", "60"}, {"audio_streams_playing", "0"}}, "system_30.fnt",
        known_fonts);
    system_info_renderer.Render();

    SDL_RenderPresent(renderer.get());
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
