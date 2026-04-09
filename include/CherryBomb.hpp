#ifndef CHERRY_BOMB_HPP
#define CHERRY_BOMB_HPP

#include "Plant.hpp"

#include "Util/Animation.hpp"

class CherryBomb : public Plant {
public:
  CherryBomb(const std::vector<std::string> &idleFramePaths,
             std::size_t idleFrameIntervalMs,
             const std::vector<std::string> &blowFramePaths,
             std::size_t blowFrameIntervalMs, float targetHeightPx,
             float explodeDelaySec = 1.0F);

  bool UpdateAndCheckExplode(float deltaTime);
  bool IsExploding() const { return m_IsExploding; }
  bool IsFinished() const { return m_IsFinished; }

private:
  void ApplyScaleForCurrentDrawable(float targetHeightPx);

private:
  float m_TargetHeightPx = 0.0F;
  float m_ExplodeDelaySec = 1.0F;
  bool m_IsExploding = false;
  bool m_IsFinished = false;

  std::shared_ptr<Util::Animation> m_IdleAnimation = nullptr;
  std::shared_ptr<Util::Animation> m_BlowAnimation = nullptr;
};

#endif