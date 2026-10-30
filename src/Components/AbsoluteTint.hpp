#ifndef SRC_COMPONENTS_ABSOLUTETINT_HPP__
#define SRC_COMPONENTS_ABSOLUTETINT_HPP__

#include <SFML/Graphics/Color.hpp>

namespace Game::Cmp
{

//! @brief Component to store an absolute colour tint, multiplied with the sprite texture. Its alpha is multiplied
//! with Cmp::AbsoluteAlpha. Used by RenderGameSystem to tint the sprite, this should be set by other systems
class AbsoluteTint
{
public:
  //! @brief Construct with an initial tint.
  //! @param tint Tint colour. Defaults to white (no tint).
  AbsoluteTint( sf::Color tint = sf::Color::White )
      : m_tint( tint )
  {
  }

  //! @brief Get the stored tint.
  //! @return sf::Color The tint colour.
  sf::Color getTint() const { return m_tint; }

  //! @brief Set the stored tint.
  //! @param tint The tint colour.
  void setTint( sf::Color tint ) { m_tint = tint; }

private:
  //! @brief Tint colour, multiplied with the sprite texture.
  sf::Color m_tint;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_ABSOLUTETINT_HPP__
