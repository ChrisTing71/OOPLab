#include "LevelConfig.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

bool LevelConfig::IsValid() const {
  if (levelId < 1 || levelId > 10) {
    return false;
  }
  if (phases.empty()) {
    return false;
  }
  return true;
}

LevelConfig LevelConfigLoader::LoadFromFile(const std::string &filePath) {
  LevelConfig config;

  try {
    std::ifstream file(filePath);
    if (!file.is_open()) {
      spdlog::warn("Failed to open level config file: {}", filePath);
      return config;
    }

    json jsonData;
    file >> jsonData;
    file.close();

    // Parse basic level info
    if (jsonData.contains("levelId")) {
      config.levelId = jsonData["levelId"].get<int>();
    }
    if (jsonData.contains("name")) {
      config.name = jsonData["name"].get<std::string>();
    }
    if (jsonData.contains("description")) {
      config.description = jsonData["description"].get<std::string>();
    }
    if (jsonData.contains("difficultyBadge")) {
      config.difficultyBadge = jsonData["difficultyBadge"].get<std::string>();
    }

    // Parse scene type
    if (jsonData.contains("sceneType")) {
      std::string sceneTypeStr = jsonData["sceneType"].get<std::string>();
      if (sceneTypeStr == "night") {
        config.sceneType = SceneType::NIGHT;
      } else {
        config.sceneType = SceneType::DAY;
      }
    }

    // Parse initial sun amount
    if (jsonData.contains("initialSunAmount")) {
      config.initialSunAmount = jsonData["initialSunAmount"].get<int>();
    }

    // Parse minimum defense points (sun needed for reward)
    if (jsonData.contains("minDefensePoints")) {
      config.minDefensePoints = jsonData["minDefensePoints"].get<int>();
    }

    // Parse zombie wave phases
    if (jsonData.contains("phases")) {
      for (const auto &phaseJson : jsonData["phases"]) {
        ZombieWavePhaseConfig phase;

        if (phaseJson.contains("id")) {
          phase.id = phaseJson["id"].get<std::string>();
        }
        if (phaseJson.contains("type")) {
          phase.type = phaseJson["type"].get<std::string>();
        }
        if (phaseJson.contains("startDelaySec")) {
          phase.startDelaySec = phaseJson["startDelaySec"].get<float>();
        }
        if (phaseJson.contains("repeat")) {
          phase.repeat = phaseJson["repeat"].get<int>();
        }
        if (phaseJson.contains("zombiesPerWave")) {
          phase.zombiesPerWave = phaseJson["zombiesPerWave"].get<int>();
        }
        if (phaseJson.contains("spawnIntervalSec")) {
          phase.spawnIntervalSec = phaseJson["spawnIntervalSec"].get<float>();
        }
        if (phaseJson.contains("waveIntervalSec")) {
          phase.waveIntervalSec = phaseJson["waveIntervalSec"].get<float>();
        }
        if (phaseJson.contains("waitUntilClear")) {
          phase.waitUntilClear = phaseJson["waitUntilClear"].get<bool>();
        }

        config.phases.push_back(phase);
      }
    }

    // Parse rewards
    if (jsonData.contains("reward")) {
      const auto &rewardJson = jsonData["reward"];
      if (rewardJson.contains("unlockedPlants")) {
        config.reward.unlockedPlants =
            rewardJson["unlockedPlants"].get<std::vector<std::string>>();
      }
      if (rewardJson.contains("sunAmountReward")) {
        config.reward.sunAmountReward =
            rewardJson["sunAmountReward"].get<int>();
      }
    }

    if (config.IsValid()) {
      spdlog::info("Loaded level config from: {} (Level {})", filePath,
                   config.levelId);
    } else {
      spdlog::warn("Level config from {} is invalid, using defaults", filePath);
      config = GetDefaultConfig(config.levelId);
    }

    return config;

  } catch (const std::exception &e) {
    spdlog::error("Exception loading level config from {}: {}", filePath,
                  e.what());
    return GetDefaultConfig(1);
  }
}

LevelConfig LevelConfigLoader::GetDefaultConfig(int levelId) {
  LevelConfig config;
  config.levelId = levelId;
  config.name = "Level " + std::to_string(levelId);
  config.sceneType = SceneType::DAY;
  config.initialSunAmount = 50;
  config.difficultyBadge = "Easy";

  // Default single-wave configuration
  ZombieWavePhaseConfig defaultPhase;
  defaultPhase.id = "default_wave";
  defaultPhase.type = "sub";
  defaultPhase.startDelaySec = 5.0F;
  defaultPhase.repeat = 1;
  defaultPhase.zombiesPerWave = 1;
  defaultPhase.spawnIntervalSec = 0.0F;
  defaultPhase.waveIntervalSec = 0.0F;
  defaultPhase.waitUntilClear = false;

  config.phases.push_back(defaultPhase);

  spdlog::info("Using default level config for level {}", levelId);

  return config;
}
