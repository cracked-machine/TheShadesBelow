#include <Components/AnimData.hpp>
#include <Components/Armed.hpp>
#include <Components/DestroyedObstacle.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Particle/BlockParticle.hpp>
#include <Components/Persistent/ArmedBlockColourBorder.hpp>
#include <Components/Persistent/ArmedBlockColourFill.hpp>
#include <Components/Persistent/ArmedOffDelay.hpp>
#include <Components/Persistent/ArmedOnDelay.hpp>
#include <Components/Persistent/FuseDelay.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/BombFactory.hpp>
#include <Systems/PersistSystem.hpp>

#include <Systems/BaseSystem.hpp>

namespace Game::Factory::Bomb
{

entt::entity create_armed( entt::registry &reg, entt::entity entity, Cmp::Armed::EpiCenter epi_center, int sequence, float zorder )
{

  // get persistent settings
  // clang-format off
  auto armed_block_colour_border = Sys::PersistSystem::get<Cmp::Persist::ArmedBlockColourBorder>(reg);
  auto armed_block_colour_fill = Sys::PersistSystem::get<Cmp::Persist::ArmedBlockColourFill>(reg);

  auto &fuse_delay = Sys::PersistSystem::get<Cmp::Persist::FuseDelay>( reg );
  auto &armed_on_delay = Sys::PersistSystem::get<Cmp::Persist::ArmedOnDelay>( reg );
  auto &armed_off_delay = Sys::PersistSystem::get<Cmp::Persist::ArmedOffDelay>( reg );
  auto new_fuse_delay = sf::seconds( fuse_delay.get_value() + ( sequence * armed_on_delay.get_value() ) );
  auto new_warning_delay = sf::seconds( armed_off_delay.get_value() + ( sequence * armed_off_delay.get_value() ) );
  // clang-format on

  // create the new armed entity
  // auto new_armed_entity = reg.create();
  // clang-format off
    reg.emplace_or_replace<Cmp::Armed>(
      entity,
      new_fuse_delay,
      new_warning_delay,
      (epi_center == Cmp::Armed::EpiCenter::YES) ? true : false,
      armed_block_colour_border,
      armed_block_colour_fill,
      sequence,
      epi_center
    );
  // clang-format on

  // reg.emplace_or_replace<Cmp::Position>( new_armed_entity, reg.get<Cmp::Position>( entity ).position, reg.get<Cmp::Position>( entity ).size );

  if ( epi_center == Cmp::Armed::EpiCenter::YES )
  {
    reg.emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{ .sprite_type = "sprite.item.bomb" } );
    reg.emplace_or_replace<Cmp::ZOrderValue>( entity, zorder );
    reg.emplace_or_replace<Cmp::Npc::NoPathFinding>( entity );
  }

  reg.emplace_or_replace<Cmp::Npc::NoPathFinding>( entity );

  return entity;
}

void destroy_armed( entt::registry &reg, entt::entity armed_entity )
{
  reg.remove<Cmp::Armed>( armed_entity );
  reg.remove<Cmp::AnimData>( armed_entity );
  reg.remove<Cmp::ZOrderValue>( armed_entity );
  if ( reg.all_of<Cmp::Npc::NoPathFinding>( armed_entity ) ) { reg.remove<Cmp::Npc::NoPathFinding>( armed_entity ); }
}

void add_detonated( entt::registry &reg, entt::entity armed_entity, [[maybe_unused]] Cmp::Position &armed_pos_cmp )
{
  reg.remove<Cmp::Npc::NoPathFinding>( armed_entity );
  reg.remove<Cmp::Player::NoPath>( armed_entity );
  reg.remove<Cmp::Particle::BlockParticle>( armed_entity );
  reg.emplace_or_replace<Cmp::AnimData>( armed_entity, Cmp::AnimData::Config{ .sprite_type = "sprite.graveyard.detonated" } );
  // floor decal: above the floor (-16), below dowsing rod guide lines (-12) and everything else
  reg.emplace_or_replace<Cmp::ZOrderValue>( armed_entity, -14.f );
  reg.emplace_or_replace<Cmp::DestroyedObstacle>( armed_entity );
}

} // namespace Game::Factory::Bomb
