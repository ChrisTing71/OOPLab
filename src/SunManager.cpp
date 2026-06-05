#include "SunManager.hpp"

#include <algorithm>

#include "CollisionSystem.hpp"
#include "config.hpp"

namespace {
float Lerp(float a, float b, float t) {
  return a + (b - a) * glm::clamp(t, 0.0F, 1.0F);
}
} // namespace

SunManager::SunManager(Util::Renderer &uiRoot, std::mt19937 &rng)
    : m_UIRoot(uiRoot), m_Rng(rng) {}

void SunManager::Start(float initialCountdownSec) {
  m_Started = true;
  m_SpawnCountdown = initialCountdownSec;
}

void SunManager::Reset() {
  for (const auto &sun : m_Suns) {
    m_UIRoot.RemoveChild(sun.object);
  }
  m_Suns.clear();
  m_Started = false;
  m_SpawnCountdown = 0.0F;
}

bool SunManager::TryCollect(float pixelX, float pixelY) {
  for (auto &sun : m_Suns) {
    if (!CollisionSystem::IsPixelInsideObject(sun.object, pixelX, pixelY)) {
      continue;
    }
    if (sun.collecting) {
      return true;
    }
    sun.collecting = true;
    sun.collectElapsed = 0.0F;
    sun.collectStart = sun.object->m_Transform.translation;
    sun.rising = false;
    sun.falling = false;
    return true;
  }
  return false;
}

void SunManager::SpawnFallingSun(float /*cameraX*/) {
  constexpr float kSpawnYPercent = 20.0F;
  constexpr float kStopMinYPercent = 52.0F;
  constexpr float kStopMaxYPercent = 78.0F;

  std::uniform_real_distribution<float> xDist(8.0F, 92.0F);
  std::uniform_real_distribution<float> stopYDist(kStopMinYPercent,
                                                   kStopMaxYPercent);

  const float spawnXPct = xDist(m_Rng);
  const float spawnPixelX =
      (spawnXPct / 100.0F) * static_cast<float>(WINDOW_WIDTH);
  const float spawnPixelY =
      (kSpawnYPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  const float localX = spawnPixelX - static_cast<float>(WINDOW_WIDTH) * 0.5F;
  const float localY = static_cast<float>(WINDOW_HEIGHT) * 0.5F - spawnPixelY;

  const float stopPixelY =
      (stopYDist(m_Rng) / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  const float stopLocalY =
      static_cast<float>(WINDOW_HEIGHT) * 0.5F - stopPixelY;

  const float sunHeightPx =
      (kSunHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  auto sun = std::make_shared<Sun>(sunHeightPx);
  sun->m_Transform.translation = {localX, localY};

  ActiveSun active;
  active.object = sun;
  active.falling = true;
  active.fromSky = true;
  active.expires = true;
  active.stopLocalY = stopLocalY;
  active.value = 25;
  m_UIRoot.AddChild(sun);
  m_Suns.push_back(active);
}

void SunManager::SpawnFromSunflower(const std::shared_ptr<Sunflower> &sf,
                                    float cameraX) {
  if (sf == nullptr) {
    return;
  }
  constexpr float kPopDistPct = 7.0F;
  constexpr float kCameraOffsetY = 0.05F * static_cast<float>(WINDOW_HEIGHT);

  const float sunHeightPx =
      (kSunHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  auto sun = std::make_shared<Sun>(sunHeightPx);

  const glm::vec2 offset = {cameraX, kCameraOffsetY};
  const glm::vec2 start =
      sf->m_Transform.translation + sf->GetSunSpawnOffset() + offset;
  const glm::vec2 target =
      sf->m_Transform.translation +
      sf->GetSunPopTargetOffset((kPopDistPct / 100.0F) *
                                static_cast<float>(WINDOW_HEIGHT)) +
      offset;
  sun->m_Transform.translation = start;

  ActiveSun active;
  active.object = sun;
  active.producer = sf;
  active.value = 25;
  active.falling = false;
  active.expires = false;
  active.rising = true;
  active.riseStart = start;
  active.riseTarget = target;
  m_UIRoot.AddChild(sun);
  m_Suns.push_back(active);
}

void SunManager::SpawnFromSunshroom(const std::shared_ptr<Sunshroom> &ss,
                                    int value, float cameraX) {
  if (ss == nullptr) {
    return;
  }
  constexpr float kPopDistPct = 7.0F;
  constexpr float kCameraOffsetY = 0.05F * static_cast<float>(WINDOW_HEIGHT);

  const float sunHeightPx =
      (kSunHeightPercent / 100.0F) * static_cast<float>(WINDOW_HEIGHT);
  auto sun = std::make_shared<Sun>(sunHeightPx);

  const glm::vec2 offset = {cameraX, kCameraOffsetY};
  const glm::vec2 start =
      ss->m_Transform.translation + ss->GetSunSpawnOffset() + offset;
  const glm::vec2 target =
      ss->m_Transform.translation +
      ss->GetSunPopTargetOffset((kPopDistPct / 100.0F) *
                                static_cast<float>(WINDOW_HEIGHT)) +
      offset;
  sun->m_Transform.translation = start;

  ActiveSun active;
  active.object = sun;
  active.producer = ss;
  active.value = value;
  active.falling = false;
  active.expires = false;
  active.rising = true;
  active.riseStart = start;
  active.riseTarget = target;
  m_UIRoot.AddChild(sun);
  m_Suns.push_back(active);
}

int SunManager::Update(float dt, const glm::vec2 &collectTarget, bool isNight,
                       float cameraX,
                       const std::vector<std::shared_ptr<Sunflower>> &sunflowers,
                       const std::vector<std::shared_ptr<Sunshroom>> &sunshrooms) {
  if (!m_Started) {
    return 0;
  }

  if (!isNight) {
    m_SpawnCountdown -= dt;
    if (m_SpawnCountdown <= 0.0F) {
      SpawnFallingSun(cameraX);
      m_SpawnCountdown = kSkySpawnIntervalSec;
    }
  }

  for (const auto &sf : sunflowers) {
    if (sf == nullptr || sf->IsDead()) {
      continue;
    }
    if (sf->ShouldProduceSun(dt)) {
      SpawnFromSunflower(sf, cameraX);
    }
  }

  for (const auto &ss : sunshrooms) {
    if (ss == nullptr || ss->IsDead()) {
      continue;
    }
    ss->Update(dt);
    if (ss->ShouldProduceSun(dt)) {
      SpawnFromSunshroom(ss, ss->GetProducedSunValue(), cameraX);
    }
  }

  const float dropSpeedPx =
      kFallingSpeedFraction * static_cast<float>(WINDOW_HEIGHT);

  for (auto &sun : m_Suns) {
    if (sun.collecting) {
      sun.collectElapsed += dt;
      const float t = glm::clamp(sun.collectElapsed / kCollectMoveSec, 0.0F, 1.0F);
      sun.object->m_Transform.translation = {
          Lerp(sun.collectStart.x, collectTarget.x, t),
          Lerp(sun.collectStart.y, collectTarget.y, t),
      };
      continue;
    }

    sun.aliveSeconds += dt;

    if (sun.rising) {
      sun.riseElapsed += dt;
      const float t = glm::clamp(sun.riseElapsed / kRiseMoveSec, 0.0F, 1.0F);
      const float eased = 1.0F - (1.0F - t) * (1.0F - t);
      sun.object->m_Transform.translation = {
          Lerp(sun.riseStart.x, sun.riseTarget.x, eased),
          Lerp(sun.riseStart.y, sun.riseTarget.y, eased),
      };
      if (t >= 1.0F) {
        sun.rising = false;
      }
      continue;
    }

    if (sun.falling) {
      sun.object->m_Transform.translation.y -= dropSpeedPx * dt;
      if (sun.fromSky && !sun.stopped &&
          sun.object->m_Transform.translation.y <= sun.stopLocalY) {
        sun.object->m_Transform.translation.y = sun.stopLocalY;
        sun.falling = false;
        sun.stopped = true;
        sun.stoppedSeconds = 0.0F;
      }
    }

    if (sun.fromSky && sun.stopped) {
      sun.stoppedSeconds += dt;
    }
  }

  int collected = 0;
  for (std::size_t i = 0; i < m_Suns.size();) {
    auto &sun = m_Suns[i];

    if (sun.collecting && sun.collectElapsed >= kCollectMoveSec) {
      const int val = sun.value;
      if (const auto producer = sun.producer.lock(); producer != nullptr) {
        producer->OnProducedSunCollected();
      }
      RemoveSunAt(i);
      collected += val;
      continue;
    }

    const glm::vec2 sunSize = sun.object->GetScaledSize();
    const float centerY = static_cast<float>(WINDOW_HEIGHT) * 0.5F -
                          sun.object->m_Transform.translation.y;
    const float sunTop = centerY - sunSize.y * 0.5F;
    const bool fallenOffScreen =
        sun.falling && sunTop > static_cast<float>(WINDOW_HEIGHT);
    const bool expired =
        sun.fromSky && sun.stopped && sun.stoppedSeconds > kSkyStopExpirySec;
    if (!sun.collecting && sun.expires && (fallenOffScreen || expired)) {
      RemoveSunAt(i);
      continue;
    }

    ++i;
  }

  return collected;
}

void SunManager::DrawCounter(int sunlight) const {
  const std::string text = std::to_string(sunlight);
  // Position is determined by the caller who has the CardSlot reference;
  // for now the counter is simply rendered at a fixed ImGui position.
  // (Actual rect computation stays in App::DrawSunlightCounter.)
  (void)text;
}

void SunManager::RemoveSunAt(std::size_t index) {
  m_UIRoot.RemoveChild(m_Suns[index].object);
  m_Suns.erase(m_Suns.begin() + static_cast<long>(index));
}
