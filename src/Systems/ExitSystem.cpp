#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
#include <Components/Exit.hpp>
#include <Components/Grave/ExitMultiBlock.hpp>
#include <Components/Grave/ExitSegment.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Persistent/ExitKeyRequirement.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>

#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/SpawnArea.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/CrowDeliveryEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/ExitSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Vector2.hpp>
#include <entt/entity/entity.hpp>
#include <stdexcept>

namespace Game::Sys

{
ExitSystem::ExitSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, entt::dispatcher &scenemanager_event_dispatcher )
    : BaseSystem( reg, window, sound_bank ),
      m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
{
  SPDLOG_DEBUG( "ExitSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&ExitSystem::on_player_action>( this );
}

void ExitSystem::update( [[maybe_unused]] sf::Time dt )
{
  if ( Utils::is_current_scene( reg(), Cmp::SceneSettings::SceneId::GRAVEYARD ) )
  {
    update_mb_zorder<Cmp::Grave::ExitMultiBlock, Cmp::Grave::ExitSegment>();
    check_exit_collision();
  }
}

void ExitSystem::create_exit( sf::Vector2f pos )
{
  if ( not reg().view<Cmp::Exit>().empty() ) { throw std::runtime_error( "create_exit() called but an exit already exists in this scene" ); }

  const auto &kGraveExitSpritesheet = Sys::SpriteStore::instance().get( "sprite.graveyard.exit.locked" );
  auto reserved_sm = m_reserved_sm.lock();

  auto world_tile_sm = m_world_tile_sm.lock();

  // Remove the existing world/obstacle entity at the exit origin first
  const Cmp::Position origin_cell( pos, Constants::kGridSizePxF );
  entt::entity world_entt = entt::null;
  if ( world_tile_sm )
  {
    for ( auto tile_entt : world_tile_sm->at( origin_cell ) )
    {
      if ( reg().valid( tile_entt ) )
      {
        world_entt = tile_entt;
        break;
      }
    }
  }
  else { world_entt = Utils::get_world_pos_entt( reg(), origin_cell ); }
  if ( world_entt != entt::null ) { Factory::Obstacle::remove_obstacle( reg(), world_entt, Factory::Obstacle::DeleteExtras::Yes, reserved_sm ); }

  Factory::Multiblock::add_multiblock_with_segments<Cmp::Grave::ExitMultiBlock, Cmp::Grave::ExitSegment>( reg(), pos, kGraveExitSpritesheet, 0, 0,
                                                                                                          reserved_sm.get(), world_tile_sm.get() );
  SPDLOG_INFO( "Added {} to ({}, {})", kGraveExitSpritesheet.type(), pos.x, pos.y );
}

void ExitSystem::on_player_action( Events::PlayerActionEvent ev )
{
  if ( ev.action != Events::PlayerActionEvent::GameActions::ACTIVATE ) return;
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type == "item.exitkey" ) { unlock_exit(); }
  else if ( auto *exit_cmp = player_at_exit(); exit_cmp and exit_cmp->m_locked )
  {
    // no key in hand: send a crow to fetch it (NpcSystem ignores this if the key is in view or already on its way)
    get_systems_event_queue().enqueue( Events::CrowDeliveryEvent( "item.exitkey" ) );
  }
}

Cmp::Exit *ExitSystem::player_at_exit()
{
  auto player_pos = Utils::Player::get_position( reg() );
  // the player can't intersect the exit itself so expand their hitbox to facilitate collision detection
  auto player_hitbox = Cmp::RectBounds::scaled( player_pos, 5.f );
  for ( auto [entity, exit_cmp, exit_pos_cmp] : reg().view<Cmp::Exit, Cmp::Position>().each() )
  {
    if ( player_hitbox.findIntersection( exit_pos_cmp ) ) return &exit_cmp;
  }
  return nullptr;
}

void ExitSystem::unlock_exit()
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( not inventory_type.contains( "exitkey" ) ) return;

  auto player_pos = Utils::Player::get_position( reg() );
  auto player_hitbox = Cmp::RectBounds::scaled( player_pos, 5.f );

  for ( auto [entity, exit_cmp, exit_pos_cmp] : reg().view<Cmp::Exit, Cmp::Position>().each() )
  {
    if ( not player_hitbox.findIntersection( exit_pos_cmp ) ) continue;

    exit_cmp.m_locked = false;

    for ( auto [exit_mb_entt, exit_mb_cmp, anim_cmp] : reg().view<Cmp::Grave::ExitMultiBlock, Cmp::AnimData>().each() )
    {
      if ( not exit_pos_cmp.findIntersection( exit_mb_cmp ) ) continue;
      anim_cmp.m_sprite_type = "sprite.graveyard.exit.unlocked";
      Factory::Multiblock::detail::update_segments<Cmp::Grave::ExitMultiBlock, Cmp::Grave::ExitSegment>(
          reg(), Sys::SpriteStore::instance().get( "sprite.graveyard.exit.unlocked" ), exit_mb_entt, exit_mb_cmp );
      break;
    }

    if ( m_sound_bank.get_effect( "secret" ).getStatus() == sf::Sound::Status::Stopped ) m_sound_bank.get_effect( "secret" ).play();
    Factory::Player::destroy_inventory( reg(), "item.exitkey" );
  }
}

void ExitSystem::update_exit_zorder()
{
  auto player_pos = Utils::Player::get_position( reg() );

  for ( auto [mb_entt, mb_cmp, mb_z_cmp] : reg().view<Cmp::Grave::ExitMultiBlock, Cmp::ZOrderValue>().each() )
  {
    if ( not player_pos.findIntersection( mb_cmp ) ) continue;
    auto segment_view = reg().view<Cmp::Grave::ExitSegment, Cmp::Position, Cmp::ZOrderValue>();
    for ( auto [segment_entt, segment_cmp, segment_pos_cmp, segment_z_cmp] : segment_view.each() )
    {
      if ( not player_pos.findIntersection( segment_pos_cmp ) ) continue;
      mb_z_cmp.set( segment_z_cmp.get() );
      SPDLOG_DEBUG( "Updated zorder to {}", segment_z_cmp.get() );
    }
  }
}

void ExitSystem::check_exit_collision()
{
  // no scene transitions mid-flight - the player must land and walk in
  if ( Utils::Player::is_flying( reg() ) ) return;

  auto exit_view = reg().view<Cmp::Exit, Cmp::Position>();
  for ( auto [entity, exit_cmp, exit_pos_cmp] : exit_view.each() )
  {
    if ( exit_cmp.m_locked ) { continue; }

    // allow the player to reach the exit from the front but not the back
    Cmp::Position adjusted_exit_pos( { exit_pos_cmp.x(), exit_pos_cmp.y() + 8 }, exit_pos_cmp.size );
    if ( Utils::Player::get_position( reg() ).findIntersection( adjusted_exit_pos ) )
    {
      SPDLOG_INFO( "Player reached the exit zone!" );
      m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::LEVEL_COMPLETE );
      Factory::Player::remove_player_last_graveyard_pos( reg() );
    }
  }
}

} // namespace Game::Sys