#pragma once

#include <SDL2/SDL.h>

#include <iostream>
#include <unordered_set>
#include <vector>

#include "wave_loader.hpp"

namespace Symphony {
namespace Audio {
struct PlayCount {
  int num_repeats{1};
  bool loop_infinite{false};
};

static constexpr PlayCount kPlayLooped{.num_repeats = 0, .loop_infinite = true};
static constexpr PlayCount kPlayOnce{.num_repeats = 1, .loop_infinite = false};
static PlayCount PlayTimes(int num_repeats) {
  return PlayCount{.num_repeats = num_repeats, .loop_infinite = false};
}

struct FadeControl {
  float fade_in_time_sec{0};
  float fade_out_time_sec{0};
};

static constexpr FadeControl kNoFade{.fade_in_time_sec = 0,
                                     .fade_out_time_sec = 0};

class Device;

class PlayingStream {
 public:
  PlayingStream() = default;
  virtual ~PlayingStream() {}

  virtual bool IsPlaying() const = 0;

 private:
  friend Device;
};

class Device {
 public:
  Device() = default;

  void Init();

  std::shared_ptr<PlayingStream> Play(
      std::shared_ptr<WaveFile> wave_file, const PlayCount& play_count,
      const FadeControl& fade_control = kNoFade);

  void Stop(std::shared_ptr<PlayingStream> playing_stream);

  void Update(float dt);

 private:
  struct PlayingStreamInternal : public PlayingStream {
   public:
    PlayingStreamInternal(std::shared_ptr<WaveFile> new_wave_file,
                          const PlayCount& new_play_count,
                          const FadeControl& new_fade_control)
        : wave_file(new_wave_file),
          play_count(new_play_count),
          fade_control(new_fade_control) {}

    bool IsPlaying() const override { return is_playing; }

    std::shared_ptr<WaveFile> wave_file;
    PlayCount play_count;
    int num_plays{0};
    FadeControl fade_control;
    bool is_playing{false};
    bool ready_to_stop{false};
    size_t samples_streamed{0};
  };

  static void dataCallback(void* userdata, Uint8* stream, int len);
  void onDataRequested(Uint8* stream, int len) const;

  SDL_AudioDeviceID sdl_audio_device_;
  std::unordered_set<std::shared_ptr<PlayingStream>> playing_streams_;
  unsigned int block_size_{0};
};

void Device::Init() {
  SDL_AudioSpec sdl_audio_spec;
  SDL_AudioSpec sdl_audio_spec_received;

  sdl_audio_spec.freq = 22050;
  sdl_audio_spec.format = AUDIO_S16LSB;
  sdl_audio_spec.channels = 2;
  sdl_audio_spec.samples = 1024;
  sdl_audio_spec.padding = 0;
  sdl_audio_spec.callback = dataCallback;
  sdl_audio_spec.userdata = this;

  sdl_audio_device_ =
      SDL_OpenAudioDevice(/*device=*/nullptr, /*iscapture=*/0, &sdl_audio_spec,
                          &sdl_audio_spec_received, 0);
  if (sdl_audio_device_ == 0) {
    std::cerr << "[Symphony::Audio::Device] Failed to create device"
              << std::endl;
  }

  block_size_ = 4;

  SDL_PauseAudioDevice(sdl_audio_device_, 0);
}

std::shared_ptr<PlayingStream> Device::Play(std::shared_ptr<WaveFile> wave_file,
                                            const PlayCount& play_count,
                                            const FadeControl& fade_control) {
  std::shared_ptr<PlayingStream> playing_stream(
      new PlayingStreamInternal(wave_file, play_count, fade_control));
  PlayingStreamInternal* playing_stream_internal =
      (PlayingStreamInternal*)playing_stream.get();

  playing_stream_internal->is_playing = true;
  playing_stream_internal->ready_to_stop = false;

  SDL_LockAudioDevice(sdl_audio_device_);
  playing_streams_.insert(playing_stream);
  SDL_UnlockAudioDevice(sdl_audio_device_);

  return playing_stream;
}

void Device::Stop(std::shared_ptr<PlayingStream> playing_stream) {
  SDL_LockAudioDevice(sdl_audio_device_);
  playing_streams_.erase(playing_stream);
  SDL_UnlockAudioDevice(sdl_audio_device_);
}

void Device::Update(float dt) { (void)dt; }

void Device::dataCallback(void* userdata, Uint8* stream, int len) {
  Device* device = (Device*)userdata;
  device->onDataRequested(stream, len);
}

void Device::onDataRequested(Uint8* stream, int len) const {
  size_t num_requested_samples = len / block_size_;
  size_t num_samples_sent = 0;

  for (auto playing_stream : playing_streams_) {
    PlayingStreamInternal* playing_stream_internal =
        (PlayingStreamInternal*)playing_stream.get();

    if (!playing_stream_internal->ready_to_stop) {
      while (num_samples_sent < num_requested_samples) {
        bool reset_samples_streamed = false;

        size_t num_samples_to_read = num_requested_samples;
        if (num_requested_samples + playing_stream_internal->samples_streamed >
            playing_stream_internal->wave_file->GetNumBlocks()) {
          num_samples_to_read =
              playing_stream_internal->wave_file->GetNumBlocks() -
              playing_stream_internal->samples_streamed;

          reset_samples_streamed = true;

          playing_stream_internal->num_plays += 1;
        }

        std::vector<float> blocks(
            num_samples_to_read *
            playing_stream_internal->wave_file->GetNumChannels());
        playing_stream_internal->wave_file->ReadBlocks(
            playing_stream_internal->samples_streamed, num_samples_to_read,
            &blocks[0]);

        int16_t* stream_types = (int16_t*)stream;
        for (size_t i = 0;
             i < num_samples_to_read *
                     playing_stream_internal->wave_file->GetNumChannels();
             ++i) {
          stream_types[num_samples_sent * 2 + i] =
              (int16_t)(blocks[i] * 65535.0f);
        }

        playing_stream_internal->samples_streamed += num_samples_to_read;
        num_samples_sent += num_samples_to_read;

        if (reset_samples_streamed) {
          playing_stream_internal->samples_streamed = 0;
        }

        if (!playing_stream_internal->play_count.loop_infinite) {
          if (playing_stream_internal->num_plays >=
              playing_stream_internal->play_count.num_repeats) {
            playing_stream_internal->ready_to_stop = true;
            break;
          }
        }
      }
    } else {
      playing_stream_internal->is_playing = false;
    }

    for (size_t i = num_samples_sent; i < num_requested_samples; ++i) {
      for (size_t j = 0; j < block_size_; ++j) {
        stream[(num_samples_sent + i) * block_size_ + j] = 0;
      }
    }

    // Now can play only one sound.
    break;
  }
}

}  // namespace Audio
}  // namespace Symphony
