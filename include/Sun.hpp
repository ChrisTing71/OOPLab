#ifndef SUN_HPP
#define SUN_HPP

#include "pch.hpp" // IWYU pragma: export

#include "Util/GameObject.hpp"

class Sun : public Util::GameObject {
public:
  explicit Sun(float targetHeightPx);
  ~Sun() override = default;
};

#endif
