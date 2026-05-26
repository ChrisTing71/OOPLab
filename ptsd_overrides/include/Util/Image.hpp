#ifndef UTIL_IMAGE_HPP
#define UTIL_IMAGE_HPP

#include "pch.hpp" // IWYU pragma: export

#include <glm/fwd.hpp>

#include "Core/Drawable.hpp"
#include "Core/Program.hpp"
#include "Core/Texture.hpp"
#include "Core/UniformBuffer.hpp"
#include "Core/VertexArray.hpp"

#include "Util/AssetStore.hpp"
#include "Util/Transform.hpp"

namespace Util {
/**
 * @class Image
 * @brief A class representing an image.
 *
 * This class encapsulates the properties and behaviors of an image.
 * It includes properties such as texture and surface.
 * It also includes behaviors such as drawing the image.
 */
class Image : public Core::Drawable {
public:
  /**
   * @brief Constructor that takes a file path to the image.
   *
   * @param filepath The file path to the image.
   */
  explicit Image(const std::string &filepath);

  /**
   * @brief Retrieves the size of the image.
   *
   * This function returns the size of the image.
   *
   * @return The size of the image as a vec2(x, y).
   */
  glm::vec2 GetSize() const override { return m_Size; };

  /**
   * @brief Sets the image to the specified file path.
   *
   * This function sets the image to the specified file path.
   *
   * @param filepath The file path to the image.
   */
  void SetImage(const std::string &filepath);

  /**
   * @brief Draws the image with a given transform and z-index.
   *
   * This function draws the image at the specified z-index and applies the
   * given transform.
   *
   * @param transform The transform to apply to the image.
   * @param zIndex The z-index at which to draw the image.
   */
  void Draw(const Core::Matrices &data) override;

  /**
   * @brief Sets the tint color for the image.
   *
   * @param color The tint color as glm::vec4 (R, G, B, A) with values 0-1.
   */
  void SetTintColor(const glm::vec4 &color) { m_TintColor = color; }

  /**
   * @brief Gets the current tint color.
   *
   * @return The current tint color as glm::vec4.
   */
  glm::vec4 GetTintColor() const { return m_TintColor; }

  /**
   * @brief Sets the fill progress for cooldown bar display.
   * 0.0 = empty, 1.0 = full. Values outside this range are clamped.
   *
   * @param progress The fill progress (0.0 to 1.0).
   */
  void SetFillProgress(float progress) {
    m_FillProgress = glm::clamp(progress, 0.0F, 1.0F);
  }

  /**
   * @brief Gets the current fill progress.
   *
   * @return The current fill progress (0.0 to 1.0).
   */
  float GetFillProgress() const { return m_FillProgress; }

  /**
   * @brief Enables/disables fill progress visualization.
   *
   * @param enabled True to show fill progress, false to show normal image.
   */
  void SetShowFillProgress(bool enabled) { m_ShowFillProgress = enabled; }

private:
  void InitProgram();
  void InitVertexArray();
  void InitUniformBuffer();

  static constexpr int UNIFORM_SURFACE_LOCATION = 0;

  static std::unique_ptr<Core::Program> s_Program;
  static std::unique_ptr<Core::VertexArray> s_VertexArray;
  std::unique_ptr<Core::UniformBuffer<Core::Matrices>> m_UniformBuffer;

  static Util::AssetStore<std::shared_ptr<SDL_Surface>> s_Store;

private:
  std::unique_ptr<Core::Texture> m_Texture = nullptr;

  std::string m_Path;
  glm::vec2 m_Size;
  glm::vec4 m_TintColor = {1.0F, 1.0F, 1.0F, 1.0F};
  float m_FillProgress = 1.0F;
  bool m_ShowFillProgress = false;
};
} // namespace Util

#endif
