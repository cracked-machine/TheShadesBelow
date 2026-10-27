#ifndef SRC_COMPONENTS_ALTAR_MULTIBLOCK_HPP__
#define SRC_COMPONENTS_ALTAR_MULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>

#include <Components/Obstacle.hpp>

namespace Game::Cmp::Altar
{

//! @brief Multi-block sprite for an altar, defined geometrically as a rectangle.
//! @details Can be used to find its Segment parts and track overall activation state, including
//!          the running sacrifice count and exit/crypt key drop thresholds.
class MultiBlock : public sf::FloatRect
{
public:
  //! @brief Construct an altar MultiBlock at `position` with the given `size`.
  //! @details Initializes the exitkey drop threshold to 4 and the cryptkey drop threshold to 5.
  //! @param position Top-left world position of the block's bounding rect.
  //! @param size Width/height of the block's bounding rect.
  MultiBlock( const sf::Vector2f &position, const sf::Vector2f &size )
      : sf::FloatRect( position, size )
  {
    m_exitkey_drop_threshold = 4;
    m_cryptkey_drop_threshold = 5;
  }

  //! @brief Increment the number of sacrifices made at this altar by one.
  void increment_sacrifice_count() { ++m_sacrifice_count; }
  //! @brief Set the number of sacrifices made at this altar.
  //! @param count The new sacrifice count.
  void set_sacrifice_count( uint8_t count ) { m_sacrifice_count = count; }
  //! @brief Get the number of sacrifices made at this altar.
  //! @return uint8_t The current sacrifice count.
  [[nodiscard]] uint8_t get_sacrifice_count() const { return m_sacrifice_count; }

  //! @brief Get the number of sacrifices required to drop an exitkey.
  //! @return uint8_t The exitkey drop threshold.
  [[nodiscard]] uint8_t get_exitkey_drop_threshold() const { return m_exitkey_drop_threshold; }
  //! @brief Get the number of sacrifices required to drop a cryptkey.
  //! @return uint8_t The cryptkey drop threshold.
  [[nodiscard]] uint8_t get_cryptkey_drop_threshold() const { return m_cryptkey_drop_threshold; }

  //! @brief Mark the exitkey as already dropped, preventing this altar from dropping it again.
  void set_exitkey_lockout() { m_exitkey_drop_lockout = true; }
  //! @brief Whether this altar's exitkey has already been dropped.
  //! @return bool True if locked out from dropping another exitkey.
  bool is_exitkey_lockout() const { return m_exitkey_drop_lockout; }

  //! @brief Mark the cryptkey as already dropped, preventing this altar from dropping it again.
  void set_cryptkey_lockout() { m_cryptkey_drop_lockout = true; }
  //! @brief Whether this altar's cryptkey has already been dropped.
  //! @return bool True if locked out from dropping another cryptkey.
  bool is_cryptkey_lockout() const { return m_cryptkey_drop_lockout; }

  //! @brief Local-space offsets of the flame particle spawn points for each sacrifice, one per
  //!        corner of the altar's four segments. Defined in MultiBlock.cpp so tweaking values doesn't
  //!        recompile every includer.
  static const std::array<sf::Vector2f, 4> flame_offsets;

private:
  //! @brief The number of sacrifices required to drop an exitkey (4 relics)
  uint8_t m_exitkey_drop_threshold{ 0 };
  //! @brief The number of sacrifices required to drop an cryptkey (4 relics + 1 exitkey)
  uint8_t m_cryptkey_drop_threshold{ 0 };

  //! @brief The number of sacrifices made at this altar so far.
  uint8_t m_sacrifice_count{ 0 };

  //! @brief Whether this altar has already dropped its exitkey.
  bool m_exitkey_drop_lockout{ false };
  //! @brief Whether this altar has already dropped its cryptkey.
  bool m_cryptkey_drop_lockout{ false };
};

} // namespace Game::Cmp::Altar

#endif // SRC_COMPONENTS_ALTAR_MULTIBLOCK_HPP__
