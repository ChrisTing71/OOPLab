#ifndef PEASHOOTER_HPP
#define PEASHOOTER_HPP

#include "Plant.hpp"

class Peashooter : public Plant {
public:
  // framePaths    - extracted PNG frames from the GIF
  // frameInterval - milliseconds per frame
  // targetHeight  - desired height in pixels (image is scaled proportionally)
  Peashooter(const std::vector<std::string> &framePaths,
             std::size_t frameIntervalMs, float targetHeight);
};

#endif
