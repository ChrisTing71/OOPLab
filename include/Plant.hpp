#ifndef PLANT_HPP
#define PLANT_HPP

#include "pch.hpp" // IWYU pragma: export

#include "Util/GameObject.hpp"

class Plant : public Util::GameObject {
public:
  Plant() = default;
  virtual ~Plant() = default;
};

#endif
