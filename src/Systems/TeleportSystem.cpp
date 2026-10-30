#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/AnimData.hpp>
#include <Components/Npc/Mimic.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/PendingTeleport.hpp>
#include <Components/Player/CameraHold.hpp>
#include <Components/Player/Character.hpp>
#include <Components/TeleportAnim.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/TeleportEntityEvent.hpp>
#include <Events/UpdateNpcPositionEvent.hpp>
#include <Events/UpdatePlayerPositionEvent.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/TeleportSystem.hpp>
#include <Utils/Constants.hpp>

#include <algorithm>
#include <vector>

namespace Game::Sys
{

TeleportSystem::TeleportSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  SPDLOG_DEBUG( "TeleportSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::TeleportEntityEvent>().connect<&TeleportSystem::on_teleport_entity_event>( this );
}

void TeleportSystem::update( sf::Time dt )
{
  std::vector<entt::entity> release_list;
  for ( auto [player_entt, camera_hold_cmp] : reg().view<Cmp::Player::CameraHold>().each() )
  {
    camera_hold_cmp.remaining -= dt;
    if ( camera_hold_cmp.remaining <= sf::Time::Zero ) release_list.push_back( player_entt );
  }
  for ( auto player_entt : release_list )
  {
    reg().remove<Cmp::Player::CameraHold>( player_entt );
    SPDLOG_INFO( "CameraHold expired, camera released" );
  }

  // fade out every actor awaiting a teleport, and collect the delayed teleports that are now due
  std::vector<Events::TeleportEntityEvent> due_list;
  for ( auto [actor_entt, pending_cmp] : reg().view<Cmp::PendingTeleport>().each() )
  {
    pending_cmp.elapsed += dt;
    const float progress = std::min( pending_cmp.elapsed / pending_cmp.duration, 1.0f );
    reg().emplace_or_replace<Cmp::AbsoluteAlpha>( actor_entt, static_cast<uint8_t>( 255 * ( 1.0f - progress ) ) );
    if ( pending_cmp.event and pending_cmp.elapsed >= pending_cmp.duration ) due_list.push_back( *pending_cmp.event );
  }
  for ( const auto &due_event : due_list )
  {
    reg().remove<Cmp::PendingTeleport>( due_event.m_entity );
    on_teleport_entity_event( due_event );
  }

  // NPCs only get an alpha from the fade, so drop it once their pending teleport is done or cancelled.
  // The player's alpha is restored by PlayerSystem::blink_player, the mimic's by NpcSystem's despawn flash
  std::vector<entt::entity> restore_list;
  for ( auto [npc_entt, alpha_cmp, npc_cmp] :
        reg().view<Cmp::AbsoluteAlpha, Cmp::Npc::NPC>( entt::exclude<Cmp::PendingTeleport, Cmp::Npc::Mimic> ).each() )
  {
    restore_list.push_back( npc_entt );
  }
  for ( auto npc_entt : restore_list )
  {
    reg().remove<Cmp::AbsoluteAlpha>( npc_entt );
  }

  std::vector<entt::entity> kill_list;
  for ( auto [anim_entt, teleport_anim_cmp, anim_cmp] : reg().view<Cmp::TeleportAnim, Cmp::AnimData>().each() )
  {
    if ( not anim_cmp.m_enabled ) kill_list.push_back( anim_entt );
  }
  for ( auto anim_entt : kill_list )
  {
    reg().destroy( anim_entt );
  }
}

void TeleportSystem::on_teleport_entity_event( const Events::TeleportEntityEvent &event )
{
  const entt::entity entity = event.m_entity;
  if ( not reg().valid( entity ) ) return;
  auto *pos_cmp = reg().try_get<Cmp::Position>( entity );
  if ( not pos_cmp ) return;

  if ( not reg().any_of<Cmp::Player::Character, Cmp::Npc::NPC>( entity ) )
  {
    SPDLOG_WARN( "Cannot teleport entity {}: it is neither the player nor an NPC", static_cast<uint32_t>( entity ) );
    return;
  }

  if ( event.m_delay > sf::Time::Zero )
  {
    if ( reg().all_of<Cmp::PendingTeleport>( entity ) )
    {
      SPDLOG_WARN( "Cannot teleport entity {}: a teleport is already pending", static_cast<uint32_t>( entity ) );
      return;
    }
    // fade the entity out first, update() performs the move once the delay has elapsed
    Events::TeleportEntityEvent due_event = event;
    due_event.m_delay = sf::Time::Zero;
    reg().emplace<Cmp::PendingTeleport>( entity, sf::Time::Zero, event.m_delay, std::move( due_event ) );
    SPDLOG_INFO( "Entity {} - TELEPORT in {}s", static_cast<uint32_t>( entity ), event.m_delay.asSeconds() );
    return;
  }

  if ( event.m_anim ) { create_teleport_anim( *pos_cmp, *event.m_anim ); }

  if ( reg().all_of<Cmp::Player::Character>( entity ) )
  {
    // keep the departure animation in view until it has finished playing
    if ( event.m_anim )
    {
      const auto &anim_ss = Sys::SpriteStore::instance().get( *event.m_anim );
      if ( anim_ss.framerate() > 0.f )
      {
        const sf::Time hold = sf::seconds( static_cast<float>( anim_ss.indices_per_sequence() ) / anim_ss.framerate() );
        reg().emplace_or_replace<Cmp::Player::CameraHold>( entity, hold );
        SPDLOG_INFO( "CameraHold created for {}s ({})", hold.asSeconds(), event.m_anim->str() );
      }
    }

    if ( event.m_sfx )
    {
      if ( m_sound_bank.get_effect( event.m_sfx.value() ).getStatus() != sf::Sound::Status::Playing )
        m_sound_bank.get_effect( event.m_sfx.value() ).play();
    }
    get_systems_event_queue().trigger( Events::UpdatePlayerPositionEvent( event.m_position ) );
  }
  else { get_systems_event_queue().trigger( Events::UpdateNpcPositionEvent( entity, event.m_position ) ); }

  if ( event.m_anim ) { create_teleport_anim( reg().get<Cmp::Position>( entity ), *event.m_anim ); }

  SPDLOG_INFO( "Entity {} - TELEPORT to ({}, {}) COMPLETE", static_cast<uint32_t>( entity ), event.m_position.x, event.m_position.y );
}

entt::entity TeleportSystem::create_teleport_anim( const Cmp::Position &pos_cmp, const Sys::SpriteKey &anim )
{
  auto anim_entity = reg().create();
  // anchor the animation on the middle cell of the entity's body (a no-op for single-block entities)
  const sf::Vector2f anim_pos = pos_cmp.position + ( pos_cmp.size - Constants::kGridSizePxF ) * 0.5f;
  reg().emplace<Cmp::Position>( anim_entity, anim_pos, Constants::kGridSizePxF );
  reg().emplace<Cmp::TeleportAnim>( anim_entity );

  // clang-format off
  reg().emplace<Cmp::AnimData>( anim_entity, Cmp::AnimData::Config{
        .sprite_type = anim,
        .enabled = true,
        .anim_type = Cmp::AnimType::ONESHOTRESET
  });
  // clang-format on

  reg().emplace<Cmp::ZOrderValue>( anim_entity, anim_pos.y + 64 );
  return anim_entity;
}

} // namespace Game::Sys
