#ifndef LEADER_ZOMBIE_HPP
#define LEADER_ZOMBIE_HPP

#include "Zombie.hpp"

class LeaderZombie : public Zombie {
public:
  LeaderZombie(const std::vector<std::string> &walkingFrames,
               const std::vector<std::string> &attackingFrames,
               const std::vector<std::string> &dyingFrames,
               const std::vector<std::string> &cherryBombDyingFrames,
               float targetHeightPx, std::size_t frameIntervalMs = 120,
               float moveSpeedPxPerSec = 17.0F, int health = 200)
      : Zombie(walkingFrames, attackingFrames, dyingFrames,
               cherryBombDyingFrames, targetHeightPx, frameIntervalMs,
               moveSpeedPxPerSec, health) {}

  ~LeaderZombie() override = default;
};

#endif