#ifndef SRC_FACTORY_WALLFACTORY_HPP__
#define SRC_FACTORY_WALLFACTORY_HPP__
#include <entt/entity/fwd.hpp>

#include <SFML/System/Vector2.hpp>
#include <Sprites/SpriteMetaType.hpp>

namespace Game::Sprites
{
class SpriteSheet;
} // namespace Game::Sprites

namespace Game::PathFinding
{
class SpatialHashGrid;
}

namespace Game::Factory::Wall
{

//! @brief Create a wall entity: sprite-decorated and blocking pathfinding. The caller is
//! responsible for reserving the entity's position (it is not reserved here).
//! @param reg
//! @param pos
//! @param ms
//! @param sprite_index
//! @return entt::entity
entt::entity add_wall_entity( entt::registry &reg, const sf::Vector2f &pos, const Sprites::SpriteSheet &ms, std::size_t sprite_index );

//! @brief Create a bare position entity, without any sprite or wall component. The caller is
//! responsible for reserving the entity's position (it is not reserved here).
//! @param reg
//! @param pos
entt::entity add_reservedposition( entt::registry &reg, const sf::Vector2f &pos );

//! @brief Mark every existing world position entity overlapping `rect` as reserved and blocking player movement.
//! @param reg
//! @param rect
//! @param reserved_sm Reserves each matching entity's position.
void add_solid_player( entt::registry &reg, sf::FloatRect rect, PathFinding::SpatialHashGrid &reserved_sm );

//! @brief Mark every existing world position entity overlapping `rect` as reserved and blocking NPC pathfinding.
//! @param reg
//! @param rect
//! @param reserved_sm Reserves each matching entity's position.
void add_solid_npc( entt::registry &reg, sf::FloatRect rect, PathFinding::SpatialHashGrid &reserved_sm );

//! @brief Mark every existing world position entity overlapping `rect` as reserved and an invalid movement destination.
//! @param reg
//! @param rect
//! @param reserved_sm Reserves each matching entity's position.
void add_no_move_dest( entt::registry &reg, sf::FloatRect rect, PathFinding::SpatialHashGrid &reserved_sm );
} // namespace Game::Factory::Wall

#endif // SRC_FACTORY_WALLFACTORY_HPP__
