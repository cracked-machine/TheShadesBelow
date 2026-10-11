
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

// initialised by first call to getEventDispatcher()
std::unique_ptr<entt::dispatcher> BaseSystem::m_systems_event_queue = nullptr;

} // namespace Game::Sys