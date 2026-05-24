#ifndef ZOMBIE_COLLISION_BOX_HELPER_HPP
#define ZOMBIE_COLLISION_BOX_HELPER_HPP

#include "CollisionSystem.hpp"
#include "ConeheadZombie.hpp"
#include "LeaderZombie.hpp"
#include "PolevaultingZombie.hpp"
#include "Zombie.hpp"

namespace ZombieCollisionBoxHelper {

inline CollisionSystem::CollisionBoxType
GetZombieCollisionBoxType(const Zombie &zombie) {
  if (dynamic_cast<const PolevaultingZombie *>(&zombie) != nullptr) {
    return CollisionSystem::CollisionBoxType::PolevaultingZombieAttack;
  }
  if (dynamic_cast<const LeaderZombie *>(&zombie) != nullptr) {
    return CollisionSystem::CollisionBoxType::LeaderZombie;
  }
  if (dynamic_cast<const ConeheadZombie *>(&zombie) != nullptr) {
    return CollisionSystem::CollisionBoxType::ConeheadZombie;
  }
  return CollisionSystem::CollisionBoxType::BasicZombie;
}

} // namespace ZombieCollisionBoxHelper

#endif