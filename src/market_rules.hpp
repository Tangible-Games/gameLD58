#pragma once

#include <list>
#include <map>
#include <set>

namespace gameLD58 {
struct KnownHumanoid {
  std::map<std::string, std::string> traits;
};

struct KnownAlien {
  std::string name;
  std::map<std::string, std::set<std::string>> likes;
  int pays_when_likes{0};
  std::string says_when_likes;
  int pays_when_dislikes{0};
  std::string says_when_dislikes;
};

struct MarketRules {
  std::list<std::string> known_traits;
  std::list<KnownHumanoid> known_humanoids;
  std::vector<KnownAlien> known_aliens;
};

struct MarketInfo {
  std::list<KnownHumanoid> cur_captured_humanoids;
};

MarketRules LoadMarketRules() {
  MarketRules result;

  std::ifstream file;

  file.open("assets/market_rules.json");
  if (!file.is_open()) {
    LOGE("Failed to load {}", "assets/market_rules.json");
    return result;
  }

  nlohmann::json market_rules_json = nlohmann::json::parse(file);
  file.close();

  return result;
}

int MatchHumanoidWithAlien(const MarketRules& rules,
                           const KnownHumanoid& known_humanoid,
                           const KnownAlien& known_alien) {
  int result = 0;
  for (const auto& trait : rules.known_traits) {
    auto humanoid_trait_it = known_humanoid.traits.find(trait);
    if (humanoid_trait_it == known_humanoid.traits.end()) {
      // Shouldn't be here:
      continue;
    }

    const auto& humanoid_trait_value = humanoid_trait_it->second;

    auto alien_likes_it = known_alien.likes.find(trait);
    if (alien_likes_it == known_alien.likes.end()) {
      // Shouldn't be here:
      continue;
    }

    auto it = alien_likes_it->second.find(humanoid_trait_value);
    if (it != alien_likes_it->second.end()) {
      ++result;
    }
  }

  return result;
}
}  // namespace gameLD58
