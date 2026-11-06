#ifndef SRC_COMPONENTS_ALTAR_MULTIBLOCK_HPP__
#define SRC_COMPONENTS_ALTAR_MULTIBLOCK_HPP__

#include <SFML/Graphics/RectangleShape.hpp>

#include <Components/Altar/Kind.hpp>
#include <Components/Obstacle.hpp>

namespace Game::Cmp::Altar
{

//! @brief Multi-block sprite for an altar, defined geometrically as a rectangle.
//! @details Can be used to find its Segment parts and track overall activation state, including
//!          the running offering count and exit/crypt key drop thresholds.
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

  //! @brief Increment the number of offerings made at this altar by one.
  void increment_offering_count() { ++m_offering_count; }
  //! @brief Set the number of offerings made at this altar.
  //! @param count The new offering count.
  void set_offering_count( uint8_t count ) { m_offering_count = count; }
  //! @brief Get the number of offerings made at this altar.
  //! @return uint8_t The current offering count.
  [[nodiscard]] uint8_t get_offering_count() const { return m_offering_count; }

  //! @brief Get the number of offerings required to drop an exitkey.
  //! @return uint8_t The exitkey drop threshold.
  [[nodiscard]] uint8_t get_exitkey_drop_threshold() const { return m_exitkey_drop_threshold; }
  //! @brief Get the number of offerings required to drop a cryptkey.
  //! @return uint8_t The cryptkey drop threshold.
  [[nodiscard]] uint8_t get_cryptkey_drop_threshold() const { return m_cryptkey_drop_threshold; }

  //! @brief Set which kind of altar this is. Defaults to Kind::GRAVEYARD.
  //! @param kind The altar kind.
  void set_kind( Kind kind ) { m_kind = kind; }
  //! @brief Get which kind of altar this is, used to pick the item's offering reward for this altar.
  //! @return Kind The altar kind.
  [[nodiscard]] Kind get_kind() const { return m_kind; }

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

private:
  //! @brief Which kind of altar this is.
  Kind m_kind{ Kind::GRAVEYARD };

  //! @brief The number of offerings required to drop an exitkey (4 relics)
  uint8_t m_exitkey_drop_threshold{ 0 };
  //! @brief The number of offerings required to drop an cryptkey (4 relics + 1 exitkey)
  uint8_t m_cryptkey_drop_threshold{ 0 };

  //! @brief The number of offerings made at this altar so far.
  uint8_t m_offering_count{ 0 };

  //! @brief Whether this altar has already dropped its exitkey.
  bool m_exitkey_drop_lockout{ false };
  //! @brief Whether this altar has already dropped its cryptkey.
  bool m_cryptkey_drop_lockout{ false };
};

} // namespace Game::Cmp::Altar

#endif // SRC_COMPONENTS_ALTAR_MULTIBLOCK_HPP__
