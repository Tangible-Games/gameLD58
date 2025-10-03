#pragma once

namespace gameLD58 {
class Demo {
 public:
  Demo(std::shared_ptr<SDL_Renderer> renderer,
       std::shared_ptr<Symphony::Audio::Device> audio)
      : renderer_(renderer), audio_(audio) {}

  void Load();
  void Draw();

 private:
  std::shared_ptr<SDL_Renderer> renderer_;
  std::shared_ptr<Symphony::Audio::Device> audio_;
};

void Demo::Load() {}

void Demo::Draw() {}

}  // namespace gameLD58
