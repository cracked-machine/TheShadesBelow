#ifndef SRC_FACTORY_RUINFACTORY_HPP__
#define SRC_FACTORY_RUINFACTORY_HPP__

#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>

#include <Systems/Stores/StoreKey.hpp>

// clang-format off
namespace Game::Sprites { class SpriteSheet; }
namespace Cmp { class Position; }
namespace Game::PathFinding { class SpatialHashGrid; }
// clang-format on

namespace Game::Factory::Ruin
{

// void create_staircase( entt::registry &reg, sf::Vector2f spawn_position, const Sprites::SpriteSheet &stairs_ms );

//! @brief Decorate the existing world position entity at `spawn_position` as a ruin bookcase obstacle.
//! Modifies the existing Position-owning entity rather than creating a new one, so pathfinding stays in sync.
//! @param reg
//! @param spawn_position
//! @param bookcase_ms
//! @param sprite_index
void create_bookcase( entt::registry &reg, sf::Vector2f spawn_position, const Sprites::SpriteSheet &bookcase_ms, size_t sprite_idx );

//! @brief Decorate `selected_entt` as a ruin cobweb obstacle.
//! @param reg
//! @param selected_entt Entity to attach the cobweb components to.
//! @param spawn_position
//! @param cobweb_ms
//! @param sprite_index
//! @param reserved_sm Reserves selected_entt's position so procgen doesn't overwrite it.
void create_cobweb( entt::registry &reg, entt::entity selected_entt, sf::Vector2f spawn_position, const Sprites::SpriteSheet &cobweb_ms,
                    size_t sprite_idx, PathFinding::SpatialHashGrid &reserved_sm );

//! @brief Create a shadow hand entt if none exists
//! @param reg
//! @param scene_dimensions Used to position the hand off-screen at the vertical center.
//! @param hand_ms
//! @param sprite_index
void create_shadow_hand( entt::registry &reg, sf::Vector2f scene_dimensions, const Sprites::SpriteSheet &hand_ms, size_t sprite_idx = 0 );

//! @brief Create a rune marker entity used to mark a hexagram position in a ruin.
//! @param reg
//! @param pos
//! @param zorder
//! @param sprite_idx
//! @param reserved_sm Reserves the new rune marker's position so procgen doesn't overwrite it.
//! @return The new rune marker entity.
entt::entity create_rune_marker( entt::registry &reg, Cmp::Position pos, float zorder, size_t sprite_idx, PathFinding::SpatialHashGrid &reserved_sm );

} // namespace Game::Factory::Ruin

#endif // SRC_FACTORY_RUINFACTORY_HPP__
