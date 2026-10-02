#include <Components/AnimData.hpp>
#include <Components/NoMoveDest.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Obstacle.hpp>
#include <Components/Particle/BlockParticle.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/WallFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Utils/Constants.hpp>

#include <entt/entity/registry.hpp>

namespace Game::Factory::Wall
{

entt::entity add_wall_entity( entt::registry &reg, const sf::Vector2f &pos, const Sprites::SpriteSheet &ms, std::size_t sprite_index )
{
  auto entity = reg.create();
  reg.emplace_or_replace<Cmp::Position>( entity, pos, Constants::kGridSizePxF );
  reg.emplace_or_replace<Cmp::Wall>( entity );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{ 
      .sprite_type = ms.type(), 
      .frame_index_offset = static_cast<size_t>(sprite_index),
      .enabled = true
  });
  // clang-format on
  Cmp::ZOrderValue zorder_cmp( 0 );
  if ( ms.zorder( sprite_index ) != 0 ) { zorder_cmp.set( ms.zorder( sprite_index ) ); }
  else { zorder_cmp.set( pos.y + ms.sprite_size().y ); }
  reg.emplace_or_replace<Cmp::ZOrderValue>( entity, zorder_cmp );
  return entity;
}

entt::entity add_reservedposition( entt::registry &reg, const sf::Vector2f &pos )
{
  auto entity = reg.create();
  reg.emplace_or_replace<Cmp::Position>( entity, pos, Constants::kGridSizePxF );
  return entity;
}

void add_solid_player( entt::registry &reg, sf::FloatRect rect, PathFinding::SpatialHashGrid &reserved_sm )
{
  // Mark any existing world position entities substantially covered by this rect as reserved -
  // checking the tile's center (rather than any overlap at all) avoids a thin boundary-shaping
  // solid object that merely grazes a tile's edge from reserving an otherwise-walkable tile.
  // Don't mark player position as Cmp::Player::NoPath or they won't be able to move!
  for ( auto [entt, pos_cmp] : reg.view<Cmp::Position>( entt::exclude<Cmp::Player::Character> ).each() )
  {
    if ( rect.contains( pos_cmp.getCenter() ) )
    {
      reserved_sm.insert( entt, pos_cmp );
      reg.emplace_or_replace<Cmp::Player::NoPath>( entt );
      reg.emplace_or_replace<Cmp::Particle::BlockParticle>( entt );
    }
  }
}

void add_solid_npc( entt::registry &reg, sf::FloatRect rect, PathFinding::SpatialHashGrid &reserved_sm )
{
  // Mark any existing world position entities substantially covered by this rect as reserved -
  // see add_solid_player() for why center-containment is used instead of any-overlap.
  for ( auto [entt, pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    if ( rect.contains( pos_cmp.getCenter() ) )
    {
      reserved_sm.insert( entt, pos_cmp );
      reg.emplace_or_replace<Cmp::Npc::NoPathFinding>( entt );
    }
  }
}

void add_no_move_dest( entt::registry &reg, sf::FloatRect rect, PathFinding::SpatialHashGrid &reserved_sm )
{
  // Mark any existing world position entities substantially covered by this rect as reserved -
  // see add_solid_player() for why center-containment is used instead of any-overlap.
  for ( auto [entt, pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    if ( rect.contains( pos_cmp.getCenter() ) )
    {
      reserved_sm.insert( entt, pos_cmp );
      reg.emplace_or_replace<Cmp::NoMoveDest>( entt );
    }
  }
}

} // namespace Game::Factory::Wall