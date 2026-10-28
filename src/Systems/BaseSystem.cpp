
#include <Components/Position.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/UUID.hpp>
#include <Events/CreateNpcEvent.hpp>
#include <Events/LightningEvent.hpp>
#include <Systems/BaseSystem.hpp>

#include <entt/entity/registry.hpp>

// clang-format off
namespace Audio  { class SoundBank; }
// clang-format on

namespace Game::Sys
{

BaseSystem::BaseSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : m_reg( reg ),
      m_window( window ),
      m_sound_bank( sound_bank )
{
  SPDLOG_DEBUG( "BaseSystem constructor called" );
}

void BaseSystem::trigger_side_effects( const Cmp::BaseAction &action, const Cmp::Position &pos_cmp, entt::entity source_entt )
{
  const auto &effects = action.side_effects();
  if ( effects.spawn_npc and effects.spawn_npc->roll() )
  {
    Cmp::UUID source_uuid;
    if ( reg().valid( source_entt ) )
    {
      if ( const auto *uuid_cmp = reg().try_get<Cmp::UUID>( source_entt ) ) { source_uuid = *uuid_cmp; }
    }
    // spawn beside the source (plant, player...) rather than on top of it
    get_systems_event_queue().trigger( Events::CreateNpcEvent( Sys::NpcKey( effects.spawn_npc->npc_type ), pos_cmp, effects.spawn_npc->max_instances,
                                                               source_uuid, {}, /*spawn_nearby=*/true ) );
  }
  // a null target lets LightningSystem pick a working lightning rod in view, otherwise the player
  if ( effects.lightning and effects.lightning->roll() ) { get_systems_event_queue().trigger( Events::LightningEvent{} ); }
}

// initialised by first call to getEventDispatcher()
std::unique_ptr<entt::dispatcher> BaseSystem::m_systems_event_queue = nullptr;

} // namespace Game::Sys