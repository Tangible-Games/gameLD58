#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_set>
#include <vector>

#include "log.hpp"
#include "wave_loader.hpp"

namespace Symphony {
namespace Audio {
struct PlayCount {
  int num_repeats{1};
  bool loop_infinite{false};
};

static constexpr PlayCount kPlayLooped{.num_repeats = 0, .loop_infinite = true};
static constexpr PlayCount kPlayOnce{.num_repeats = 1, .loop_infinite = false};
PlayCount PlayTimes(int num_repeats) {
  return PlayCount{.num_repeats = num_repeats, .loop_infinite = false};
}

struct FadeControl {
  float fade_in_time_sec{0};
  float fade_out_time_sec{0};
};

static constexpr FadeControl kNoFade{.fade_in_time_sec = 0,
                                     .fade_out_time_sec = 0};
FadeControl FadeInOut(float fade_in_time_sec, float fade_out_time_sec) {
  return FadeControl{.fade_in_time_sec = fade_in_time_sec,
                     .fade_out_time_sec = fade_out_time_sec};
}

struct StopControl {
  bool stop_at_end{false};
  std::optional<float> fade_out_time_sec;
};

StopControl StopFade(float fade_out_time_sec) {
  return StopControl{.stop_at_end = false,
                     .fade_out_time_sec = fade_out_time_sec};
}

StopControl StopAtEndFade(float fade_out_time_sec) {
  return StopControl{.stop_at_end = true,
                     .fade_out_time_sec = fade_out_time_sec};
}

StopControl StopAtEnd() {
  return StopControl{.stop_at_end = true, .fade_out_time_sec = 0.0f};
}

class Device;

class PlayingStream {
 public:
  PlayingStream() = default;
  virtual ~PlayingStream() {}
};

class Device {
 public:
  Device() = default;

  ~Device();

  void Init();

  std::shared_ptr<PlayingStream> Play(
      std::shared_ptr<WaveFile> wave_file, const PlayCount& play_count,
      const FadeControl& fade_control = kNoFade);

  bool IsPlaying(std::shared_ptr<PlayingStream> playing_stream);
  size_t GetNumPlaying();
  void Stop(std::shared_ptr<PlayingStream> playing_stream,
            const StopControl& stop_control);
  void StopImmediately(std::shared_ptr<PlayingStream> playing_stream);

 private:
  static inline constexpr int32_t kMaxGain = 128;
  static int32_t ToIntGain(float gain) { return (int32_t)(gain * 128.0f); }
  static int32_t ApplyGain(int32_t sample, int32_t gain) {
    return (sample * gain) >> 7;
  }

  static inline constexpr int32_t kSampleMax16 = 32767;
  static inline constexpr int32_t kSampleMin16 = -32768;

  enum class GainState { kAttack, kSustain, kRelease };

  struct PlayingStreamInternal : public PlayingStream {
   public:
    PlayingStreamInternal(std::shared_ptr<WaveFile> new_wave_file,
                          const PlayCount& new_play_count,
                          const FadeControl& new_fade_control)
        : wave_file(new_wave_file),
          play_count(new_play_count),
          fade_control(new_fade_control) {}

    std::shared_ptr<WaveFile> wave_file;
    PlayCount play_count;
    int num_plays{0};
    FadeControl fade_control;
    size_t looped_blocks_streamed{0};
    size_t total_blocks_streamed{0};
    size_t total_blocks_to_play{0};
    GainState gain_state{GainState::kAttack};
    float cur_gain{1.0f};
    float gain_at_release{0.0f};
    std::optional<StopControl> stop_control;
    std::optional<StopControl> stop_control_in_callback;
  };

#pragma pack(push, 1)
  struct StereoBlock16 {
    int16_t left;
    int16_t right;
  };

  struct StereoBlock32 {
    int32_t left;
    int32_t right;
  };
#pragma pack(pop)

  static void destroyAudioDevice(SDL_AudioStream* stream);

  static void startPlayingStream(
      PlayingStreamInternal* playing_stream_internal);

  // Returns: gain.
  static int32_t updateGainStateInCallback(
      PlayingStreamInternal* playing_stream_internal);

  static void accumulateStereoSamples(StereoBlock32* accumulate_buffer,
                                      const StereoBlock16* stream,
                                      size_t num_blocks);
  static void accumulateStereoSamplesWithGain(StereoBlock32* accumulate_buffer,
                                              int32_t gain,
                                              const StereoBlock16* stream,
                                              size_t num_blocks);
  static void accumulateMonoSamples(StereoBlock32* accumulate_buffer,
                                    const int16_t* stream, size_t num_blocks);
  static void accumulateMonoSamplesWithGain(StereoBlock32* accumulate_buffer,
                                            int32_t gain, const int16_t* stream,
                                            size_t num_blocks);
  static void accumulateSamples(StereoBlock32* accumulate_buffer, int32_t gain,
                                size_t num_channels, const int16_t* stream,
                                size_t num_blocks);

  void allocateMixBuffer(size_t num_blocks);
  void allocateReadBuffer(size_t num_blocks);
  void allocateSendBuffer(size_t num_blocks);

  static void dataCallback(void* userdata, SDL_AudioStream* stream,
                           int additional_amount, int total_amount);
  void fillMixBuffer(int bytes_amount);
  void sendMixedToMainStream(int bytes_amount);

  std::shared_ptr<SDL_AudioStream> sdl_audio_stream_;
  std::mutex mutex_;
  std::unordered_set<std::shared_ptr<PlayingStream>> playing_streams_;
  std::vector<StereoBlock32> mix_buffer_;
  std::vector<StereoBlock16> send_buffer_;
  std::vector<int16_t> read_buffer_;
};

Device::~Device() {
  std::lock_guard<std::mutex> lock(mutex_);
  playing_streams_.clear();
}

void Device::Init() {
  SDL_AudioSpec sdl_audio_spec;

  sdl_audio_spec.freq = 22050;
  sdl_audio_spec.format = SDL_AUDIO_S16;
  sdl_audio_spec.channels = 2;

  sdl_audio_stream_.reset(
      SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                &sdl_audio_spec, dataCallback, this),
      destroyAudioDevice);
  if (!sdl_audio_stream_) {
    LOGE("[Symphony::Audio::Device] Failed to create stream: {}",
         SDL_GetError());
  }

  allocateMixBuffer(512);
  allocateReadBuffer(512);
  send_buffer_.resize(512);

  SDL_ResumeAudioStreamDevice(sdl_audio_stream_.get());
}

std::shared_ptr<PlayingStream> Device::Play(std::shared_ptr<WaveFile> wave_file,
                                            const PlayCount& play_count,
                                            const FadeControl& fade_control) {
  if (!wave_file->GetNumBlocks()) {
    LOGE("[Symphony::Audio::Device] Not playing empty wave file: {}",
         wave_file->GetFilePath());
    return nullptr;
  }

  std::shared_ptr<PlayingStream> playing_stream(
      new PlayingStreamInternal(wave_file, play_count, fade_control));
  PlayingStreamInternal* playing_stream_internal =
      (PlayingStreamInternal*)playing_stream.get();

  startPlayingStream(playing_stream_internal);

  {
    std::lock_guard<std::mutex> lock(mutex_);
    playing_streams_.insert(playing_stream);
  }

  return playing_stream;
}

bool Device::IsPlaying(std::shared_ptr<PlayingStream> playing_stream) {
  if (!playing_stream) {
    return false;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  return playing_streams_.find(playing_stream) != playing_streams_.end();
}

size_t Device::GetNumPlaying() {
  std::lock_guard<std::mutex> lock(mutex_);
  return playing_streams_.size();
}

void Device::Stop(std::shared_ptr<PlayingStream> playing_stream,
                  const StopControl& stop_control) {
  if (!playing_stream) {
    return;
  }

  PlayingStreamInternal* playing_stream_internal =
      static_cast<PlayingStreamInternal*>(playing_stream.get());

  std::lock_guard<std::mutex> lock(mutex_);
  playing_stream_internal->stop_control = stop_control;
}

void Device::StopImmediately(std::shared_ptr<PlayingStream> playing_stream) {
  if (!playing_stream) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  playing_streams_.erase(playing_stream);
}

void Device::destroyAudioDevice(SDL_AudioStream* /*stream*/) {}

void Device::startPlayingStream(
    PlayingStreamInternal* playing_stream_internal) {
  if (playing_stream_internal->fade_control.fade_in_time_sec > 0.0f) {
    playing_stream_internal->gain_state = GainState::kAttack;
  } else {
    playing_stream_internal->gain_state = GainState::kSustain;
  }

  if (!playing_stream_internal->play_count.loop_infinite) {
    playing_stream_internal->total_blocks_to_play =
        playing_stream_internal->wave_file->GetNumBlocks() *
        playing_stream_internal->play_count.num_repeats;
  } else {
    playing_stream_internal->total_blocks_to_play = 0;
  }
}

int32_t Device::updateGainStateInCallback(
    PlayingStreamInternal* playing_stream_internal) {
  int32_t gain = kMaxGain;

  // Time to stop, stop_control_in_callback works like one-off signal:
  if (playing_stream_internal->stop_control_in_callback.has_value()) {
    const StopControl& stop_control =
        playing_stream_internal->stop_control_in_callback.value();

    playing_stream_internal->fade_control.fade_out_time_sec =
        stop_control.fade_out_time_sec.value_or(
            playing_stream_internal->fade_control.fade_out_time_sec);

    size_t num_blocks_left = 0;
    if (stop_control.stop_at_end) {
      num_blocks_left = playing_stream_internal->wave_file->GetNumBlocks() -
                        playing_stream_internal->looped_blocks_streamed;
      if (playing_stream_internal->total_blocks_to_play) {
        size_t num_blocks_left_to_play =
            playing_stream_internal->total_blocks_to_play -
            playing_stream_internal->total_blocks_streamed;
        if (num_blocks_left_to_play < num_blocks_left) {
          num_blocks_left = num_blocks_left_to_play;
        }
      }
    }

    if (num_blocks_left == 0) {
      num_blocks_left = static_cast<size_t>(
          playing_stream_internal->wave_file->GetSampleRate() *
          playing_stream_internal->fade_control.fade_out_time_sec);
    }

    size_t num_blocks_to_fade_out = static_cast<size_t>(
        playing_stream_internal->wave_file->GetSampleRate() *
        playing_stream_internal->fade_control.fade_out_time_sec);
    if (num_blocks_left < num_blocks_to_fade_out) {
      playing_stream_internal->fade_control.fade_out_time_sec =
          static_cast<float>(num_blocks_left) /
          static_cast<float>(
              playing_stream_internal->wave_file->GetSampleRate());
    }

    playing_stream_internal->total_blocks_to_play =
        playing_stream_internal->total_blocks_streamed + num_blocks_left;

    if (playing_stream_internal->gain_state == GainState::kRelease) {
      playing_stream_internal->gain_at_release =
          playing_stream_internal->cur_gain;
    }

    playing_stream_internal->stop_control_in_callback = std::nullopt;
  }

  if (playing_stream_internal->gain_state == GainState::kAttack) {
    // It should be possible to switch to GainState::kRelease in
    // GainState::kAttack too.
    if (playing_stream_internal->total_blocks_to_play) {
      size_t num_blocks_to_fade_out = static_cast<size_t>(
          playing_stream_internal->wave_file->GetSampleRate() *
          playing_stream_internal->fade_control.fade_out_time_sec);

      if (playing_stream_internal->total_blocks_streamed +
              num_blocks_to_fade_out >
          playing_stream_internal->total_blocks_to_play) {
        playing_stream_internal->gain_at_release =
            playing_stream_internal->cur_gain;
        playing_stream_internal->gain_state = GainState::kRelease;
      }
    }
  }

  if (playing_stream_internal->gain_state == GainState::kAttack) {
    // We didn't switch previously, checks end of GainState::kAttack state:
    size_t num_blocks_to_fade_in =
        (size_t)(playing_stream_internal->wave_file->GetSampleRate() *
                 playing_stream_internal->fade_control.fade_in_time_sec);
    if (playing_stream_internal->total_blocks_streamed >
        num_blocks_to_fade_in) {
      playing_stream_internal->cur_gain = 1.0f;
      gain = kMaxGain;

      playing_stream_internal->gain_state = GainState::kSustain;
    } else {
      playing_stream_internal->cur_gain =
          (float)playing_stream_internal->total_blocks_streamed /
          (float)num_blocks_to_fade_in;
      gain = ToIntGain(playing_stream_internal->cur_gain);
    }
  }

  if (playing_stream_internal->gain_state == GainState::kSustain) {
    if (playing_stream_internal->fade_control.fade_out_time_sec > 0.0f) {
      if (playing_stream_internal->total_blocks_to_play) {
        size_t num_blocks_to_fade_out =
            (size_t)(playing_stream_internal->wave_file->GetSampleRate() *
                     playing_stream_internal->fade_control.fade_out_time_sec);

        if (playing_stream_internal->total_blocks_streamed +
                num_blocks_to_fade_out >
            playing_stream_internal->total_blocks_to_play) {
          playing_stream_internal->gain_at_release =
              playing_stream_internal->cur_gain;
          playing_stream_internal->gain_state = GainState::kRelease;
        }
      }
    }
  }

  if (playing_stream_internal->gain_state == GainState::kRelease) {
    if (playing_stream_internal->total_blocks_streamed >=
        playing_stream_internal->total_blocks_to_play) {
      playing_stream_internal->cur_gain = 0.0f;
      gain = 0;
    } else {
      size_t num_blocks_to_fade_out =
          (size_t)(playing_stream_internal->wave_file->GetSampleRate() *
                   playing_stream_internal->fade_control.fade_out_time_sec);

      if (playing_stream_internal->total_blocks_streamed +
              num_blocks_to_fade_out >=
          playing_stream_internal->total_blocks_to_play) {
        size_t num_blocks_left_to_play =
            playing_stream_internal->total_blocks_to_play -
            playing_stream_internal->total_blocks_streamed;
        playing_stream_internal->cur_gain =
            ((float)num_blocks_left_to_play / (float)num_blocks_to_fade_out) *
            playing_stream_internal->gain_at_release;
      }

      gain = ToIntGain(playing_stream_internal->cur_gain);
    }
  }

  return gain;
}

void Device::accumulateStereoSamples(StereoBlock32* accumulate_buffer,
                                     const StereoBlock16* stream,
                                     size_t num_blocks) {
  for (size_t i = 0; i < num_blocks; ++i) {
    accumulate_buffer[i].left += stream[i].left;
    accumulate_buffer[i].right += stream[i].right;
  }
}

void Device::accumulateStereoSamplesWithGain(StereoBlock32* accumulate_buffer,
                                             int32_t gain,
                                             const StereoBlock16* stream,
                                             size_t num_blocks) {
  for (size_t i = 0; i < num_blocks; ++i) {
    accumulate_buffer[i].left += ApplyGain(stream[i].left, gain);
    accumulate_buffer[i].right += ApplyGain(stream[i].right, gain);
  }
}

void Device::accumulateMonoSamples(StereoBlock32* accumulate_buffer,
                                   const int16_t* stream, size_t num_blocks) {
  for (size_t i = 0; i < num_blocks; ++i) {
    accumulate_buffer[i].left += stream[i];
    accumulate_buffer[i].right += stream[i];
  }
}

void Device::accumulateMonoSamplesWithGain(StereoBlock32* accumulate_buffer,
                                           int32_t gain, const int16_t* stream,
                                           size_t num_blocks) {
  for (size_t i = 0; i < num_blocks; ++i) {
    accumulate_buffer[i].left += ApplyGain(stream[i], gain);
    accumulate_buffer[i].right += ApplyGain(stream[i], gain);
  }
}

void Device::accumulateSamples(StereoBlock32* accumulate_buffer, int32_t gain,
                               size_t num_channels, const int16_t* stream,
                               size_t num_blocks) {
  if (num_channels == 1) {
    if (gain == kMaxGain) {
      accumulateMonoSamples(accumulate_buffer, stream, num_blocks);
    } else {
      accumulateMonoSamplesWithGain(accumulate_buffer, gain, stream,
                                    num_blocks);
    }
  } else if (num_channels == 2) {
    const StereoBlock16* stereo_blocks_16 = (const StereoBlock16*)stream;
    if (gain == kMaxGain) {
      accumulateStereoSamples(accumulate_buffer, stereo_blocks_16, num_blocks);
    } else {
      accumulateStereoSamplesWithGain(accumulate_buffer, gain, stereo_blocks_16,
                                      num_blocks);
    }
  }
}

void Device::allocateMixBuffer(size_t num_blocks) {
  if (mix_buffer_.size() < num_blocks) {
    mix_buffer_.resize(num_blocks);
  }
}

void Device::allocateReadBuffer(size_t num_blocks) {
  if (read_buffer_.size() < num_blocks * 2) {
    read_buffer_.resize(num_blocks * 2);
  }
}

void Device::allocateSendBuffer(size_t num_blocks) {
  if (send_buffer_.size() < num_blocks) {
    send_buffer_.resize(num_blocks);
  }
}

void Device::dataCallback(void* userdata, SDL_AudioStream* /*stream*/,
                          int additional_amount, int /*total_amount*/) {
  if (additional_amount == 0) {
    return;
  }
  auto* device = (Device*)userdata;
  device->fillMixBuffer(additional_amount);
  device->sendMixedToMainStream(additional_amount);
}

void Device::fillMixBuffer(int bytes_amount) {
  std::vector<std::shared_ptr<PlayingStream>> playing_streams_saved;
  std::vector<int32_t> gains;

  {
    std::lock_guard<std::mutex> lock(mutex_);

    playing_streams_saved.resize(playing_streams_.size());

    for (size_t i = 0; auto playing_stream : playing_streams_) {
      playing_streams_saved[i] = playing_stream;
      ++i;
    }

    gains.resize(playing_streams_saved.size());
    for (size_t i = 0; i < playing_streams_saved.size(); ++i) {
      PlayingStreamInternal* playing_stream_internal =
          static_cast<PlayingStreamInternal*>(playing_streams_saved[i].get());

      if (playing_stream_internal->stop_control.has_value()) {
        playing_stream_internal->stop_control_in_callback =
            playing_stream_internal->stop_control;
        playing_stream_internal->stop_control = std::nullopt;
      }

      // We apply gain to the whole buffer while it is very small.
      gains[i] = updateGainStateInCallback(playing_stream_internal);
    }
  }

  std::unordered_set<std::shared_ptr<PlayingStream>> to_delete;

  size_t num_requested_blocks = bytes_amount / (sizeof(StereoBlock16));

  allocateMixBuffer(num_requested_blocks);
  allocateReadBuffer(num_requested_blocks);

  for (size_t i = 0; i < num_requested_blocks; ++i) {
    mix_buffer_[i].left = 0;
    mix_buffer_[i].right = 0;
  }

  for (size_t playing_stream_index = 0;
       playing_stream_index < playing_streams_saved.size();
       ++playing_stream_index) {
    PlayingStreamInternal* playing_stream_internal =
        static_cast<PlayingStreamInternal*>(
            playing_streams_saved[playing_stream_index].get());

    size_t num_blocks_sent = 0;
    while (num_blocks_sent < num_requested_blocks) {
      bool reset_looped_blocks_streamed = false;

      size_t num_blocks_to_read = num_requested_blocks - num_blocks_sent;
      if (num_blocks_to_read + playing_stream_internal->looped_blocks_streamed >
          playing_stream_internal->wave_file->GetNumBlocks()) {
        num_blocks_to_read =
            playing_stream_internal->wave_file->GetNumBlocks() -
            playing_stream_internal->looped_blocks_streamed;

        reset_looped_blocks_streamed = true;

        playing_stream_internal->num_plays += 1;
      }

      const int16_t* read_buffer = 0;
      if (playing_stream_internal->wave_file->IsInMemory()) {
        read_buffer = playing_stream_internal->wave_file->GetBufferWhenInMemory(
            playing_stream_internal->looped_blocks_streamed);
      } else {
        playing_stream_internal->wave_file->ReadBlocks(
            playing_stream_internal->looped_blocks_streamed, num_blocks_to_read,
            &read_buffer_[0]);
        read_buffer = &read_buffer_[0];
      }

      playing_stream_internal->looped_blocks_streamed += num_blocks_to_read;
      playing_stream_internal->total_blocks_streamed += num_blocks_to_read;

      accumulateSamples(&mix_buffer_[num_blocks_sent],
                        gains[playing_stream_index],
                        playing_stream_internal->wave_file->GetNumChannels(),
                        read_buffer, num_blocks_to_read);
      num_blocks_sent += num_blocks_to_read;

      if (reset_looped_blocks_streamed) {
        playing_stream_internal->looped_blocks_streamed = 0;
      }

      // Has total_blocks_to_play specified, can stop playing when reached:
      if (playing_stream_internal->total_blocks_to_play) {
        if (playing_stream_internal->total_blocks_streamed >=
            playing_stream_internal->total_blocks_to_play) {
          to_delete.insert(playing_streams_saved[playing_stream_index]);
          break;
        }
      }
    }
  }

  for (size_t i = 0; i < num_requested_blocks; ++i) {
    mix_buffer_[i].left =
        std::clamp(mix_buffer_[i].left, kSampleMin16, kSampleMax16);
    mix_buffer_[i].right =
        std::clamp(mix_buffer_[i].right, kSampleMin16, kSampleMax16);
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto playing_stream_to_delete : to_delete) {
      playing_streams_.erase(playing_stream_to_delete);
    }
  }
}

void Device::sendMixedToMainStream(int bytes_amount) {
  size_t num_requested_blocks = bytes_amount / (sizeof(StereoBlock16));

  allocateSendBuffer(num_requested_blocks);

  for (size_t i = 0; i < num_requested_blocks; ++i) {
    send_buffer_[i].left = (int16_t)mix_buffer_[i].left;
    send_buffer_[i].right = (int16_t)mix_buffer_[i].right;
  }

  SDL_PutAudioStreamData(sdl_audio_stream_.get(), send_buffer_.data(),
                         bytes_amount);
}

}  // namespace Audio
}  // namespace Symphony
