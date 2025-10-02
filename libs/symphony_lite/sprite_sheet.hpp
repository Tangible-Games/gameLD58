#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "log.hpp"

namespace Symphony {
namespace Sprite {

struct SpriteFrame {
  std::string filename;

  int x = 0, y = 0, w = 0, h = 0;

  bool rotated = false;
  bool trimmed = false;

  int sss_x = 0, sss_y = 0, sss_w = 0, sss_h = 0;  // sprite source size
  int src_w = 0, src_h = 0;                        // source size
};

class SpriteSheet {
 public:
  SpriteSheet(SDL_Renderer* renderer, const std::string& dirPath,
              const std::string& jsonFile) {
    load(renderer, dirPath, jsonFile);
  }

  ~SpriteSheet() {
    if (atlas_) {
      SDL_DestroyTexture(atlas_);
      atlas_ = nullptr;
    }
  }

  std::optional<const SpriteFrame*> GetFrame(const std::string& anim,
                                             size_t frame_idx) const {
    auto it = anim_to_indices_.find(anim);
    if (it == anim_to_indices_.end()) return std::nullopt;
    const auto& indices = it->second;
    if (frame_idx >= indices.size()) return std::nullopt;
    size_t idx = indices[frame_idx];
    if (idx >= frames_.size()) return std::nullopt;
    return &frames_[idx];
  }

  SDL_Texture* GetAtlas() const { return atlas_; }

  const std::vector<size_t>& GetAnimIndices(const std::string& anim) const {
    static const std::vector<size_t> empty;
    auto it = anim_to_indices_.find(anim);
    return (it == anim_to_indices_.end()) ? empty : it->second;
  }

 private:
  std::vector<SpriteFrame> frames_;
  SDL_Texture* atlas_ = nullptr;
  std::unordered_map<std::string, std::vector<size_t>> anim_to_indices_;

 private:
  static std::string readFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return {};
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
  }

  static std::string normalizeAnimName(const std::filesystem::path& p) {
    auto parent = p.parent_path().generic_string();
    if (parent.empty()) return "";
    return parent;
  }

  void load(SDL_Renderer* renderer, const std::string& dirPath,
            const std::string& jsonFile) {
    const std::string full_json_path = dirPath + "/" + jsonFile;
    const auto text = readFile(full_json_path);
    if (text.empty()) {
      LOGE("[Symphony::Sprite::SpriteSheet]: cannot read file '{}'",
           full_json_path);
      return;
    }

    nlohmann::json sprite_json = nlohmann::json::parse(text, nullptr, false);
    if (sprite_json.is_discarded()) {
      LOGE("[Symphony::Sprite::SpriteSheet]: cannot parse JSON from file '{}'",
           full_json_path);
      return;
    }

    const auto& jframes = sprite_json["frames"];
    frames_.reserve(jframes.size());

    size_t index = 0;
    for (const auto& jf : jframes) {
      SpriteFrame sf;

      sf.filename = jf.value("filename", std::string{});

      if (jf.contains("frame") && jf["frame"].is_object()) {
        const auto& fr = jf["frame"];
        sf.x = fr.value("x", 0);
        sf.y = fr.value("y", 0);
        sf.w = fr.value("w", 0);
        sf.h = fr.value("h", 0);
      }

      sf.rotated = jf.value("rotated", false);
      sf.trimmed = jf.value("trimmed", false);

      if (jf.contains("spriteSourceSize") &&
          jf["spriteSourceSize"].is_object()) {
        const auto& sss = jf["spriteSourceSize"];
        sf.sss_x = sss.value("x", 0);
        sf.sss_y = sss.value("y", 0);
        sf.sss_w = sss.value("w", 0);
        sf.sss_h = sss.value("h", 0);
      }

      if (jf.contains("sourceSize") && jf["sourceSize"].is_object()) {
        const auto& ss = jf["sourceSize"];
        sf.src_w = ss.value("w", 0);
        sf.src_h = ss.value("h", 0);
      }

      frames_.push_back(sf);

      if (!sf.filename.empty()) {
        std::filesystem::path fpath(sf.filename);
        const std::string animName = normalizeAnimName(fpath);
        anim_to_indices_[animName].push_back(index);
      }

      ++index;
    }

    std::string atlas_file;
    if (sprite_json.contains("meta") && sprite_json["meta"].is_object()) {
      const auto& meta = sprite_json["meta"];
      atlas_file = meta.value("image", std::string{});
    }

    const std::string atlas_path = dirPath + "/" + atlas_file;
    atlas_ = IMG_LoadTexture(renderer, atlas_path.c_str());
    if (!atlas_) {
      LOGE(
          "[Symphony::Sprite::SpriteSheet] Failed to load atlas texture '{}', "
          "error: {}",
          atlas_path, SDL_GetError());
      return;
    }
  }
};

}  // namespace Sprite
}  // namespace Symphony
