#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_rect.h>
#include <SDL3_image/SDL_image.h>
#include <spine/spine.h>
#if defined __PSP__
#include <pspkernel.h>
#endif

#include <chrono>
#include <ctime>
#include <fstream>
#include <symphony_lite/all_symphony.hpp>
#include <thread>
#include <vector>

#include "consts.hpp"
#include "game.hpp"
#include "keyboard.hpp"
#include "spine_sdl3.h"

using namespace gameLD58;

bool game_running = true;

class ExitHandler : public Keyboard::Callback {
 public:
  void OnKeyDown(Keyboard::Key /*key*/) override {}

  void OnKeyUp(Keyboard::Key key) override {
    if (key == Keyboard::Key::kSelect) {
      game_running = false;
    }
  }
};

#if defined __PSP__
int exit_callback(int /*arg1*/, int /*arg2*/, void* /*arg*/) {
  game_running = false;
  sceKernelExitGame();
  return 0;
}
#endif

int main(int /* argc */, char* /* argv */[]) {
  Symphony::Log::Logger::init()
      .set_verbosity(Symphony::Log::Logger::Verbosity::DEBUG)
      .add_sink(Symphony::Log::FileSink::create(LOG_FILE));

  LOGI("Game starting...");

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);

  LOGI("SDL initialized.");

  SDL_Window* window =
      SDL_CreateWindow("window", kScreenWidth, kScreenHeight, 0);
  std::shared_ptr<SDL_Renderer> renderer{SDL_CreateRenderer(window, nullptr),
                                         &SDL_DestroyRenderer};
  if (!renderer) {
    LOGI("Creating renderer failed, error: {}", SDL_GetError());
  }
  LOGI("Window is created.");

  // Fix rendering on linux.
  SDL_Rect clip = {
      .x = 0,
      .y = 0,
      .w = kScreenWidth,
      .h = kScreenHeight,
  };
  SDL_SetRenderClipRect(renderer.get(), &clip);

  auto audio = std::make_shared<Symphony::Audio::Device>();
  LOGI("Audio is created.");

  Game game(renderer, audio);

  auto prev_frame_start_time{std::chrono::steady_clock::now()};
  while (game.IsRunning()) {
    auto frame_start_time{std::chrono::steady_clock::now()};
    std::chrono::duration<float> dt_period_seconds{frame_start_time -
                                                   prev_frame_start_time};
    float dt = dt_period_seconds.count();
    prev_frame_start_time = frame_start_time;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT:
          game.Quit();
          break;
      }

      Keyboard::Instance().OnEvent(&event);
    }

    Keyboard::Instance().Update(dt);

    game.Update(dt);

    SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
    SDL_RenderClear(renderer.get());
    game.Draw();
    SDL_RenderPresent(renderer.get());

    if (game.ReadyForLoading()) {
      game.Load();

      prev_frame_start_time = std::chrono::steady_clock::now();
    }
  }

  renderer.reset();

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
