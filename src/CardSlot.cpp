#include "CardSlot.hpp"

#include "Util/Image.hpp"

CardSlot::CardSlot() {
  auto image = std::make_shared<Util::Image>("Resources/ui/hud/upper_slot.png");
  SetDrawable(image);
  SetZIndex(10.0F);
  m_SourceSize = image->GetSize();
}
