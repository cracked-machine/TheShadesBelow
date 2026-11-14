#include <Components/Inventory/Grimoire.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/SceneSettings/Paused.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Systems/Render/DebugOverlaySystem.hpp>
#include <Systems/Render/GrimoireOverlaySystem.hpp>
#include <Systems/Render/HudOverlaySystem.hpp>
#include <Systems/Render/OverlayArbitrationSystem.hpp>
#include <Systems/Render/PauseOverlaySystem.hpp>
#include <Systems/Render/ShopOverlaySystem.hpp>
#include <Systems/Render/TooltipOverlaySystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Utils/Profiling.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <entt/entity/registry.hpp>
#include <imgui-SFML.h>
#include <spdlog/spdlog.h>

namespace Game::Sys
{

namespace
{

//! @brief The `enabled` flag of a singleton scene setting, or false if the scene has none (e.g. a menu scene, or a
//! registry being torn down).
template <typename SceneSetting>
bool setting_enabled( entt::registry &reg )
{
  auto view = reg.view<SceneSetting>();
  if ( view.empty() ) { return false; }
  return view.template get<SceneSetting>( view.front() ).enabled;
}

//! @brief True if the player's grimoire is open.
bool grimoire_open( entt::registry &reg )
{
  auto view = reg.view<Cmp::Grimoire>();
  if ( view.empty() ) { return false; }
  return view.get<Cmp::Grimoire>( view.front() ).is_enabled;
}

} // namespace

OverlayArbitrationSystem::OverlayArbitrationSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, Store &sys )
    : BaseSystem( reg, window, sound_bank ),
      m_sys( sys )
{
  std::ignore = get_systems_event_queue().sink<Events::OverlayEvent>().connect<&OverlayArbitrationSystem::on_overlay_event>( this );
}

void OverlayArbitrationSystem::update( sf::Time dt, sf::Time world_dt )
{
  m_grimoire_toggle_debounce += dt;

  begin_frame( dt );

  PROFILED( m_sys.find<Store::Type::HudOverlaySystem>().update( world_dt ) );
  if ( has_shop() ) { PROFILED( m_sys.find<Store::Type::ShopOverlaySystem>().update( dt ) ); }
  if ( has_level_depth() ) { PROFILED( m_sys.find<Store::Type::HudOverlaySystem>().render_level_depth() ); }
  // the tooltip countdown stops while gameplay is paused, by either modal overlay
  if ( active_modal( reg() ) == Modal::NONE ) { PROFILED( m_sys.find<Store::Type::TooltipOverlaySystem>().update( dt ) ); }
  // real dt even while paused, so the grimoire can still be paged
  if ( has_grimoire() ) { PROFILED( m_sys.find<Store::Type::GrimoireOverlaySystem>().update( dt ) ); }
  PROFILED( m_sys.find<Store::Type::PauseOverlaySystem>().update( dt ) );
  // the debug visualisations are hidden while paused
  if ( not is_paused() ) { PROFILED( m_sys.find<Store::Type::DebugOverlaySystem>().update( dt ) ); }

  end_frame();
}

OverlayArbitrationSystem::Modal OverlayArbitrationSystem::active_modal( entt::registry &reg )
{
  if ( setting_enabled<Cmp::SceneSettings::Paused>( reg ) ) { return Modal::PAUSE; }
  if ( grimoire_open( reg ) ) { return Modal::GRIMOIRE; }
  return Modal::NONE;
}

bool OverlayArbitrationSystem::can_open_modal( entt::registry &reg )
{
  return active_modal( reg ) == Modal::NONE and not setting_enabled<Cmp::SceneSettings::ShowDebugStats>( reg );
}

bool OverlayArbitrationSystem::debug_panels_visible( entt::registry &reg )
{
  return setting_enabled<Cmp::SceneSettings::ShowDebugStats>( reg ) and active_modal( reg ) == Modal::NONE;
}

bool OverlayArbitrationSystem::hover_allowed( entt::registry &reg )
{
  return active_modal( reg ) == Modal::NONE and not setting_enabled<Cmp::SceneSettings::ShowDebugStats>( reg );
}

void OverlayArbitrationSystem::on_overlay_event( const Events::OverlayEvent &ev )
{
  using Action = Events::OverlayEvent::OverlayActions;
  switch ( ev.action )
  {
    case Action::PAUSE_TOGGLE:
      toggle_pause();
      break;
    case Action::GRIMOIRE_TOGGLE:
      toggle_grimoire();
      break;
    case Action::DEBUG_TOGGLE:
      toggle_debug();
      break;
    case Action::GRIMOIRE_PREV:
    case Action::GRIMOIRE_NEXT:
      // page turning is handled by GrimoireOverlaySystem
      break;
  }
}

void OverlayArbitrationSystem::toggle_pause()
{
  auto &pause = m_sys.find<Store::Type::PauseOverlaySystem>();
  if ( pause.is_open() ) { pause.close(); }
  else if ( can_open_modal( reg() ) ) { pause.open(); }
}

void OverlayArbitrationSystem::toggle_grimoire()
{
  constexpr float kToggleDebounceThreshold = 1.f;
  if ( m_grimoire_toggle_debounce.asSeconds() < kToggleDebounceThreshold ) { return; }

  auto &grimoire = m_sys.find<Store::Type::GrimoireOverlaySystem>();
  if ( grimoire.is_open() ) { grimoire.close(); }
  else if ( can_open_modal( reg() ) and has_grimoire() ) { grimoire.open(); }
  else { return; }
  m_grimoire_toggle_debounce = sf::Time::Zero;
}

void OverlayArbitrationSystem::toggle_debug()
{
  if ( active_modal( reg() ) != Modal::NONE ) { return; }
  auto debug_view = reg().view<Cmp::SceneSettings::ShowDebugStats>();
  if ( debug_view.empty() ) { return; }
  bool &show_debug_stats = debug_view.get<Cmp::SceneSettings::ShowDebugStats>( debug_view.front() ).enabled;
  show_debug_stats = not show_debug_stats;
  SPDLOG_INFO( "Show debug stats is now {}", show_debug_stats ? "ENABLED" : "DISABLED" );
}

void OverlayArbitrationSystem::begin_frame( sf::Time dt )
{
  // ImGui asserts on a non-positive DeltaTime, so never hand it a zero frame time
  ImGui::SFML::Update( m_window, std::max( dt, sf::microseconds( 1 ) ) );
}

void OverlayArbitrationSystem::end_frame() { ImGui::SFML::Render( m_window ); }

bool OverlayArbitrationSystem::is_paused() { return setting_enabled<Cmp::SceneSettings::Paused>( reg() ); }

bool OverlayArbitrationSystem::has_shop() { return Utils::is_current_scene( reg(), Cmp::SceneSettings::SceneId::SHOP ); }

bool OverlayArbitrationSystem::has_grimoire() { return Utils::is_current_scene( reg(), Cmp::SceneSettings::SceneId::GRAVEYARD ); }

bool OverlayArbitrationSystem::has_level_depth() { return Utils::is_current_scene( reg(), Cmp::SceneSettings::SceneId::GRAVEYARD ); }

} // namespace Game::Sys
