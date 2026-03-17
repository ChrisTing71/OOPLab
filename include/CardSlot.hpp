#ifndef CARD_SLOT_HPP
#define CARD_SLOT_HPP

#include "pch.hpp" // IWYU pragma: export

#include "Util/GameObject.hpp"

class CardSlot : public Util::GameObject {
public:
  CardSlot();
  ~CardSlot() override = default;

  glm::vec2 GetSourceSize() const { return m_SourceSize; }

private:
  glm::vec2 m_SourceSize = {0.0F, 0.0F};
};

#endif
