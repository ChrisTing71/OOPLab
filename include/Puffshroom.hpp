#ifndef PUFFSHROOM_HPP
#define PUFFSHROOM_HPP

#include "Plant.hpp"
#include "Util/Animation.hpp"

class Puffshroom : public Plant {
public:
  Puffshroom(const std::vector<std::string> &framePaths,
             std::size_t frameIntervalMs, float targetHeightPx);

  bool CanShoot() const override { return true; }

private:
  void ApplyScaleForCurrentDrawable(float targetHeightPx);
  std::shared_ptr<Util::Animation> m_IdleAnimation = nullptr;
  float m_TargetHeightPx = 0.0F;
};

#endif
