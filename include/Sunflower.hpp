#ifndef SUNFLOWER_HPP
#define SUNFLOWER_HPP

#include "Plant.hpp"

class Sunflower : public Plant {
public:
  // framePaths    - extracted PNG frames from the GIF
  // frameInterval - milliseconds per frame
  // targetHeightPx - externally provided sprite height in pixels
  Sunflower(const std::vector<std::string> &framePaths,
            std::size_t frameIntervalMs, float targetHeightPx);

  bool ProducesSun() const override { return true; }

  bool ShouldProduceSun(float deltaTime);
  void OnProducedSunCollected() override;
  glm::vec2 GetSunSpawnOffset() const;
  glm::vec2 GetSunPopTargetOffset(float popDistancePx) const;

private:
  bool m_HasActiveProducedSun = false;
  float m_TimeUntilNextSun = 7.0F;
};

#endif
