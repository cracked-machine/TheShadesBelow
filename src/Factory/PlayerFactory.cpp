#include <Components/AbsoluteAlpha.hpp>
#include <Components/AbsoluteOffset.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/AnimData.hpp>
#include <Components/DeathPosition.hpp>
#include <Components/Direction.hpp>
#include <Components/Inventory/FlashUIHealth.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/ScryingBall.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/LastDirection.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Particle/SpriteBase.hpp>
#include <Components/Persistent/BlastRadius.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/BlastRadius.hpp>
#include <Components/Player/CadaverCount.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Player/ExtraLife.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/LastGraveyardPosition.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/MovementDelta.hpp>
#include <Components/Player/TorchRadius.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/SpawnArea.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Components/Stats/SacrificeAction.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlantFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/SpriteFactory.hpp>
#include <Factory/ToxicityFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>
#include <spdlog/spdlog.h>

namespace Game::Factory::Player
{

void create_player( entt::registry &reg, const PathFinding::SpatialHashGridSharedPtr &reserved_sm )
{
  SPDLOG_DEBUG( "Creating player entity" );
  auto entity = reg.create();

  // start position must be pixel coordinates within the screen resolution (kDisplaySize),
  // but also grid aligned (kMapGridSize) to avoid collision detection errors.
  // So we must recalc start position to the nearest grid position here
  auto start_pos = Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( reg );
  start_pos = Utils::snap_to_grid( start_pos );
  Cmp::Position player_pos( start_pos, Constants::kGridSizePxF );
  reg.emplace_or_replace<Cmp::Position>( entity, player_pos );
  auto &blast_radius = Sys::PersistSystem::get<Cmp::Persist::BlastRadius>( reg );
  reg.emplace_or_replace<Cmp::Player::Character>( entity );
  if ( reserved_sm ) reserved_sm->insert( entity, player_pos );
  reg.emplace_or_replace<Cmp::Player::BlastRadius>( entity, blast_radius.get_value() );
  reg.emplace_or_replace<Cmp::PlayerStats>( entity, Cmp::Stats::Health{ 100 }, Cmp::Stats::Fear{ 0 }, Cmp::Stats::Despair{ 0 },
                                            Cmp::Stats::Infamy{ 0 }, Cmp::Stats::Toxicity{ 0 }, Cmp::Stats::Luck{ 50 } );

  reg.emplace_or_replace<Cmp::Direction>( entity, sf::Vector2f{ 0, 0 } );
  reg.emplace_or_replace<Cmp::LastDirection>( entity, sf::Vector2f{ 0, 0 } );
  reg.emplace_or_replace<Cmp::Player::MovementDelta>( entity );
  reg.emplace_or_replace<Cmp::Player::Footstep>( entity, Cmp::Player::Footstep::Type::GRASS );
  reg.emplace_or_replace<Cmp::Toxicity::Toxidrome>( entity, Factory::Toxicity::ToxidromeBuilder<>{}.build() );

  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{ 
        .sprite_type = "sprite.player.walk.south", 
        .current_frame = 1,  // middle cell of three frames: standing still
        .enabled = true
  });
  // clang-format on

  reg.emplace_or_replace<Cmp::Player::CadaverCount>( entity, 0 );
  reg.emplace_or_replace<Cmp::Player::Wealth>( entity, 0 );
  reg.emplace_or_replace<Cmp::Player::Mortality>( entity, Cmp::Player::Mortality::State::ALIVE );
  reg.emplace_or_replace<Cmp::Player::Curse>( entity, false );
  reg.emplace_or_replace<Cmp::Player::LevelDepth>( entity, 1 );
  reg.emplace_or_replace<Cmp::TorchRadius>( entity, 32.f );

  reg.emplace_or_replace<Cmp::ZOrderValue>( entity, start_pos.y ); // z-order based on y-position
  reg.emplace_or_replace<Cmp::AbsoluteAlpha>( entity, 255 );       // fully opaque
  reg.emplace_or_replace<Cmp::AbsoluteRotation>( entity, 0 );
  add_inventory( reg, "item.pickaxe" );
}

entt::entity add_spawn_area( entt::registry &reg, entt::entity entity, Sprites::SpriteFactory &sfactory, float zorder )
{
  // We need to reserve these positions for the player start area, dont add NpcNoPathFinding.
  // We want NPCs to pathfind player within spawn. We block NPCs from entering spawn directly in NpcSystem::update_pathfinding.
  reg.emplace_or_replace<Cmp::SpawnArea>( entity, false );
  auto [_, idx] = sfactory.get_random_type_and_texture_index( { "sprite.graveyard.playerspawn" } );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{ 
        .sprite_type = "sprite.graveyard.playerspawn", 
        .frame_index_offset = idx,
        .enabled = true
  });
  // clang-format on

  reg.emplace_or_replace<Cmp::ZOrderValue>( entity, zorder );
  return entity;
}

void create_player_death_anim( entt::registry &reg, Cmp::Position player_pos_cmp, const Sprites::SpriteSheet &sprite )
{
  auto player_blood_splat_entity = reg.create();
  sf::Vector2f offset;
  if ( ( sprite.get_sprite_size().x == Constants::kGridSizePxF.x ) and ( sprite.get_sprite_size().y == Constants::kGridSizePxF.y ) )
  {
    offset = sf::Vector2f{ 0, 0 };
  }
  else { offset = sprite.get_sprite_size() / 2.f; }
  reg.emplace_or_replace<Cmp::Position>( player_blood_splat_entity, player_pos_cmp.position - offset, player_pos_cmp.size );
  reg.emplace_or_replace<Cmp::DeathPosition>( player_blood_splat_entity, player_pos_cmp.position - offset, player_pos_cmp.size );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( player_blood_splat_entity, Cmp::AnimData::Config{ 
        .sprite_type = sprite.get_sprite_type(), 
        .framerate = 0.1,
        .enabled = true,
        .anim_type = Cmp::AnimType::ONESHOTHOLD
  });
  // clang-format on
  reg.emplace_or_replace<Cmp::ZOrderValue>( player_blood_splat_entity, player_pos_cmp.position.y * 3 ); // always infront
}

void add_inventory( entt::registry &reg, const std::string &item )
{
  auto inventory_entity = reg.create();
  reg.emplace_or_replace<Cmp::PlayerInventorySlot>( inventory_entity, Sys::ItemStore::instance().get_item( item ) );
  if ( item.contains( "axe" ) or item.contains( "shovel" ) ) { reg.emplace_or_replace<Cmp::Inventory::WearLevel>( inventory_entity, 100.f ); }
  if ( item.contains( "scryingball" ) )
  {
    Cmp::SeeingStone sb;
    sb.target = Game::Cmp::SeeingStone::random_pick( {} );
    reg.emplace_or_replace<Cmp::SeeingStone>( inventory_entity, sb );
  }
  if ( item.contains( "candle" ) ) { reg.emplace_or_replace<Cmp::UUID>( inventory_entity, Cmp::UUID::generate() ); }

  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( inventory_entity, Cmp::AnimData::Config{ 
        .sprite_type = Sys::ItemStore::instance().get_item( item ).sprite_type, 
        .enabled = true
  });
  // clang-format on
}

void destroy_inventory( entt::registry &reg, const Sprites::SpriteMetaType &type )
{
  auto inventory_view = reg.view<Cmp::PlayerInventorySlot>();
  for ( auto [inventory_entt, inventory_cmp] : inventory_view.each() )
  {
    if ( inventory_cmp.m_item.item_type == type ) reg.destroy( inventory_entt );
  }
}

Cmp::Position add_player_last_graveyard_pos( entt::registry &reg, Cmp::Position &last_known_pos, [[maybe_unused]] sf::Vector2f offset )
{
  SPDLOG_INFO( "Player will re-enter grave yard at {},{}", last_known_pos.position.x, last_known_pos.position.y );
  auto player_entt = Utils::Player::get_entity( reg );
  reg.emplace_or_replace<Cmp::Player::LastGraveyardPosition>( player_entt, last_known_pos.position, last_known_pos.size );
  return last_known_pos;
}

void remove_player_last_graveyard_pos( entt::registry &reg )
{
  auto player_entt = Utils::Player::get_entity( reg );
  reg.remove<Cmp::Player::LastGraveyardPosition>( player_entt );
}

void remove_player_extra_life( entt::registry &reg )
{
  for ( auto [extra_life_entt, extra_life_cmp] : reg.view<Cmp::Player::ExtraLife>().each() )
  {
    reg.remove<Cmp::Player::ExtraLife>( extra_life_entt );
  }

  for ( auto [flash_entt, flash_cmp] : reg.view<Cmp::FlashUIHealth>().each() )
  {
    reg.destroy( flash_entt );
  }

  for ( auto [ps_entt, ps_cmp] : reg.view<Cmp::Particle::SpriteOwner>().each() )
  {
    if ( ps_cmp.sprite->get_tag() == "crypt.altar.particle.active" ) reg.destroy( ps_entt );
  }

  for ( auto [altar_mb_entt, altar_mb_cmp, altar_anim_cmp] : reg.view<Cmp::Altar::MultiBlock, Cmp::AnimData>().each() )
  {
    if ( altar_anim_cmp.m_sprite_type == "sprite.crypt.altar.active" ) { altar_anim_cmp.m_sprite_type = "sprite.crypt.altar.inactive"; }
  }
}

} // namespace Game::Factory::Player
