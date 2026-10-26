#ifndef SRC_COMPONENTS_WORMHOLE_JUMP_HPP__
#define SRC_COMPONENTS_WORMHOLE_JUMP_HPP__

#include <SFML/System/Time.hpp>
namespace Game::Cmp::Wormhole
{

//! @brief Tracks the teleport cooldown for an actor caught inside a wormhole.
//! @details Attached to an entity when it collides with a wormhole's Singularity/MultiBlock.
//!          WormholeSystem teleports the actor once jump_elapsed reaches
//!          jump_cooldown, then removes this component.
class Jump
{
public:
  //! @brief Gameplay time counting up towards the next teleport jump. Accumulated by WormholeSystem.
  sf::Time jump_elapsed{};
  //! @brief Time the actor must remain in the wormhole before being teleported.
  sf::Time jump_cooldown{ sf::seconds( 2.0f ) };
  //! @brief Whether this jump is currently active.
  bool active{ true };
};

} // namespace Game::Cmp::Wormhole

#endif // SRC_COMPONENTS_WORMHOLE_JUMP_HPP__
