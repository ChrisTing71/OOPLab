#ifndef SUN_MANAGER_HPP
#define SUN_MANAGER_HPP

#include <memory>
#include <random>
#include <vector>

#include "Plant.hpp"
#include "Sun.hpp"
#include "Sunflower.hpp"
#include "Sunshroom.hpp"
#include "Util/Renderer.hpp"

// Owns all active sun objects and drives their lifecycle:
//   falling sky-suns, sunflower/sunshroom pops, collection animation.
// App retains m_Sunlight (tightly coupled to plant placement cost checks).
class SunManager {
public:
  struct ActiveSun {
    std::shared_ptr<Sun> object;
    std::weak_ptr<Plant> producer; // set for plant-produced suns
    int value = 25;
    float aliveSeconds = 0.0F;
    float stoppedSeconds = 0.0F;
    bool collecting = false;
    float collectElapsed = 0.0F;
    glm::vec2 collectStart = {0.0F, 0.0F};
    bool rising = false;
    float riseElapsed = 0.0F;
    glm::vec2 riseStart = {0.0F, 0.0F};
    glm::vec2 riseTarget = {0.0F, 0.0F};
    bool falling = true;
    bool stopped = false;
    float stopLocalY = 0.0F;
    bool expires = true;
    bool fromSky = false;
  };

  // @param uiRoot  Renderer that sun GameObjects are added to / removed from.
  // @param rng     Shared RNG (owned by App).
  SunManager(Util::Renderer &uiRoot, std::mt19937 &rng);

  void Start(float initialCountdownSec);
  void Reset();

  bool IsStarted() const { return m_Started; }

  // Advance all suns and spawn new ones.
  // @param collectTarget  UI-space position of the sun counter (collection
  //                       animation target).
  // @param sunflowers     Grid of sunflower plants (nullptr slots allowed).
  // @param sunshrooms     Grid of sunshroom plants (nullptr slots allowed).
  // @param cameraX        Current camera X offset (local-space).
  // @param isNight        If true, sky-suns are not spawned.
  // @returns              Amount of sunlight collected this frame (may be 0).
  int Update(float dt, const glm::vec2 &collectTarget, bool isNight,
             float cameraX,
             const std::vector<std::shared_ptr<Sunflower>> &sunflowers,
             const std::vector<std::shared_ptr<Sunshroom>> &sunshrooms);

  // Try to collect a sun at the given screen pixel position.
  bool TryCollect(float pixelX, float pixelY);

  // Spawn a sky-sun at a random position.
  void SpawnFallingSun(float cameraX);

  // Spawn a sun from a sunflower plant.
  void SpawnFromSunflower(const std::shared_ptr<Sunflower> &sf, float cameraX);

  // Spawn a sun from a sunshroom plant.
  void SpawnFromSunshroom(const std::shared_ptr<Sunshroom> &ss, int value,
                          float cameraX);

  void DrawCounter(int sunlight) const;

private:
  void RemoveSunAt(std::size_t index);

  static constexpr float kCollectMoveSec = 0.30F;
  static constexpr float kRiseMoveSec = 0.35F;
  static constexpr float kSkyStopExpirySec = 5.0F;
  static constexpr float kFallingSpeedFraction = 0.05F;
  static constexpr float kSkySpawnIntervalSec = 8.0F;
  static constexpr float kSunHeightPercent = 10.0F;

  Util::Renderer &m_UIRoot;
  std::mt19937 &m_Rng;

  bool m_Started = false;
  float m_SpawnCountdown = 0.0F;
  std::vector<ActiveSun> m_Suns;
};

#endif // SUN_MANAGER_HPP
