#ifndef NUT_HPP
#define NUT_HPP

#include "Plant.hpp"
#include "Util/Animation.hpp"

class Nut : public Plant {
public:
  Nut(const std::vector<std::string> &nut1FramePaths,
      std::size_t nut1FrameIntervalMs,
      const std::vector<std::string> &nut2FramePaths,
      std::size_t nut2FrameIntervalMs,
      const std::vector<std::string> &nut3FramePaths,
      std::size_t nut3FrameIntervalMs,
      const std::vector<std::string> &nut4FramePaths,
      std::size_t nut4FrameIntervalMs, float targetHeight);

  void TakeDamage(int amount) override;

private:
  void ApplyScaleForCurrentDrawable(float targetHeight);
  void UpdateStageDrawableByHealth();

private:
  static constexpr int kMaxHealth = 4000;

  float m_TargetHeight = 0.0F;
  int m_CurrentStage = 1;

  std::shared_ptr<Util::Animation> m_Stage1Animation = nullptr;
  std::shared_ptr<Util::Animation> m_Stage2Animation = nullptr;
  std::shared_ptr<Util::Animation> m_Stage3Animation = nullptr;
  std::shared_ptr<Util::Animation> m_Stage4Animation = nullptr;
};

#endif
