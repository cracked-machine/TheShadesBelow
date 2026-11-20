#ifndef SRC_SYSTEMS_BASESYSTEM_HPP__
#define SRC_SYSTEMS_BASESYSTEM_HPP__

#include <Components/Altar/MultiBlock.hpp>
#include <Components/Crypt/BuildingMultiBlock.hpp>
#include <Components/Crypt/BuildingSegment.hpp>
#include <Components/Grave/ExitMultiBlock.hpp>
#include <Components/Grave/ExitSegment.hpp>
#include <Components/Ruin/BuildingMultiBlock.hpp>
#include <Components/Ruin/BuildingSegment.hpp>
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Spring/HealingSpringBuildingSegment.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/signal/dispatcher.hpp>
#include <functional>

#include <PathFinding/SmartPointers.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <spdlog/fwd.h>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
namespace Game::PathFinding{ class SpatialHashGrid; }
namespace Game::Cmp { class BaseAction; class Position; struct ActionReward; struct ActionSfx; }
namespace Game::Cmp::Altar { class MultiBlock; class Segment; }
namespace Game::Cmp::Crypt { class BuildingMultiBlock; class BuildingSegment; }
namespace Game::Cmp::Grave { class ExitMultiBlock; class ExitSegment; }
namespace Game::Cmp::Ruin { class BuildingMultiBlock; class BuildingSegment; }
namespace Game::Cmp { class HealingSpringBuildingMultiBlock; class HealingSpringBuildingSegment; }

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
  //! @param source_entt The world item whose action this is, if any. Its Cmp::UUID keys Cmp::SpawnNpcEffect::max_instances.
  void trigger_side_effects( const Cmp::BaseAction &action, const Cmp::Position &pos_cmp, entt::entity source_entt = entt::null );

  //! @brief Play an action's "final" sound effect (see Cmp::ActionSfx), if it has one.
  //! @param sfx The action's sound effects.
  void play_action_sfx( const Cmp::ActionSfx &sfx );
  //! @brief Play the action's "final" sound effect (see Cmp::ActionSfx), if it has one.
  //! @param action The applied action.
  void play_action_sfx( const Cmp::BaseAction &action );

  //! @brief Play an action's "loop" sound effect (see Cmp::ActionSfx), if it has one and it isn't already playing.
  //! Call every frame while the action is in progress; the sound is replayed each time it finishes.
  //! @param sfx The in-progress action's sound effects.
  void play_action_loop_sfx( const Cmp::ActionSfx &sfx );
  //! @brief Play the action's "loop" sound effect, see play_action_loop_sfx( const Cmp::ActionSfx & ).
  //! @param action The in-progress action.
  void play_action_loop_sfx( const Cmp::BaseAction &action );

  //! @brief Stop an action's "loop" sound effect (see Cmp::ActionSfx), if it has one.
  //! @param sfx The finished action's sound effects.
  void stop_action_loop_sfx( const Cmp::ActionSfx &sfx );
  //! @brief Stop the action's "loop" sound effect, see stop_action_loop_sfx( const Cmp::ActionSfx & ).
  //! @param action The action that has finished.
  void stop_action_loop_sfx( const Cmp::BaseAction &action );

  //! @brief Grant a reward item at its explicit target (see Cmp::ActionReward::Target): Events::AddInventoryEvent if the
  //! player's hand is free, otherwise Events::CreateItemEvent on free cells beside the source. A wildcard item is picked
  //! afresh for each item. Callers handle Cmp::ActionReward::Target::DEFAULT themselves, which is treated as NEARBY here.
  //! @param reward The reward, its item must be set.
  //! @param source The source's bounds, e.g. the plant or altar.
  //! @param source_entt The world item whose action this is, if any. Keys Cmp::ActionReward::max_instances.
  //! @param on_created Optional callback receiving each item dropped in the world (not called for an inventory item).
  void grant_item_reward( const Cmp::ActionReward &reward, const Cmp::Position &source, entt::entity source_entt = entt::null,
                          const std::function<void( entt::entity )> &on_created = {} );

  //! @brief Sync the multiblock's z-order to the current player-segment collision.
  template <class MULTIBLOCK, class MBSEGMENT>
  void update_mb_zorder();

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

extern template void BaseSystem::update_mb_zorder<Cmp::Crypt::BuildingMultiBlock, Cmp::Crypt::BuildingSegment>();
extern template void BaseSystem::update_mb_zorder<Cmp::Grave::ExitMultiBlock, Cmp::Grave::ExitSegment>();
extern template void BaseSystem::update_mb_zorder<Cmp::Ruin::BuildingMultiBlock, Cmp::Ruin::BuildingSegment>();
extern template void BaseSystem::update_mb_zorder<Cmp::Altar::MultiBlock, Cmp::Altar::Segment>();
extern template void BaseSystem::update_mb_zorder<Cmp::HealingSpringBuildingMultiBlock, Cmp::HealingSpringBuildingSegment>();

} // namespace Game::Sys

#endif // SRC_SYSTEMS_BASESYSTEM_HPP__
