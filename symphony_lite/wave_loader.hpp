#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "audio_utils.hpp"

namespace Symphony {
namespace {
struct FourCC {
  char code[4];
};

size_t GetValue16(const uint8_t bytes[]) {
  size_t s0 = bytes[0];
  size_t s1 = bytes[1];
  return (s0 << 0) + (s1 << 8);
}

size_t GetValue32(const uint8_t bytes[]) {
  size_t s0 = bytes[0];
  size_t s1 = bytes[1];
  size_t s2 = bytes[2];
  size_t s3 = bytes[3];
  return (s0 << 0) + (s1 << 8) + (s2 << 16) + (s3 << 24);
}

struct RiffChunkHeader {
  FourCC four_cc;
  uint8_t chunk_size[4];

  bool TestChunk(const char* code) const {
    return four_cc.code[0] == code[0] && four_cc.code[1] == code[1] &&
           four_cc.code[2] == code[2] && four_cc.code[3] == code[3];
  }

  std::string GetFourCC() const {
    std::string result;
    result.resize(4);
    result[0] = four_cc.code[0];
    result[1] = four_cc.code[1];
    result[2] = four_cc.code[2];
    result[3] = four_cc.code[3];
    return result;
  }

  size_t GetSize() const { return GetValue32(chunk_size); }
};
}  // namespace

namespace Audio {
enum WaveFormatCategory {
  kWaveFormatPcm = 1,
};

struct WaveFormatCommonFields {
  uint8_t format_category[2];
  uint8_t channels[2];
  uint8_t sample_rate[4];
  uint8_t byte_rate[4];
  uint8_t block_align[2];

  size_t GetFormatCategory() const { return GetValue16(format_category); }

  size_t GetNumChannels() const { return GetValue16(channels); }

  size_t GetSampleRate() const { return GetValue32(sample_rate); }

  size_t GetByteRate() const { return GetValue32(byte_rate); }

  size_t GetBlockAlign() const { return GetValue16(block_align); }
};

struct WaveFormatPCMFields {
  uint8_t bits_per_sample[2];

  size_t GetBitsPerSample() const { return GetValue16(bits_per_sample); }
};

class WaveFile {
 public:
  enum Mode {
    kModeStreamingFromFile = 1,
    kModeLoadInMemory = 2,
  };

  WaveFile() = default;
  WaveFile(std::string& file_path, Mode mode) { Load(file_path, mode); }

  bool Load(const std::string& file_path, Mode mode);

  const WaveFormatCommonFields& GetFormatCommonFileds() const {
    return format_common_;
  }
  const WaveFormatPCMFields& GetFormatPCMFields() const { return format_pcm_; }

  size_t GetNumBlocks() const { return wave_data_size_ / GetBlockSize(); }

  size_t GetBlockSize() const { return format_common_.GetBlockAlign(); }

  size_t GetNumChannels() const { return format_common_.GetNumChannels(); }

  float GetLengthSec() const {
    return (float)GetNumBlocks() / (float)format_common_.GetSampleRate();
  }

  void ReadBlocks(size_t first_block, size_t num_blocks, float* blocks_out);

 private:
  void convertToFloat(const std::vector<char>& samples_in,
                      float* samples_out) const;

  std::ifstream file_;
  WaveFormatCommonFields format_common_;
  WaveFormatPCMFields format_pcm_;
  size_t wave_data_offset_{0};
  size_t wave_data_size_{0};
  std::vector<float> wave_data_;
};

bool WaveFile::Load(const std::string& file_path, WaveFile::Mode mode) {
  std::ifstream file;

  file.open(file_path, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "[Symphony::Audio::WaveFile] Can't open file, file_path: "
              << file_path << std::endl;
    return false;
  }

  RiffChunkHeader riff;
  file.read((char*)&riff, sizeof(RiffChunkHeader));
  if (!riff.TestChunk("RIFF")) {
    std::cerr << "[Symphony::Audio::WaveFile] Not a RIFF file, file_path: "
              << file_path << std::endl;
    return false;
  }

  size_t bytes_to_scan = riff.GetSize();
  if (bytes_to_scan < 4) {
    std::cerr << "[Symphony::Audio::WaveFile] File too small, file_path: "
              << file_path << std::endl;
    return false;
  }

  char format_id[4];
  file.read(format_id, 4);
  if (format_id[0] != 'W' || format_id[1] != 'A' || format_id[2] != 'V' ||
      format_id[3] != 'E') {
    std::cerr << "[Symphony::Audio::WaveFile] Not a WAVE file, file_path: "
              << file_path << std::endl;
    return false;
  }

  bytes_to_scan -= 4;

  bool format_read = false;
  bool wave_data_read = false;
  while (bytes_to_scan >= sizeof(RiffChunkHeader)) {
    RiffChunkHeader chunk;
    file.read((char*)&chunk, sizeof(RiffChunkHeader));
    bytes_to_scan -= sizeof(RiffChunkHeader);

    if (bytes_to_scan < chunk.GetSize()) {
      std::cerr
          << "[Symphony::Audio::WaveFile] File is misconfigured, file_path: "
          << file_path << std::endl;
      return false;
    }

    if (chunk.TestChunk("fmt ")) {
      size_t fmt_bytes_read = 0;

      file.read((char*)&format_common_, sizeof(WaveFormatCommonFields));
      fmt_bytes_read += sizeof(WaveFormatCommonFields);

      if (format_common_.GetFormatCategory() != kWaveFormatPcm) {
        std::cerr << "[Symphony::Audio::WaveFile] Format is not supported, "
                     "file_path: "
                  << file_path << std::endl;
        return false;
      }

      file.read((char*)&format_pcm_, sizeof(WaveFormatPCMFields));
      fmt_bytes_read += sizeof(WaveFormatPCMFields);

      if (fmt_bytes_read < chunk.GetSize()) {
        file.seekg(chunk.GetSize() - fmt_bytes_read, std::ios::cur);
      }

      if (format_pcm_.GetBitsPerSample() != 8 &&
          format_pcm_.GetBitsPerSample() != 16) {
        std::cerr << "[Symphony::Audio::WaveFile] Only 8 and 16 bits per "
                     "sample formats are supported, file_path: "
                  << file_path << std::endl;
        return false;
      }

      format_read = true;
    } else if (chunk.TestChunk("data")) {
      if (!format_read) {
        std::cerr << "[Symphony::Audio::WaveFile] Data chunk should follow "
                     "format chunk, file_path: "
                  << file_path << std::endl;
        return false;
      }

      wave_data_offset_ = file.tellg();
      wave_data_size_ = chunk.GetSize();

      file.seekg(chunk.GetSize(), std::ios::cur);

      wave_data_read = true;
    } else {
      file.seekg(chunk.GetSize(), std::ios::cur);
    }

    if (!file.good()) {
      std::cerr << "[Symphony::Audio::WaveFile] Something is wrong with "
                   "reading file, file_path: "
                << file_path << std::endl;
      return false;
    }

    bytes_to_scan -= chunk.GetSize();
  }

  if (!format_read || !wave_data_read) {
    std::cerr << "[Symphony::Audio::WaveFile] File doesn't contain needed "
                 "chunks, file_path: "
              << file_path << std::endl;
    return false;
  }

  if (mode == kModeLoadInMemory) {
    std::vector<char> samples_in(wave_data_size_);

    file.seekg(wave_data_offset_, std::ios::beg);
    file.read(&samples_in[0], wave_data_size_);

    wave_data_.resize(GetNumBlocks() * format_common_.GetNumChannels());
    convertToFloat(samples_in, &wave_data_[0]);
  } else {
    file_ = std::move(file);
  }

  return true;
}

void WaveFile::ReadBlocks(size_t first_block, size_t num_blocks,
                          float* blocks_out) {
  size_t block_size = GetBlockSize();
  if (!wave_data_.empty()) {
    memcpy(blocks_out, wave_data_.data() + first_block * sizeof(float),
           num_blocks * sizeof(float));
  } else {
    file_.seekg(wave_data_offset_ + first_block * block_size, std::ios::beg);

    static std::vector<char> samples_in;

    samples_in.resize(num_blocks * block_size);
    file_.read(&samples_in[0], samples_in.size());

    convertToFloat(samples_in, blocks_out);
  }
}

void WaveFile::convertToFloat(const std::vector<char>& blocks_in,
                              float* blocks_out) const {
  size_t num_blocks = blocks_in.size() / GetBlockSize();
  size_t num_channels = format_common_.GetNumChannels();

  if (format_pcm_.GetBitsPerSample() == 8) {
    const uint8_t* blocks_in_typed = (const uint8_t*)blocks_in.data();
    for (size_t i = 0; i < num_blocks * num_channels; ++i) {
      blocks_out[i] = ConvertPcmToFloat<uint8_t>(blocks_in_typed[i]);
    }
  } else if (format_pcm_.GetBitsPerSample() == 16) {
    const int16_t* blocks_in_typed = (const int16_t*)blocks_in.data();
    for (size_t i = 0; i < num_blocks * num_channels; ++i) {
      blocks_out[i] = ConvertPcmToFloat<int16_t>(blocks_in_typed[i]);
    }
  }
}

std::shared_ptr<WaveFile> LoadWave(const std::string& file_path,
                                   WaveFile::Mode mode) {
  std::shared_ptr<WaveFile> result(new WaveFile());
  if (!result->Load(file_path, mode)) {
    result.reset();
  }
  return result;
}

}  // namespace Audio
}  // namespace Symphony
