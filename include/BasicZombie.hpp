#ifndef BASIC_ZOMBIE_HPP
#define BASIC_ZOMBIE_HPP

#include "Zombie.hpp"

class BasicZombie : public Zombie {
public:
  BasicZombie(const std::vector<std::string> &walkingFrames,
              const std::vector<std::string> &attackingFrames,
              const std::vector<std::string> &dyingFrames,
              std::size_t frameIntervalMs = 120,
              float moveSpeedPxPerSec = 17.0F, int health = 200)
      : Zombie(walkingFrames, attackingFrames, dyingFrames, frameIntervalMs,
               moveSpeedPxPerSec, health) {}

  ~BasicZombie() override = default;
};

#endif
