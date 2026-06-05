#ifndef ZOMBIE_WAVE_CONTROLLER_HPP
#define ZOMBIE_WAVE_CONTROLLER_HPP

#include <random>
#include <string>
#include <utility>
#include <vector>

#include "WaveConfig.hpp"

// Flattened spawn event derived from one wave phase repeat.
// Stored in the ordered spawn plan built at level start.
struct ZombieWaveSpawnGroup {
  std::string phaseId;
  std::string phaseType;
  std::string zombieType = "basic";
  std::vector<std::string> zombieTypes;
  bool randomOrder = false;
  float earliestStartSec = 0.0F;
  int zombieCount = 0;
  float spawnIntervalSec = 0.0F;
  bool waitUntilClear = false;
};

// Manages the wave state machine: when to start each spawn group and how
// many zombies to emit per frame. Call Tick() every frame; act on the
// returned spawn requests in App.
class ZombieWaveController {
public:
  struct SpawnRequest {
    std::string zombieType;
    // Row is determined by the caller (App) via PickSpawnRow.
  };

  ZombieWaveController() = default;

  // Build the flat spawn plan from level config; resets all state.
  void Initialize(const LevelWaveConfig &config);

  void Reset();
  void Start() { m_Started = true; }
  bool IsStarted() const { return m_Started; }
  bool IsFinished() const;

  // Advance the state machine by @dt seconds.
  // @hasAliveZombie    true if any zombie is currently alive (for waitUntilClear).
  // @hugeBannerSec     seconds remaining on the huge-wave banner delay.
  // @outTriggerHuge    set to true if a huge-wave banner should be shown now.
  // @returns           List of zombie types to spawn this frame (may be empty).
  std::vector<SpawnRequest> Tick(float dt, bool hasAliveZombie,
                                 float hugeBannerSec, bool &outTriggerHuge);

  const std::vector<ZombieWaveSpawnGroup> &GetPlan() const { return m_Plan; }

private:
  static std::vector<ZombieWaveSpawnGroup>
  BuildPlan(const LevelWaveConfig &config);

  static std::vector<std::string>
  BuildTypeSequence(const ZombieWavePhaseConfig &phase);

  bool m_Started = false;
  float m_ElapsedSec = 0.0F;
  std::vector<ZombieWaveSpawnGroup> m_Plan;
  std::size_t m_CurrentGroupIndex = 0;
  bool m_GroupActive = false;
  int m_GroupSpawnedCount = 0;
  float m_GroupSpawnTimer = 0.0F;
  ZombieWaveSpawnGroup m_CurrentGroup;
};

#endif // ZOMBIE_WAVE_CONTROLLER_HPP
