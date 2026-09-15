#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/Armed.hpp>
#include <Components/DeathPosition.hpp>
#include <Components/Direction.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/Npc/Container.hpp>
#include <Components/Npc/Drknox.hpp>
#include <Components/Npc/Friendly.hpp>
#include <Components/Npc/Ghost.hpp>
#include <Components/Npc/LerpSpeed.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/Priest.hpp>
#include <Components/Npc/Shockwave.hpp>
#include <Components/Npc/ShockwaveTimer.hpp>
#include <Components/Npc/Skeleton.hpp>
#include <Components/Npc/Spider.hpp>
#include <Components/Npc/Watchman.hpp>
#include <Components/Npc/WatchmanSearchlight.hpp>
#include <Components/Npc/Wisp.hpp>
#include <Components/Npc/Witch.hpp>
#include <Components/Persistent/NpcShockwaveFreq.hpp>
#include <Components/Persistent/NpcShockwaveResolution.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/UUID.hpp>
#include <Factory/Factory.hpp>
#include <Factory/LootFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Utils/Cardinal.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>

#include <entt/entity/entity.hpp>
#include <numbers>
#include <spdlog/spdlog.h>
#include <utility>

namespace Game::Factory::Npc
{

void create_npc_container( entt::registry &reg, entt::entity entt, Cmp::Position pos_cmp, Sprites::SpriteMetaType sprite_type,
                           std::size_t sprite_tile_idx, float zorder )
{
  reg.emplace_or_replace<Cmp::Armable>( entt );
  reg.emplace_or_replace<Cmp::Npc::Container>( entt );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entt, Cmp::AnimData::Config{ 
        .sprite_type = std::move(sprite_type), 
        .frame_index_offset = sprite_tile_idx,
        .enabled = true
  });
  // clang-format on
  reg.emplace_or_replace<Cmp::ZOrderValue>( entt, pos_cmp.position.y - zorder );
}

void destroy_npc_container( entt::registry &registry, entt::entity npc_container_entity, const PathFinding::SpatialHashGridSharedPtr &reserved_sm )
{
  if ( reserved_sm )
  {
    if ( auto *pos_cmp = registry.try_get<Cmp::Position>( npc_container_entity ) ) reserved_sm->remove( npc_container_entity, *pos_cmp );
  }
  registry.remove<Cmp::Armed>( npc_container_entity );
  registry.remove<Cmp::Npc::Container>( npc_container_entity );
  registry.remove<Cmp::AnimData>( npc_container_entity );
  registry.remove<Cmp::ZOrderValue>( npc_container_entity );
}

bool create_shockwave( entt::registry &registry, entt::entity npc_entt )
{
  // get the shockwave timer for the NPC
  auto *shockwave_timer = registry.try_get<Cmp::Npc::ShockwaveTimer>( npc_entt );
  if ( not shockwave_timer ) { return false; }

  // check cooldown on this NPC shockwave timer
  sf::Time sw_emit_freq{ sf::milliseconds( Sys::PersistSystem::get<Cmp::Persist::NpcShockwaveFreq>( registry ).get_value() ) };
  if ( *shockwave_timer > sw_emit_freq )
  {
    // create a new entity for adding the shockwave component to the NPC position
    auto *npc_pos = registry.try_get<Cmp::Position>( npc_entt );
    if ( not npc_pos )
    {
      SPDLOG_WARN( "Unable to get position from NPC entity" );
      return false;
    }
    auto npc_sw_entt = registry.create();
    int circle_resolution = Sys::PersistSystem::get<Cmp::Persist::NpcShockwaveResolution>( registry ).get_value();
    registry.emplace_or_replace<Cmp::Npc::Shockwave>( npc_sw_entt, npc_pos->getCenter(), circle_resolution );
    registry.emplace_or_replace<Cmp::Npc::ShockwaveTimer>( npc_entt );
    return true;
  }
  return false;
}

entt::entity create_npc( entt::registry &reg, entt::entity position_entity, const std::string &npc_type,
                         const PathFinding::SpatialHashGridSharedPtr &reserved_sm )
{

  auto *pos_cmp = reg.try_get<Cmp::Position>( position_entity );
  if ( not pos_cmp )
  {
    SPDLOG_ERROR( "Cannot add NPC entity {} without a Position component", static_cast<int>( position_entity ) );
    return entt::null;
  }

  // create a new entity for the NPC using the existing position
  auto new_pos_entity = reg.create();
  reg.emplace_or_replace<Cmp::Position>( new_pos_entity, pos_cmp->position, Constants::kGridSizePxF );
  reg.emplace_or_replace<Cmp::Armable>( new_pos_entity );
  reg.emplace_or_replace<Cmp::ZOrderValue>( new_pos_entity, pos_cmp->position.y );
  reg.emplace_or_replace<Cmp::Direction>( new_pos_entity, sf::Vector2f{ 0, 0 } );
  reg.emplace_or_replace<Cmp::UUID>( new_pos_entity, Cmp::UUID::generate() );
  auto npc_cmp = Sys::NpcStore::instance().get_item( npc_type );
  reg.emplace_or_replace<Cmp::Npc::NPC>( new_pos_entity, npc_cmp );

  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( new_pos_entity, Cmp::AnimData::Config{ 
        .sprite_type = npc_cmp.sprite_type_list.front(), 
        .framerate = npc_cmp.m_frame_rate,
        .enabled = true
  });
  // clang-format on

  SPDLOG_DEBUG( "Spawned NPC {} ({}) at position ({}, {})", static_cast<int>( new_pos_entity ), npc_type, pos_cmp->position.x, pos_cmp->position.y );

  if ( npc_type == "npc.ghost" )
  {
    reg.emplace_or_replace<Cmp::Npc::Ghost>( new_pos_entity );
    reg.emplace_or_replace<Cmp::Npc::LerpSpeed>( new_pos_entity, npc_cmp.m_lerp_speed );
  }
  else if ( npc_type == "npc.nightwatchman" )
  {
    reg.emplace_or_replace<Cmp::Npc::Watchman>( new_pos_entity );
    reg.emplace_or_replace<Cmp::Npc::LerpSpeed>( new_pos_entity, npc_cmp.m_lerp_speed );
    // randomize the initial sweep phase and idle-facing direction so multiple Watchmen don't
    // sweep/patrol in lockstep
    Cmp::RandomFloat sweep_phase_rng( 0.f, 2.f * std::numbers::pi_v<float> );
    Cmp::RandomInt idle_direction_rng( 0, 3 );
    reg.emplace_or_replace<Cmp::Npc::WatchmanSearchlight>(
        new_pos_entity,
        Cmp::Npc::WatchmanSearchlight{ .sweep_phase = sweep_phase_rng.gen(),
                                       .idle_direction = Utils::Cardinal( static_cast<Utils::Cardinal::Value>( idle_direction_rng.gen() ) ) } );
    Factory::Npc::destroy_npc_container( reg, position_entity, reserved_sm );
  }
  else if ( npc_type == "npc.skeleton" )
  {
    reg.emplace_or_replace<Cmp::Npc::Skeleton>( new_pos_entity );
    reg.emplace_or_replace<Cmp::Npc::LerpSpeed>( new_pos_entity, npc_cmp.m_lerp_speed );
    Factory::Npc::destroy_npc_container( reg, position_entity, reserved_sm );
  }
  else if ( npc_type == "npc.priest" )
  {
    reg.emplace_or_replace<Cmp::Npc::Priest>( new_pos_entity );
    reg.emplace_or_replace<Cmp::Npc::LerpSpeed>( new_pos_entity, npc_cmp.m_lerp_speed );
    reg.emplace_or_replace<Cmp::Npc::ShockwaveTimer>( new_pos_entity );
    Factory::Npc::create_shockwave( reg, new_pos_entity );
  }
  else if ( npc_type == "npc.witch" )
  {
    reg.emplace_or_replace<Cmp::Npc::Witch>( new_pos_entity );
    reg.emplace_or_replace<Cmp::Npc::LerpSpeed>( new_pos_entity, npc_cmp.m_lerp_speed );
  }
  else if ( npc_type == "npc.wisp" )
  {
    reg.emplace_or_replace<Cmp::Npc::Wisp>( new_pos_entity );
    reg.emplace_or_replace<Cmp::Npc::LerpSpeed>( new_pos_entity, npc_cmp.m_lerp_speed );
    reg.emplace_or_replace<Cmp::Direction>( new_pos_entity, sf::Vector2f{ 1, 1 } );
    reg.emplace_or_replace<Cmp::Npc::Friendly>( new_pos_entity );
  }
  else if ( npc_type == "npc.spider" )
  {
    reg.emplace_or_replace<Cmp::Npc::Spider>( new_pos_entity );
    reg.emplace_or_replace<Cmp::Npc::LerpSpeed>( new_pos_entity, npc_cmp.m_lerp_speed );
    reg.emplace_or_replace<Cmp::Direction>( new_pos_entity, sf::Vector2f{ 1, 1 } );
  }
  else if ( npc_type == "npc.drknox" )
  {
    reg.emplace_or_replace<Cmp::Npc::Drknox>( new_pos_entity );
    reg.emplace_or_replace<Cmp::Npc::Friendly>( new_pos_entity );
  }
  Utils::Player::apply_action_from_npc_store<Cmp::SpawnAction>( reg, npc_type );
  return new_pos_entity;
}

entt::entity destroy_npc( entt::registry &reg, entt::entity npc_entity )
{

  // check for position component
  entt::entity loot_entity = entt::null;
  auto *npc_pos_cmp = reg.try_get<Cmp::Position>( npc_entity );
  if ( not npc_pos_cmp ) { SPDLOG_WARN( "Cannot process loot drop for NPC entity {} without a Position component", static_cast<int>( npc_entity ) ); }

  // apply destroy action effect to player
  auto *npc_cmp = reg.try_get<Cmp::Npc::NPC>( npc_entity );
  if ( npc_cmp )
  {
    auto action_timer_pair = npc_cmp->actions.at( std::type_index( typeid( Cmp::DestroyAction ) ) );
    Utils::Player::get_stats( reg ).apply( action_timer_pair.action );
  }

  // kill npc once we are done
  reg.remove<Cmp::Npc::NPC>( npc_entity );
  reg.remove<Cmp::Position>( npc_entity );
  reg.remove<Cmp::Direction>( npc_entity );
  reg.remove<Cmp::AnimData>( npc_entity );
  reg.remove<Cmp::ZOrderValue>( npc_entity );

  return loot_entity;
}

entt::entity create_npc_death_anim( entt::registry &reg, Cmp::Position npc_pos_cmp, const Sprites::SpriteMetaType &death_anim )
{
  auto npc_death_entity = reg.create();
  reg.emplace<Cmp::Position>( npc_death_entity, npc_pos_cmp.position, npc_pos_cmp.size );
  reg.emplace_or_replace<Cmp::DeathPosition>( npc_death_entity, npc_pos_cmp.position, npc_pos_cmp.size );

  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( npc_death_entity, Cmp::AnimData::Config{ 
        .sprite_type = death_anim, 
        .framerate = 0.1,
        .enabled = true,
        .anim_type =  Cmp::AnimType::ONESHOTRESET
  });
  // clang-format on

  reg.emplace_or_replace<Cmp::ZOrderValue>( npc_death_entity, npc_pos_cmp.position.y );
  return npc_death_entity;
}

void remove_npc_death_anim( entt::registry &registry, entt::entity entity )
{
  registry.remove<Cmp::DeathPosition>( entity );
  registry.remove<Cmp::AnimData>( entity );
  registry.remove<Cmp::ZOrderValue>( entity );
  registry.remove<Cmp::Position>( entity );
  SPDLOG_DEBUG( "Explosion animation complete, removing component from entity {}", static_cast<int>( entity ) );
}

} // namespace Game::Factory::Npc