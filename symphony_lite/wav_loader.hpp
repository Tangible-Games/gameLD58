#pragma once

#include <fstream>
#include <string>

namespace Symphony {
namespace {
struct FourCC {
  char code[4];
};

struct RiffChunkHeader {
  FourCC four_cc;
  uint32_t chunk_size;
};

bool TestRiffChunk(RiffChunkHeader& chunk, const char* code) {
  return chunk.four_cc.code[0] == code[0] && chunk.four_cc.code[1] == code[1] &&
         chunk.four_cc.code[2] == code[2] && chunk.four_cc.code[3] == code[3];
};
}  // namespace

namespace Audio {
class WavLoader {
 public:
  enum Mode {
    kModeStreamingFromFile = 1,
    kModeLoadInMemory = 2,
  };

  WavLoader() = default;
  WavLoader(std::string& file_path, Mode mode) { Load(file_path, mode); }

  bool Load(std::string& file_path, Mode mode);

 private:
};

bool WavLoader::Load(std::string& file_path, WavLoader::Mode /*mode*/) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  RiffChunkHeader riff;
  file.read((char*)&riff, sizeof(RiffChunkHeader));
  if (file.bad()) {
    return false;
  }
  if (!TestRiffChunk(riff, "WAVE")) {
    return false;
  }

  return true;
}

}  // namespace Audio
}  // namespace Symphony
