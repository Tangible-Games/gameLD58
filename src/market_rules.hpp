#pragma once

#include <map>

namespace gameLD58 {
struct CapturedHumanoid {
  std::map<std::string, std::string> traits;
};

struct MarketInfo {
  std::vector<CapturedHumanoid> cur_captured_humanoids;
};
}  // namespace gameLD58
