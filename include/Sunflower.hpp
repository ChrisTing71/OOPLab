#ifndef SUNFLOWER_HPP
#define SUNFLOWER_HPP

#include "Plant.hpp"

class Sunflower : public Plant {
public:
  // framePaths    - extracted PNG frames from the GIF
  // frameInterval - milliseconds per frame
  // targetHeight  - desired height in pixels (image is scaled proportionally)
  Sunflower(const std::vector<std::string> &framePaths,
            std::size_t frameIntervalMs, float targetHeight);
};

#endif
