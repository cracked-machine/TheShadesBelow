#ifndef SRC_COMPONENTS_CRYPT_INTERIORMULTIBLOCK_HPP__
#define SRC_COMPONENTS_CRYPT_INTERIORMULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>


namespace Game::Cmp::Crypt
{

//! @brief Multi-block sprite for a crypt interior decoration, defined geometrically as a rectangle spanning
//! all of its Cmp::Crypt::InteriorSegment parts.
//! @note Used to find the interior's segment parts and to track their overall activation state. Unlike
//! Cmp::Crypt::BuildingMultiBlock, the activation threshold is fixed at 1 rather than derived from area.
class InteriorMultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct a multi-block covering the given rectangle with a fixed activation threshold of 1.
  //! @param position Top-left world position of the multi-block.
  //! @param size Pixel dimensions of the multi-block.
  InteriorMultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
    m_activation_threshold = 1;
  }

  //! @brief Increment the count of activated segments by one.
  void increment_activation_count() { ++m_activation_count; }
  //! @brief Set the count of activated segments.
  //! @param count
  void set_activation_count( uint8_t count ) { m_activation_count = count; }
  //! @brief Get the current count of activated segments.
  //! @return uint8_t
  uint8_t get_activation_count() const { return m_activation_count; }

  //! @brief Get the number of activated segments required for this multi-block to be considered fully activated.
  //! @return uint8_t
  uint8_t get_activation_threshold() const { return m_activation_threshold; }

private:
  //! @brief Number of activated segments required for this multi-block to be considered fully activated.
  //! @note Callers are responsible for managing/enforcing this threshold.
  uint8_t m_activation_threshold{ 0 };
  //! @brief Number of segments currently activated in this multi-block.
  uint8_t m_activation_count{ 0 };
};

} // namespace Game::Cmp::Crypt

#endif // SRC_COMPONENTS_CRYPT_INTERIORMULTIBLOCK_HPP__
