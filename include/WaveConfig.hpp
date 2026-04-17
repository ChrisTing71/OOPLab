#ifndef WAVE_CONFIG_HPP
#define WAVE_CONFIG_HPP

#include <string>
#include <vector>

struct ZombieWavePhaseConfig {
  std::string id;
  std::string type;
  std::string zombieType = "basic";
  std::vector<std::string> zombieTypes;
  bool randomOrder = false;
  float startDelaySec = 0.0F;
  int repeat = 1;
  int zombiesPerWave = 1;
  float spawnIntervalSec = 0.0F;
  float waveIntervalSec = 0.0F;
  bool waitUntilClear = false;
};

struct LevelWaveConfig {
  std::string levelId;
  std::vector<ZombieWavePhaseConfig> phases;
};

class WaveConfigLoader {
public:
  static bool LoadFromFile(const std::string &path, LevelWaveConfig &outConfig,
                           std::string &outError);
};

#endif