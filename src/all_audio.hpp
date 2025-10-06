#pragma once

#include <symphony_lite/all_symphony.hpp>

namespace gameLD58 {
enum class Sound {
  kButtonClick,
  kHumanoidSelect,
  kKaChing,
  kBeamLoop,
  kBodyFall_1,
  kBodyFall_2,
  kBodyFall_3,
  kCapture,
  kPanic_1,
  kPanic_2,
  kPanic_3,
  kPanic_4,
  kPanic_5,
  kPanic_5,
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

  result.audio[Sound::kBeamLoop] = Symphony::Audio::LoadWave(
      "assets/beam.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);

  result.audio[Sound::kBodyFall_1] = Symphony::Audio::LoadWave(
      "assets/bodyfall_1.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  result.audio[Sound::kBodyFall_2] = Symphony::Audio::LoadWave(
      "assets/bodyfall_2.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  result.audio[Sound::kBodyFall_3] = Symphony::Audio::LoadWave(
      "assets/bodyfall_3.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);

  result.audio[Sound::kCapture] = Symphony::Audio::LoadWave(
      "assets/capture.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);

  result.audio[Sound::kPanic_1] = Symphony::Audio::LoadWave(
      "assets/panic_1.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  result.audio[Sound::kPanic_2] = Symphony::Audio::LoadWave(
      "assets/panic_2.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  result.audio[Sound::kPanic_3] = Symphony::Audio::LoadWave(
      "assets/panic_3.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  result.audio[Sound::kPanic_4] = Symphony::Audio::LoadWave(
      "assets/panic_4.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  result.audio[Sound::kPanic_5] = Symphony::Audio::LoadWave(
      "assets/panic_5.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);
  result.audio[Sound::kPanic_6] = Symphony::Audio::LoadWave(
      "assets/panic_6.wav", Symphony::Audio::WaveFile::kModeLoadInMemory);

  return result;
}
}  // namespace gameLD58
