#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
#include <Components/Exit.hpp>
#include <Components/Grave/ExitMultiBlock.hpp>
#include <Components/Grave/ExitSegment.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Persistent/ExitKeyRequirement.hpp>
#include <Components/Persistent/MaxNumAltars.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>

#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/ExitSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Vector2.hpp>
#include <entt/entity/entity.hpp>
#include <stdexcept>

namespace Game::Sys

{
ExitSystem::ExitSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank,
                        entt::dispatcher &scenemanager_event_dispatcher )
    : BaseSystem( reg, window, sprite_factory, sound_bank ),
      m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
{
  SPDLOG_DEBUG( "ExitSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&ExitSystem::on_player_action>( this );
}

void ExitSystem::create_exit()
{
  if ( not reg().view<Cmp::Exit>().empty() ) { throw std::runtime_error( "create_exit() called but an exit already exists in this scene" ); }

  entt::entity selected_entity = entt::null;
  Cmp::Position selected_pos_cmp( { 0, 0 }, { 0, 0 } );
  const auto &kGraveExitSpritesheet = m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.exit.locked" );
  const int kMaxAttempts = 100;

  auto reserved_sm = m_reserved_sm.lock();
  bool found_valid_position = false;
  for ( int attempt_count = 0; attempt_count < kMaxAttempts; ++attempt_count )
  {
    auto exclude_list = Utils::Rnd::ExcludePack<Cmp::Wall, Cmp::Exit, Cmp::Player::Character, Cmp::Npc::NPC>{};
    auto [rand_entity, rand_pos_cmp] = Utils::Rnd::get_random_position( reg(), {}, exclude_list, 0 );
    if ( reserved_sm && not reserved_sm->at( rand_pos_cmp ).empty() ) continue;
    Cmp::Position multiblock_hitbox( rand_pos_cmp.position, kGraveExitSpritesheet.get_px_size() );

    bool collides_with_wall = false;
    for ( auto [wall_entt, wall_cmp, wall_pos_cmp] : reg().view<Cmp::Wall, Cmp::Position>().each() )
    {
      if ( multiblock_hitbox.findIntersection( wall_pos_cmp ) )
      {
        collides_with_wall = true;
        break;
      }
    }
    if ( not collides_with_wall )
    {
      selected_entity = rand_entity;
      selected_pos_cmp = rand_pos_cmp;
      found_valid_position = true;
      break;
    }
  }
  if ( not found_valid_position ) { throw std::runtime_error( "Unable to spawn graveyard exit" ); }

  // Remove the existing wall obstacle first
  Factory::Obstacle::remove_obstacle( reg(), selected_entity, Factory::Obstacle::DeleteExtras::Yes, reserved_sm );

  Factory::Multiblock::add_multiblock_with_segments<Cmp::Grave::ExitMultiBlock, Cmp::Grave::ExitSegment>(
      reg(), selected_pos_cmp.position, kGraveExitSpritesheet, 0, 0, reserved_sm.get() );
  SPDLOG_INFO( "Exit spawned at position ({}, {})", selected_pos_cmp.position.x, selected_pos_cmp.position.y );
}

void ExitSystem::on_player_action( Events::PlayerActionEvent ev )
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( ev.action == Events::PlayerActionEvent::GameActions::ACTIVATE && inventory_type == "item.exitkey" ) { unlock_exit(); }
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
          reg(), m_sprite_factory.get_spritesheet_by_type( "sprite.graveyard.exit.unlocked" ), exit_mb_entt, exit_mb_cmp );
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
      mb_z_cmp.setZOrder( segment_z_cmp.getZOrder() );
      SPDLOG_DEBUG( "Updated zorder to {}", segment_z_cmp.getZOrder() );
    }
  }
}

void ExitSystem::check_exit_collision()
{

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