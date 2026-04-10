#include "WaveConfig.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

namespace {
bool ValidatePhase(const ZombieWavePhaseConfig &phase, std::string &error) {
  if (phase.id.empty()) {
    error = "phase id is empty";
    return false;
  }
  if (phase.type.empty()) {
    error = "phase type is empty";
    return false;
  }
  if (phase.startDelaySec < 0.0F) {
    error = "phase startDelaySec cannot be negative";
    return false;
  }
  if (phase.repeat <= 0) {
    error = "phase repeat must be greater than 0";
    return false;
  }
  if (phase.zombiesPerWave <= 0) {
    error = "phase zombiesPerWave must be greater than 0";
    return false;
  }
  if (phase.spawnIntervalSec < 0.0F) {
    error = "phase spawnIntervalSec cannot be negative";
    return false;
  }
  if (phase.waveIntervalSec < 0.0F) {
    error = "phase waveIntervalSec cannot be negative";
    return false;
  }
  return true;
}
} // namespace

bool WaveConfigLoader::LoadFromFile(const std::string &path,
                                    LevelWaveConfig &outConfig,
                                    std::string &outError) {
  std::ifstream file(path);
  if (!file.is_open()) {
    outError = "failed to open wave config: " + path;
    return false;
  }

  nlohmann::json root;
  try {
    file >> root;
  } catch (const nlohmann::json::parse_error &e) {
    outError = std::string("wave config parse error: ") + e.what();
    return false;
  }

  if (!root.contains("levelId") || !root["levelId"].is_string()) {
    outError = "wave config missing string field: levelId";
    return false;
  }
  if (!root.contains("phases") || !root["phases"].is_array()) {
    outError = "wave config missing array field: phases";
    return false;
  }

  LevelWaveConfig parsed;
  parsed.levelId = root["levelId"].get<std::string>();

  for (const auto &phaseJson : root["phases"]) {
    ZombieWavePhaseConfig phase;
    if (!phaseJson.contains("id") || !phaseJson["id"].is_string()) {
      outError = "phase missing string field: id";
      return false;
    }
    if (!phaseJson.contains("type") || !phaseJson["type"].is_string()) {
      outError = "phase missing string field: type";
      return false;
    }

    phase.id = phaseJson["id"].get<std::string>();
    phase.type = phaseJson["type"].get<std::string>();
    phase.startDelaySec = phaseJson.value("startDelaySec", 0.0F);
    phase.repeat = phaseJson.value("repeat", 1);
    phase.zombiesPerWave = phaseJson.value("zombiesPerWave", 1);
    phase.spawnIntervalSec = phaseJson.value("spawnIntervalSec", 0.0F);
    phase.waveIntervalSec = phaseJson.value("waveIntervalSec", 0.0F);
    phase.waitUntilClear = phaseJson.value("waitUntilClear", false);

    std::string validationError;
    if (!ValidatePhase(phase, validationError)) {
      outError = "invalid phase '" + phase.id + "': " + validationError;
      return false;
    }

    parsed.phases.push_back(phase);
  }

  if (parsed.phases.empty()) {
    outError = "wave config has no phases";
    return false;
  }

  outConfig = parsed;
  return true;
}