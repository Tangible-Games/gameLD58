#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_rect.h>
#include <SDL3_image/SDL_image.h>

#include <memory>
#if defined __PSP__
#include <pspkernel.h>
#endif
#ifdef EMSCRIPTEN_TARGET
#include <emscripten/emscripten.h>
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

using namespace gameLD58;

namespace {

struct GameCtx {
  Game* game{nullptr};
  float fps = 0.0f;
  std::chrono::time_point<std::chrono::steady_clock> prev_frame_start_time{
      std::chrono::steady_clock::now()};
  SDL_Window* window{nullptr};
  std::shared_ptr<SDL_Renderer> renderer;
  std::shared_ptr<Symphony::Text::TextRenderer> system_info_renderer;
  std::shared_ptr<Symphony::Audio::Device> audio;
};

bool game_running = true;
std::map<std::string, std::shared_ptr<Symphony::Text::Font>>
    system_info_renderer_fonts;

class ExitHandler : public Keyboard::Callback {
 public:
  void OnKeyDown(Keyboard::Key /*key*/) override {}

  void OnKeyUp(Keyboard::Key key) override {
    if (key == Keyboard::Key::kSelect) {
      game_running = false;
    }
  }
};

void mainloop(void* gameCtx) {
  auto* ctx = reinterpret_cast<GameCtx*>(gameCtx);

  if (!ctx->game->IsRunning()) {
    LOGI("Exiting main loop.");

    ctx->audio.reset();

    ctx->renderer.reset();

    SDL_DestroyWindow(ctx->window);
    SDL_Quit();
#ifdef __EMSCRIPTEN__
    emscripten_cancel_main_loop(); /* this should "kill" the app. */
    delete ctx;
#else
    game_running = false;
#endif
    return;
  }

  auto frame_start_time{std::chrono::steady_clock::now()};
  std::chrono::duration<float> dt_period_seconds{frame_start_time -
                                                 ctx->prev_frame_start_time};
  float dt = dt_period_seconds.count();
  ctx->prev_frame_start_time = frame_start_time;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT:
        ctx->game->Quit();
        break;
      default:
        break;
    }

    Keyboard::Instance().OnEvent(&event);
  }

  Keyboard::Instance().Update(dt);

  ctx->game->Update(dt);

  SDL_SetRenderDrawColor(ctx->renderer.get(), 0, 0, 0, 255);
  SDL_RenderClear(ctx->renderer.get());
  ctx->game->Draw();
  if (kDrawSystemCounters) {
    ctx->fps = ((1.0f / dt) * 0.1f) + (ctx->fps * 0.9f);
    size_t num_playing_audio_streams = ctx->audio->GetNumPlaying();
    ctx->system_info_renderer->ReFormat(
        {{"fps_count", std::format("{:.1f}", ctx->fps)},
         {"audio_streams_playing", std::to_string(num_playing_audio_streams)},
         {"down_keys", Keyboard::Instance().GetDownKeysListString()}},
        "system_20.fnt", system_info_renderer_fonts);
    ctx->system_info_renderer->Render(0);
  }
  SDL_RenderPresent(ctx->renderer.get());

  if (ctx->game->ReadyForLoading()) {
    ctx->game->Load();

    ctx->prev_frame_start_time = std::chrono::steady_clock::now();
  }
}

}  // namespace

#if defined __PSP__
int exit_callback(int /*arg1*/, int /*arg2*/, void* /*arg*/) {
  game_running = false;
  sceKernelExitGame();
  return 0;
}
#endif

int main(int /* argc */, char* /* argv */[]) {
  Symphony::Log::Logger::init().set_verbosity(
      Symphony::Log::Logger::Verbosity::DEBUG);

#ifndef EMSCRIPTEN_TARGET
  Symphony::Log::Logger::instance().add_sink(
      Symphony::Log::FileSink::create(LOG_FILE));
#endif

  LOGI("Game starting...");

#ifndef EMSCRIPTEN_TARGET
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);
#else
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
#endif

  LOGI("SDL initialized.");

  auto* ctx = new GameCtx;

  ctx->window = SDL_CreateWindow("window", kScreenWidth, kScreenHeight, 0);
  ctx->renderer = std::shared_ptr<SDL_Renderer>{
      SDL_CreateRenderer(ctx->window, nullptr), &SDL_DestroyRenderer};
  if (!ctx->renderer) {
    LOGI("Creating renderer failed, error: {}", SDL_GetError());
  }
  LOGI("Window is created.");

  if (!SDL_SetRenderVSync(ctx->renderer.get(), 1)) {
    LOGE("Could not enable VSync! SDL error: {}", SDL_GetError());
    return 1;
  }

  // Fix rendering on linux.
  SDL_Rect clip = {
      .x = 0,
      .y = 0,
      .w = kScreenWidth,
      .h = kScreenHeight,
  };
  SDL_SetRenderClipRect(ctx->renderer.get(), &clip);

  ctx->audio = std::make_shared<Symphony::Audio::Device>();
  ctx->audio->Init();
  LOGI("Audio is created and initialized.");

  ctx->system_info_renderer =
      std::make_unique<Symphony::Text::TextRenderer>(ctx->renderer);

  if (kDrawSystemCounters) {
    auto system_font_20 = Symphony::Text::LoadBmFont("assets/system_20.fnt");
    system_font_20->LoadTexture(ctx->renderer);

    system_info_renderer_fonts.insert(
        std::make_pair("system_20", system_font_20));

    ctx->system_info_renderer->LoadFromFile("assets/system_counters.txt");
    ctx->system_info_renderer->SetPosition(5, 5);
    ctx->system_info_renderer->SetSizes(kScreenWidth - 5, kScreenHeight - 10);
  }

  ctx->game = new Game(ctx->renderer, ctx->audio);

  ctx->prev_frame_start_time = std::chrono::steady_clock::now();
  ctx->fps = 0.0f;

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(mainloop, ctx, 0, 1);
#else
  while (game_running) {
    mainloop(ctx);
  }
  delete ctx;
#endif

  return 0;
}