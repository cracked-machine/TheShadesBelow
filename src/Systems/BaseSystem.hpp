#ifndef SRC_SYSTEMS_BASESYSTEM_HPP__
#define SRC_SYSTEMS_BASESYSTEM_HPP__

#include <entt/entity/fwd.hpp>
#include <entt/signal/dispatcher.hpp>

#include <PathFinding/SmartPointers.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <spdlog/fwd.h>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
namespace Game::PathFinding{ class SpatialHashGrid; }
namespace Game::Cmp { class BaseAction; class Position; }
// clang-format on

namespace Game::Sys
{

//! @brief Base class for all game systems. Provides shared access to the entity registry, render
//! window, sound bank, and the systems-wide event dispatcher.
class BaseSystem
{
public:
  //! @brief Construct a new Base System object
  BaseSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief polymorphic destructor for derived classes
  virtual ~BaseSystem() { SPDLOG_INFO( "BaseSystem destructor called for system at {}", static_cast<void *>( this ) ); };

  //! @brief Systems are non-copyable and non-movable: they hold references to shared engine
  //! resources (registry, window, sound bank) that must not be duplicated or rebound.
  BaseSystem( const BaseSystem & ) = delete;
  BaseSystem( BaseSystem && ) = delete;
  BaseSystem &operator=( const BaseSystem & ) = delete;
  BaseSystem &operator=( BaseSystem && ) = delete;

  //! @brief Singleton event dispatcher shared by all systems.
  //! @note Use this to get temporary access to the dispatcher to register event handlers.
  static entt::dispatcher &get_systems_event_queue()
  {
    if ( not m_systems_event_queue ) { m_systems_event_queue = std::make_unique<entt::dispatcher>(); }
    return *m_systems_event_queue;
  }

  //! @brief Get the entity registry.
  entt::registry &reg() { return m_reg.get(); }

  //! @brief Get the entity registry.
  [[nodiscard]] const entt::registry &reg() const { return m_reg.get(); }

  //! @brief Re-point this system at a different registry (called by SceneManager on scene change).
  void reg( entt::registry &reg ) { m_reg = std::ref( reg ); }

protected:
  //! @brief Roll and trigger the side effects of an applied action (see Cmp::SideEffects): NPC spawns via
  //! Events::CreateNpcEvent and lightning strikes via Events::LightningEvent. Movement effects are not yet executed.
  //! @param action The applied action, e.g. as returned by the Utils::Player::apply_action_from_* helpers.
  //! @param pos_cmp The action's source position (e.g. the world item, or the player for inventory/store actions).
  void trigger_side_effects( const Cmp::BaseAction &action, const Cmp::Position &pos_cmp );

  //! @brief Entity registry: non-owning, re-assignable reference (by SceneManager).
  //! The registry is owned by the current Scene.
  std::reference_wrapper<entt::registry> m_reg;

  //! @brief Non-owning reference to the OpenGL window
  sf::RenderWindow &m_window;

  //! @brief Non-owning reference to the shared sound effects system
  Audio::SoundBank &m_sound_bank;

private:
  //! @brief Prevent access to uninitialised dispatcher - use get_systems_event_queue()
  static std::unique_ptr<entt::dispatcher> m_systems_event_queue;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_BASESYSTEM_HPP__
