#include "ZombieWaveController.hpp"

#include <algorithm>
#include <random>

// ── Plan building ────────────────────────────────────────────────────────────

std::vector<std::string>
ZombieWaveController::BuildTypeSequence(const ZombieWavePhaseConfig &phase) {
  std::vector<std::string> seq;
  seq.reserve(static_cast<std::size_t>(phase.zombiesPerWave));

  if (!phase.zombieTypes.empty()) {
    for (int i = 0; i < phase.zombiesPerWave; ++i) {
      seq.push_back(
          phase.zombieTypes[static_cast<std::size_t>(i) %
                            phase.zombieTypes.size()]);
    }
  } else {
    const std::string t = phase.zombieType.empty() ? "basic" : phase.zombieType;
    for (int i = 0; i < phase.zombiesPerWave; ++i) {
      seq.push_back(t);
    }
  }

  if (phase.randomOrder && seq.size() > 1) {
    std::mt19937 rng{std::random_device{}()};
    std::shuffle(seq.begin(), seq.end(), rng);
  }

  return seq;
}

std::vector<ZombieWaveSpawnGroup>
ZombieWaveController::BuildPlan(const LevelWaveConfig &config) {
  std::vector<ZombieWaveSpawnGroup> plan;
  float timelineSec = 0.0F;

  for (const auto &phase : config.phases) {
    timelineSec += phase.startDelaySec;
    for (int i = 0; i < phase.repeat; ++i) {
      ZombieWavePhaseConfig copy = phase;
      if (copy.zombieTypes.empty()) {
        copy.zombieTypes = {copy.zombieType};
      }
      copy.zombieTypes = BuildTypeSequence(copy);

      plan.push_back({
          phase.id,
          phase.type,
          phase.zombieType,
          copy.zombieTypes,
          copy.randomOrder,
          timelineSec,
          phase.zombiesPerWave,
          phase.spawnIntervalSec,
          phase.waitUntilClear,
      });
      timelineSec += phase.waveIntervalSec;
    }
  }
  return plan;
}

// ── Public interface ─────────────────────────────────────────────────────────

void ZombieWaveController::Initialize(const LevelWaveConfig &config) {
  Reset();
  m_Plan = BuildPlan(config);
}

void ZombieWaveController::Reset() {
  m_Started = false;
  m_ElapsedSec = 0.0F;
  m_Plan.clear();
  m_CurrentGroupIndex = 0;
  m_GroupActive = false;
  m_GroupSpawnedCount = 0;
  m_GroupSpawnTimer = 0.0F;
  m_CurrentGroup = {};
}

bool ZombieWaveController::IsFinished() const {
  return m_Started && !m_GroupActive &&
         m_CurrentGroupIndex >= m_Plan.size();
}

std::vector<ZombieWaveController::SpawnRequest>
ZombieWaveController::Tick(float dt, bool hasAliveZombie, float hugeBannerSec,
                           bool &outTriggerHuge) {
  outTriggerHuge = false;
  std::vector<SpawnRequest> requests;

  if (!m_Started) {
    return requests;
  }

  m_ElapsedSec += dt;

  // Activate the next group if conditions are met.
  if (!m_GroupActive && m_CurrentGroupIndex < m_Plan.size()) {
    const ZombieWaveSpawnGroup &candidate = m_Plan[m_CurrentGroupIndex];
    const bool timeReached = m_ElapsedSec >= candidate.earliestStartSec;
    const bool clearOk = !candidate.waitUntilClear || !hasAliveZombie;
    const bool isHuge =
        (candidate.phaseId == "huge_wave") || (candidate.phaseType == "huge");

    if (timeReached && clearOk) {
      if (isHuge) {
        outTriggerHuge = true;
      }
      if (isHuge && hugeBannerSec > 0.0F) {
        return requests; // wait for banner to finish
      }
      m_CurrentGroup = candidate;
      m_GroupActive = true;
      m_GroupSpawnedCount = 0;
      m_GroupSpawnTimer = 0.0F;
    }
  }

  if (!m_GroupActive) {
    return requests;
  }

  // Emit zombie spawn requests for this frame.
  m_GroupSpawnTimer -= dt;
  while (m_GroupSpawnedCount < m_CurrentGroup.zombieCount &&
         m_GroupSpawnTimer <= 0.0F) {
    const std::string type =
        m_CurrentGroup.zombieTypes.empty()
            ? m_CurrentGroup.zombieType
            : m_CurrentGroup.zombieTypes[static_cast<std::size_t>(
                                             m_GroupSpawnedCount) %
                                         m_CurrentGroup.zombieTypes.size()];
    requests.push_back({type});
    ++m_GroupSpawnedCount;

    m_GroupSpawnTimer += (m_CurrentGroup.spawnIntervalSec <= 0.0F)
                             ? 0.0F
                             : m_CurrentGroup.spawnIntervalSec;
  }

  if (m_GroupSpawnedCount >= m_CurrentGroup.zombieCount) {
    m_GroupActive = false;
    ++m_CurrentGroupIndex;
  }

  return requests;
}
