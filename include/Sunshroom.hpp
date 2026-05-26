#ifndef SUNSHROOM_HPP
#define SUNSHROOM_HPP

#include "Plant.hpp"
#include "Util/Animation.hpp"

class Sunshroom : public Plant {
public:
  static constexpr float kGrowUpTimeSeconds = 120.0F;
  static constexpr int kInitialSunValue = 15;
  static constexpr int kGrownSunValue = 25;

  Sunshroom(const std::vector<std::string> &initialFramePaths,
            const std::vector<std::string> &grownFramePaths,
            std::size_t frameIntervalMs, float targetHeightPx);

  void Update(float deltaTime);
  bool ShouldProduceSun(float deltaTime);
  int GetProducedSunValue() const;
  int GetSunValue() const;
  int GetSunValueAtGrowthTime(float growthTime) const;
  float GetGrowthTimeSeconds() const;
  glm::vec2 GetSunSpawnOffset() const;
  glm::vec2 GetSunPopTargetOffset(float popDistancePx) const;
  void OnProducedSunCollected() override;

private:
  enum class State {
    Initial,
    Grown,
  };

  void SetCurrentAnimation();
  void ApplyScaleToDrawable(float targetHeightPx);

  State m_State = State::Initial;
  float m_GrowthTime = 0.0F;
  bool m_HasActiveProducedSun = false;
  int m_ProducedSunValue = kInitialSunValue;
  float m_TimeUntilNextSun = 7.0F;
  std::shared_ptr<Util::Animation> m_InitialAnimation = nullptr;
  std::shared_ptr<Util::Animation> m_GrownAnimation = nullptr;
};

#endif
