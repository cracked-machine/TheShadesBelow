#ifndef SRC_COMPONENTS_GRAVE_MULTIBLOCK_HPP__
#define SRC_COMPONENTS_GRAVE_MULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>

#include <Components/Obstacle.hpp>

namespace Game::Cmp::Grave
{

//! @brief Represents a diggable grave as a multi-block sprite defined geometrically as a rectangle.
//! @details Can be used to find its Segment parts and track overall activation state. The grave is dug
//! up by the player, reducing hp until it reaches zero.
class MultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct a new MultiBlock object.
  //! @param position Top-left world position of the multi-block bounds.
  //! @param size Width/height of the multi-block bounds. Also used to derive the activation threshold
  //! (one "block" per 16x16 pixel area).
  MultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
    m_activation_threshold = ( size.x / 16 ) * ( size.y / 16 ); // each block is 16x16 pixels
  }

  //! @brief Increment the number of activated sprites in this multi-block.
  void increment_activation_count() { ++m_activation_count; }

  //! @brief Set the number of activated sprites in this multi-block.
  //! @param count New activation count.
  void set_activation_count( uint8_t count ) { m_activation_count = count; }

  //! @brief Get the number of activated sprites in this multi-block.
  //! @return uint8_t
  [[nodiscard]] uint8_t get_activation_count() const { return m_activation_count; }

  //! @brief Get the activation count threshold, derived from the multi-block's size in 16x16 blocks.
  //! @return uint8_t
  [[nodiscard]] uint8_t get_activation_threshold() const { return m_activation_threshold; }

  //! @brief Set whether this multi-block's powers/effects are active.
  //! @param activate true to activate, false to deactivate.
  void set_powers_active( bool activate = true ) { m_powers_active = activate; }

  //! @brief Check whether this multi-block's powers/effects are active.
  //! @return bool
  [[nodiscard]] bool are_powers_active() const { return m_powers_active; }

  //! @brief hit points/health until opened
  int hp{ 100 };

private:
  //! @brief Track the number of activated sprites in this multi-block.
  //! @note Callees are responsible for managing max thresholds.
  uint8_t m_activation_threshold{ 0 };
  //! @brief The current count of activated sprites in this multi-block.
  uint8_t m_activation_count{ 0 };

  //! @brief Overall multi-block activation. Has multiple meanings depending on context.
  //! @note The callee is responsible for knowing when to set this. Usually when m_activation_count
  //! reaches some threshold value.
  bool m_powers_active{ false };
};

} // namespace Game::Cmp::Grave

#endif // SRC_COMPONENTS_GRAVE_MULTIBLOCK_HPP__
