#include "Sunflower.hpp"

#include "Util/Animation.hpp"

Sunflower::Sunflower(const std::vector<std::string> &framePaths,
                     const std::size_t frameIntervalMs,
                     const float targetHeight) {
  auto animation = std::make_shared<Util::Animation>(framePaths, true,
                                                     frameIntervalMs, true, 0);

  SetDrawable(animation);
  SetZIndex(1.0F);

  const float drawableHeight = animation->GetSize().y;
  if (drawableHeight > 0.0F) {
    const float uniformScale = targetHeight / drawableHeight;
    m_Transform.scale = {uniformScale, uniformScale};
  }
}
