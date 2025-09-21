#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vlog/vlog.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <symphony_lite/all_symphony.hpp>
#include <thread>
#include <vector>

using namespace Symphony::Math;
using namespace Symphony::Collision;

namespace {

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

void handleControllerButtonDownEvent(SDL_Event& event, bool& running,
                                     bool& is_left, bool& is_right,
                                     bool& is_up) {
  LOGD("event.cbutton.button: {}", (int)event.cbutton.button);
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
}

void handleControllerButtonUpEvent(SDL_Event& event,
                                   [[maybe_unused]] bool& running,
                                   bool& is_left, bool& is_right, bool& is_up) {
  LOGD("event.cbutton.button: {}", (int)event.cbutton.button);
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
}

void handleKeyDownEvent(SDL_Event& event, bool& running, bool& is_left,
                        bool& is_right, bool& is_up) {
  if (event.key.keysym.sym == SDLK_ESCAPE) {
    running = false;
  }
  if (event.key.keysym.sym == SDLK_LEFT) {
    is_left = true;
  }
  if (event.key.keysym.sym == SDLK_RIGHT) {
    is_right = true;
  }
  if (event.key.keysym.sym == SDLK_UP) {
    is_up = true;
  }
}

void handleKeyUpEvent(SDL_Event& event, [[maybe_unused]] bool& running,
                      bool& is_left, bool& is_right, bool& is_up) {
  if (event.key.keysym.sym == SDLK_LEFT) {
    is_left = false;
  }
  if (event.key.keysym.sym == SDLK_RIGHT) {
    is_right = false;
  }
  if (event.key.keysym.sym == SDLK_UP) {
    is_up = false;
  }
}

}  // namespace

int main(int /* argc */, char* /* argv */[]) {
  std::vector<Sprite> all_sprites;

  Vector2d character_pos;
  Vector2d character_half_sizes;
  Vector2d character_cur_velocity;

  float music_timeout = 0.0f;

  vlog::Logger::init()
      .set_verbosity(vlog::Logger::Verbosity::DEBUG)
      .add_sink(vlog::FileSink::create(LOG_FILE));

  LOGI("Game starting...");

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);
  IMG_Init(IMG_INIT_PNG);

  SDL_Window* window = SDL_CreateWindow("window", SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, 480, 272, 0);
  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  srand(time(nullptr));

  // Load sprites
  SDL_Surface* pixels = IMG_Load("assets/dummy_50x50.png");
  SDL_Texture* sprite_black = SDL_CreateTextureFromSurface(renderer, pixels);
  SDL_FreeSurface(pixels);

  pixels = IMG_Load("assets/dummy_50x50.png");
  SDL_Texture* sprite_red = SDL_CreateTextureFromSurface(renderer, pixels);
  SDL_FreeSurface(pixels);

  pixels = IMG_Load("assets/character.png");
  SDL_Texture* sprite_character =
      SDL_CreateTextureFromSurface(renderer, pixels);
  character_half_sizes.x = pixels->w / 2.0f;
  character_half_sizes.y = pixels->h / 2.0f;
  SDL_FreeSurface(pixels);

  character_pos = Vector2d(480.0f / 2, 272.0f / 2);

  auto audio_device = std::make_unique<Symphony::Audio::Device>();
  auto music = Symphony::Audio::LoadWave(
      "assets/bioorange_22k.wav",
      Symphony::Audio::WaveFile::kModeStreamingFromFile);
  auto jump = Symphony::Audio::LoadWave(
      "assets/dummy_22k.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  audio_device->Init();

  auto fps_font = Symphony::Text::LoadBmFont("assets/system_30.fnt");

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

  SDL_Event event;

  bool running = true;
  bool is_left = false;
  bool is_right = false;
  bool is_up = false;

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

    if (SDL_PollEvent(&event) != 0) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;

        case SDL_CONTROLLERDEVICEADDED:
          SDL_GameControllerOpen(event.cdevice.which);
          break;

        case SDL_CONTROLLERBUTTONDOWN:
          handleControllerButtonDownEvent(event, running, is_left, is_right,
                                          is_up);
          break;

        case SDL_CONTROLLERBUTTONUP:
          handleControllerButtonUpEvent(event, running, is_left, is_right,
                                        is_up);
          break;

        case SDL_KEYDOWN:
          handleKeyDownEvent(event, running, is_left, is_right, is_up);
          break;

        case SDL_KEYUP:
          handleKeyUpEvent(event, running, is_left, is_right, is_up);
          break;

        default:
          break;
      }
    }

    Vector2d character_new_pos;
    bool character_touches_floor = characterTouchesFloor(
        character_pos, character_half_sizes, 272.0f, character_new_pos);

    if (character_touches_floor) {
      character_cur_velocity.x = 0.0f;
      if (is_left) {
        character_cur_velocity.x = -kCharacter_velocity;
      }
      if (is_right) {
        character_cur_velocity.x = kCharacter_velocity;
      }
    }
    if (is_up) {
      Vector2d new_pos;
      if (characterTouchesFloor(character_pos, character_half_sizes, 272.0f,
                                new_pos)) {
        audio_device->Play(jump, Symphony::Audio::PlayTimes(1));
        character_cur_velocity.y = -kCharacter_jump_velocity;
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

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

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
      SDL_RenderCopy(renderer, sprite_img, nullptr, &square);
    }

    SDL_Rect square = {(int)character_pos.x - (int)character_half_sizes.x,
                       (int)character_pos.y - (int)character_half_sizes.y,
                       (int)character_half_sizes.x * 2,
                       (int)character_half_sizes.y * 2};
    SDL_RenderCopy(renderer, sprite_character, nullptr, &square);

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
