#ifndef LEVELCONFIG_HPP
#define LEVELCONFIG_HPP

#include <string>
#include <vector>

#include "WaveConfig.hpp" // Use ZombieWavePhaseConfig from WaveConfig

// Scene lighting type
enum class SceneType {
  DAY,   // Bright daytime
  NIGHT, // Dark nighttime
};

// Reward structure for passing a level
struct LevelReward {
  std::vector<std::string>
      unlockedPlants;      // Plant IDs unlocked (e.g., "peashooter", "squash")
  int sunAmountReward = 0; // Bonus sun points for completing level
};

// Complete level configuration
struct LevelConfig {
  int levelId = 0;         // Unique level identifier (1-10)
  std::string name;        // Display name (e.g., "Level 1 - Beginning")
  std::string description; // Level description
  SceneType sceneType = SceneType::DAY; // Lighting scenario
  int initialSunAmount = 50;            // Starting sun points for the level
  std::vector<ZombieWavePhaseConfig>
      phases;               // Wave phase definitions (reusing from WaveConfig)
  LevelReward reward;       // Unlock rewards
  int minDefensePoints = 0; // Minimum sun/resources to earn reward
  std::string difficultyBadge = "Easy"; // Visual difficulty indicator

  // Validation
  bool IsValid() const;
};

// Service class for loading and managing level configurations
class LevelConfigLoader {
public:
  /**
   * Load level configuration from a JSON file
   * @param filePath Path to the level JSON file (e.g.,
   * "Resources/levels/level1.json")
   * @return Loaded LevelConfig, populated with valid data or defaults
   */
  static LevelConfig LoadFromFile(const std::string &filePath);

  /**
   * Get fallback/default configuration for a specific level ID
   * @param levelId Level identifier (1-10)
   * @return Default LevelConfig for the level
   */
  static LevelConfig GetDefaultConfig(int levelId);
};

#endif // LEVELCONFIG_HPP
