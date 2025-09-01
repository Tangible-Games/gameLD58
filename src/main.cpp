#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <ctime>
#include <symphony_lite/all_symphony.hpp>
#include <vector>

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

int main(int /* argc */, char* /* argv */[]) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);
  IMG_Init(IMG_INIT_PNG);

  SDL_Window* window = SDL_CreateWindow("window", SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, 480, 272, 0);

  SDL_Renderer* renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  srand(time(0));

  // Load sprites
  SDL_Surface* pixels = IMG_Load("assets/psp.png");
  SDL_Texture* spriteBlack = SDL_CreateTextureFromSurface(renderer, pixels);
  SDL_FreeSurface(pixels);

  pixels = IMG_Load("assets/psp_red.png");
  SDL_Texture* spriteRed = SDL_CreateTextureFromSurface(renderer, pixels);
  SDL_FreeSurface(pixels);

  auto audio_device = new Symphony::Audio::Device();
  auto music = Symphony::Audio::LoadWave(
      "assets/bioorange_44k.wav",
      Symphony::Audio::WaveFile::kModeStreamingFromFile);
  audio_device->Init();
  audio_device->Play(music, Symphony::Audio::PlayTimes(3));

  for (int i = 0; i < 100; ++i) {
    all_sprites.push_back(Sprite());
    all_sprites.back().pos = Point2d(rand() % 480, rand() % 272);
    all_sprites.back().cur_pos = all_sprites.back().pos;
    all_sprites.back().half_sizes = Vector2d(rand() % 10 + 2, rand() % 20 + 4);
    float angle = DegToRad(rand() % 180);
    all_sprites.back().dir = Vector2d(cosf(angle), sinf(angle));
    all_sprites.back().dir.MakeNormalized();
    all_sprites.back().velocity = rand() % 6 + 2;
    all_sprites.back().max_travel_dist = rand() % 50 + 20;
  }

  bool running = true;
  SDL_Event event;
  auto prev_time = SDL_GetTicks64();
  while (running) {
    auto cur_time = SDL_GetTicks64();

    float dt = (cur_time - prev_time) / 1000.0f;
    if (dt < 1.0f / 60.0f) {
      dt = 1.0f / 60.0f;
    }

    prev_time = cur_time;

    audio_device->Update(dt);

    // Process input
    if (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          // End the loop if the programs is being closed
          running = false;
          break;
        case SDL_CONTROLLERDEVICEADDED:
          // Connect a controller when it is connected
          SDL_GameControllerOpen(event.cdevice.which);
          break;
        case SDL_CONTROLLERBUTTONDOWN:
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
            // Close the program if start is pressed
            running = false;
          }
          break;
      }
    }

    for (auto& sprite : all_sprites) {
      sprite.cur_pos = sprite.cur_pos + sprite.dir * dt * sprite.velocity;
      float d = (sprite.cur_pos - sprite.pos).GetLength();
      if (d > sprite.max_travel_dist) {
        sprite.dir = -sprite.dir;
      }
    }

    SpatialBin2d<int> broad_phase(50.0f, 50.0f, 256);
    for (size_t i = 0; i < all_sprites.size(); ++i) {
      broad_phase.Add(all_sprites[i].cur_pos, all_sprites[i].half_sizes, i);
    }

    for (size_t i = 0; i < all_sprites.size(); ++i) {
      auto& sprite = all_sprites[i];

      sprite.collides = false;

      std::vector<int> pcs;
      broad_phase.Query(sprite.cur_pos, sprite.half_sizes, pcs);

      AARect2d rect(sprite.cur_pos, sprite.half_sizes);
      for (int pc_index : pcs) {
        if (pc_index == static_cast<int>(i)) {
          continue;
        }

        AARect2d pc_rect(all_sprites[pc_index].cur_pos,
                         all_sprites[pc_index].half_sizes);
        if (rect.Intersect(pc_rect)) {
          sprite.collides = true;
          break;
        }
      }
    }
    // Draw everything on a white background
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Clear the screen
    SDL_RenderClear(renderer);

    // Draw a red square
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for (auto& sprite : all_sprites) {
      SDL_Texture* spriteImg = nullptr;
      if (sprite.collides) {
        spriteImg = spriteRed;
      } else {
        spriteImg = spriteBlack;
      }

      SDL_Rect square = {(int)sprite.cur_pos.x - (int)sprite.half_sizes.x,
                         (int)sprite.cur_pos.y - (int)sprite.half_sizes.y,
                         (int)sprite.half_sizes.x * 2,
                         (int)sprite.half_sizes.y * 2};
      SDL_RenderCopy(renderer, spriteImg, NULL, &square);
    }

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
