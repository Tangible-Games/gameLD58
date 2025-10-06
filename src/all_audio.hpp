#pragma once

#include <symphony_lite/all_symphony.hpp>

namespace gameLD58 {
enum class Sound {
  kButtonClick,
  kHumanoidSelect,
  kKaChing,
};

struct AllAudio {
  std::unordered_map<Sound, std::shared_ptr<Symphony::Audio::WaveFile>> audio;
};

AllAudio LoadAllAudio() {
  AllAudio result;

  result.audio[Sound::kButtonClick] = Symphony::Audio::LoadWave(
      "assets/button_click.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  result.audio[Sound::kHumanoidSelect] = Symphony::Audio::LoadWave(
      "assets/select_human.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  result.audio[Sound::kKaChing] = Symphony::Audio::LoadWave(
      "assets/sell_human.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);

  return result;
}
}  // namespace gameLD58
