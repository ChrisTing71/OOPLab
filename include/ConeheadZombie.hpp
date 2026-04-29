#ifndef CONEHEAD_ZOMBIE_HPP
#define CONEHEAD_ZOMBIE_HPP

#include "Zombie.hpp"

class ConeheadZombie : public Zombie {
public:
  ConeheadZombie(const std::vector<std::string> &walkingFrames,
                 const std::vector<std::string> &attackingFrames,
                 const std::vector<std::string> &dyingFrames,
                 const std::vector<std::string> &cherryBombDyingFrames,
                 float targetHeightPx, std::size_t frameIntervalMs = 120,
                 float moveSpeedPxPerSec = 17.0F, int health = 600)
      : Zombie(walkingFrames, attackingFrames, dyingFrames,
               cherryBombDyingFrames, targetHeightPx, frameIntervalMs,
               moveSpeedPxPerSec, health) {}

  ~ConeheadZombie() override = default;
};

#endif
