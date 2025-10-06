#pragma once

#include <list>
#include <map>
#include <nlohmann/json.hpp>
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
  std::vector<KnownHumanoid> known_humanoids;
  std::vector<KnownAlien> known_aliens;
};

struct PlayerStatus {
  int credits_earned{0};
  int credits_earned_of{0};
  int humans_captured{0};
  int levels_completed{0};
  int best_price{0};
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

  for (const auto& trait_json : market_rules_json["known_traits"]) {
    result.known_traits.push_back(trait_json.get<std::string>());
  }

  for (int index = 0;
       const auto& known_humanoid_json : market_rules_json["known_humanoids"]) {
    result.known_humanoids.push_back(KnownHumanoid());

    for (const auto& trait : result.known_traits) {
      if (known_humanoid_json[trait].empty()) {
        LOGE("Trait {} not set for humanoid with index {}", trait, index);
      }

      result.known_humanoids.back().traits.insert(
          std::make_pair(trait, known_humanoid_json[trait]));
    }

    ++index;
  }

  for (const auto& known_alien_json : market_rules_json["known_aliens"]) {
    result.known_aliens.push_back(KnownAlien());

    result.known_aliens.back().name = known_alien_json["name"];
    result.known_aliens.back().pays_when_likes =
        known_alien_json.value("pays_when_likes", 0);
    result.known_aliens.back().says_when_likes =
        known_alien_json.value("says_when_likes", "");
    result.known_aliens.back().pays_when_dislikes =
        known_alien_json.value("pays_when_dislikes", 0);
    result.known_aliens.back().says_when_dislikes =
        known_alien_json.value("says_when_dislikes", "");

    for (const auto& trait : result.known_traits) {
      for (const auto& trait_value_json : known_alien_json["likes"][trait]) {
        result.known_aliens.back().likes[trait].insert(
            trait_value_json.get<std::string>());
      }
    }
  }

  LOGD("Finished loading Market rules.");
  LOGD("Known traits:");
  for (const auto& trait : result.known_traits) {
    LOGD("\t- {}", trait);
  }

  LOGD("Known humanoids:");
  for (int index = 0; auto& known_humanoid : result.known_humanoids) {
    LOGD("\t- {}", index);
    for (const auto& trait : result.known_traits) {
      LOGD("\t\t- {}: {}", trait, known_humanoid.traits[trait]);
    }
    ++index;
  }

  LOGD("Known aliens:");
  for (auto& known_alien : result.known_aliens) {
    LOGD("\t- {}", known_alien.name);
    LOGD("\t\t- pays_when_likes: {}", known_alien.pays_when_likes);
    LOGD("\t\t- says_when_likes: {}", known_alien.says_when_likes);
    LOGD("\t\t- pays_when_dislikes: {}", known_alien.pays_when_dislikes);
    LOGD("\t\t- says_when_dislikes: {}", known_alien.says_when_dislikes);
    LOGD("\t\t- likes:");
    for (const auto& trait : result.known_traits) {
      LOGD("\t\t\t- {}:", trait);
      for (auto& trait_value : known_alien.likes[trait]) {
        LOGD("\t\t\t\t- {}", trait_value);
      }
    }
  }

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

std::list<KnownHumanoid> CaptureRandomHumanoids(const MarketRules& rules,
                                                size_t amount) {
  std::list<KnownHumanoid> result;
  for (size_t i = 0; i < amount; ++i) {
    int rand_index = rand() % rules.known_humanoids.size();
    result.push_back(rules.known_humanoids[rand_index]);
  }

  return result;
}
}  // namespace gameLD58
