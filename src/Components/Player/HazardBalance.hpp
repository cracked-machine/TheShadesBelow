#ifndef SRC_CMPS_PLAYER_HAZARDBALANCE_HPP__
#define SRC_CMPS_PLAYER_HAZARDBALANCE_HPP__

#include <Utils/Cardinal.hpp>

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/entity.hpp>

namespace Game::Cmp::Player
{

//! @brief Added to the player entity when they walk into a lethal hazard cell (sinkhole/lava pit). Movement
//!        is frozen and the wobble animation plays; pressing the direction opposite `hazard_cell_dir` before
//!        Cmp::Persist::HazardBalanceTimeout expires returns the player to `safe_pos`, otherwise they fall in.
//!        See Sys::PlayerSystem::update_hazard_balance().
struct HazardBalance
{
  //! @brief The lethal hazard cell the player is balancing over.
  entt::entity hazard{ entt::null };
  //! @brief Direction from the player towards the hazard cell.
  Utils::Cardinal hazard_cell_dir;
  //! @brief Where the player is held while balancing. If anything else moves them (e.g. a knockback) the balance is cancelled.
  sf::Vector2f balance_pos;
  //! @brief Position the player returns to on recovery: 1px back from the balance position, against the movement
  //!        input (both axes only if the player was moving diagonally).
  sf::Vector2f safe_pos;
  //! @brief Time spent balancing so far.
  sf::Time elapsed{ sf::Time::Zero };
};

} // namespace Game::Cmp::Player

#endif // SRC_CMPS_PLAYER_HAZARDBALANCE_HPP__
