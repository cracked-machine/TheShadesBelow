#ifndef SRC_COMPONENTS_INVENTORY_DOWSINGTARGET_HPP__
#define SRC_COMPONENTS_INVENTORY_DOWSINGTARGET_HPP__

#include <Components/Random.hpp>

#include <SFML/System/Time.hpp>

#include <algorithm>
#include <vector>

namespace Game::Cmp::Inventory
{

//! @brief The landmark a dowsing rod guides towards. Assigned once when the rod is created and carried with it
//! between the world item and the player inventory slot. While the rod is in the player inventory, DoglegSystem
//! sends colored chevrons along a "dogleg" path from the player to each landmark matching `target`.
struct DowsingTarget
{
  //! @brief The landmark a dowsing rod guides towards, also used as the color of its guide chevrons.
  enum class Target {
    //! @brief No landmark - no guide chevrons are drawn.
    NONE,
    //! @brief Guides towards crypt entrances, drawn in red.
    RED,
    //! @brief Guides towards altars, drawn in yellow.
    YELLOW,
    //! @brief Guides towards exits, drawn in green.
    GREEN
  };

  //! @brief Pick a random Target not present in `excludes`.
  //! @param excludes Targets to exclude from the pick.
  //! @return Target A randomly chosen Target from the remaining pool, or Target::NONE if none remain.
  static Target random_pick( std::vector<Target> excludes )
  {
    std::vector<Target> pool{ Target::GREEN, Target::RED, Target::YELLOW };
    std::erase_if( pool, [&excludes]( Target t ) { return std::ranges::find( excludes.begin(), excludes.end(), t ) != excludes.end(); } );
    if ( pool.empty() ) return Target::NONE;
    Cmp::RandomInt rnd( 0, pool.size() - 1 );
    return pool.at( rnd.gen() );
  }

  //! @brief The landmark this dowsing rod guides towards.
  Target target{ Target::NONE };

  //! @brief Time since the chevron pulse last left the player. Advanced and reset by DoglegSystem.
  sf::Time m_dowsing_pulse_time{ sf::Time::Zero };
};

} // namespace Game::Cmp::Inventory

#endif // SRC_COMPONENTS_INVENTORY_DOWSINGTARGET_HPP__
