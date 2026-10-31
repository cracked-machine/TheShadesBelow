#ifndef SRC_COMPONENTS_BURRCOUNT_HPP__
#define SRC_COMPONENTS_BURRCOUNT_HPP__

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <Utils/Cardinal.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace Game::Cmp
{

//! @brief Burrs stuck to the faces of an NPC or an obstacle, where they struck. Each burr slows movement until it expires: an
//! NPC's burrs slow that NPC, an obstacle's burrs slow the neighbouring grid position outside the face they are stuck to.
//! @note Managed by Sys::BurrSystem. See Utils::get_burr_speed_multiplier() for how the slowdown is applied.
class BurrCount
{
public:
  //! @brief Render scale of a burr stuck to an NPC or obstacle, relative to the in-flight burr sprite.
  static constexpr float kStuckScale{ 1.f };

  //! @brief A single attached burr.
  struct Entry
  {
    //! @brief Time elapsed since the burr was attached.
    sf::Time elapsed;
    //! @brief The face the burr is stuck to, pointing outwards.
    Utils::Cardinal side;
    //! @brief Where the burr is stuck, relative to the centre of the NPC/obstacle it is stuck to.
    sf::Vector2f offset;
  };

  //! @brief Attach another burr, starting its expiry timer from zero.
  //! @param side The face the burr is stuck to, pointing outwards.
  //! @param offset Where the burr is stuck, relative to the centre of the NPC/obstacle.
  void add( Utils::Cardinal side, sf::Vector2f offset ) { m_burrs.push_back( { sf::Time::Zero, side, offset } ); }

  //! @brief Advance every burr's timer and drop the burrs that have expired.
  //! @param dt Time since the last update.
  //! @param expiry How long a burr lasts, see Cmp::Persist::BurrExpiry.
  void tick( sf::Time dt, sf::Time expiry )
  {
    for ( auto &burr : m_burrs )
      burr.elapsed += dt;
    std::erase_if( m_burrs, [expiry]( const Entry &burr ) { return burr.elapsed >= expiry; } );
  }

  //! @brief Number of burrs currently attached.
  [[nodiscard]] int count() const { return static_cast<int>( m_burrs.size() ); }

  //! @brief The attached burrs, oldest first.
  [[nodiscard]] const std::vector<Entry> &burrs() const { return m_burrs; }

  //! @brief Number of burrs stuck to one face.
  //! @param side The face, pointing outwards.
  [[nodiscard]] int count( Utils::Cardinal side ) const
  {
    return static_cast<int>( std::ranges::count_if( m_burrs, [side]( const Entry &burr ) { return burr.side == side; } ) );
  }

  //! @brief Whether every burr has expired.
  [[nodiscard]] bool empty() const { return m_burrs.empty(); }

  //! @brief Movement speed multiplier: each burr compounds the slowdown.
  //! @param factor Speed multiplier for a single burr, see Cmp::Persist::BurrSlowFactor.
  //! @return float `factor` raised to the burr count, 1.f if there are no burrs.
  [[nodiscard]] float multiplier( float factor ) const { return std::pow( factor, static_cast<float>( count() ) ); }

  //! @brief Movement speed multiplier for the grid position outside one obstacle face.
  //! @param factor Speed multiplier for a single burr, see Cmp::Persist::BurrSlowFactor.
  //! @param side The face, pointing outwards.
  //! @return float `factor` raised to the face's burr count, 1.f if there are no burrs on it.
  [[nodiscard]] float multiplier( float factor, Utils::Cardinal side ) const { return std::pow( factor, static_cast<float>( count( side ) ) ); }

private:
  //! @brief The attached burrs, oldest first.
  std::vector<Entry> m_burrs;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_BURRCOUNT_HPP__
