#include "LevelConfig.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

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

    // Parse allowed plants for this level
    if (jsonData.contains("allowedPlants") && jsonData["allowedPlants"].is_array()) {
      config.allowedPlants = jsonData["allowedPlants"].get<std::vector<std::string>>();
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
        if (phaseJson.contains("zombieType")) {
          phase.zombieType = phaseJson["zombieType"].get<std::string>();
        }
        if (phaseJson.contains("zombieTypes") &&
            phaseJson["zombieTypes"].is_array()) {
          phase.zombieTypes =
              phaseJson["zombieTypes"].get<std::vector<std::string>>();
        }
        if (phaseJson.contains("randomOrder")) {
          phase.randomOrder = phaseJson["randomOrder"].get<bool>();
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

    if (!config.IsValid()) {
      config = GetDefaultConfig(config.levelId);
    }

    return config;

  } catch (const std::exception &e) {
    (void)e;
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
  defaultPhase.zombieType = "basic";
  defaultPhase.zombieTypes = {"basic"};
  defaultPhase.randomOrder = false;
  defaultPhase.startDelaySec = 5.0F;
  defaultPhase.repeat = 1;
  defaultPhase.zombiesPerWave = 1;
  defaultPhase.spawnIntervalSec = 0.0F;
  defaultPhase.waveIntervalSec = 0.0F;
  defaultPhase.waitUntilClear = false;

  config.phases.push_back(defaultPhase);

  config.allowedPlants = {"sunflower", "sunshroom", "puffshroom", "fumeshroom", "peashooter", "nut", "cherrybomb"};
  return config;
}
