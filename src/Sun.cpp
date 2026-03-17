#include "Sun.hpp"

#include "Util/Image.hpp"

Sun::Sun(const float targetHeightPx) {
  auto image = std::make_shared<Util::Image>("Resources/sun.png");
  SetDrawable(image);
  SetZIndex(11.0F);

  const glm::vec2 size = image->GetSize();
  if (size.y > 0.0F) {
    const float scale = targetHeightPx / size.y;
    m_Transform.scale = {scale, scale};
  }
}
