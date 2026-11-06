#ifndef SRC_SYSTEMS_GRAVESYSTEM_HPP__
#define SRC_SYSTEMS_GRAVESYSTEM_HPP__

#include <Components/Grave/Consequence.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <vector>

// clang-format off
namespace Game::Cmp { class AnimData; }
namespace Game::Cmp::Player { class Character; }
namespace Game::Cmp::Grave { class Consequence; }
namespace Game::Events { struct AltarActivatedEvent; }
// clang-format on

namespace Game::Sys
{

//! @brief Handles digging up graveyard graves and their random consequences (traps, relics, jewelry).
class GraveSystem : public BaseSystem
{
public:
  //! @brief Construct a new Grave System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  GraveSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Use Cmp::SelectedPosition to mark targetted graves and destroy them when appropriate tool is equipped.
  void update();

private:
  //! @brief handler for Events::PlayerActionEvent::GameActions::DIG
  //! @param event
  void on_player_action( const Events::PlayerActionEvent &event );

  //! @brief Handler for Events::AltarActivatedEvent. Tags Cmp::Persist::GraveTrailStartCount random un-exhumed graves, at least
  //! 10 grid cells clear of the altar, with Cmp::Grave::TrailStart. Each start faces a different edge of the altar until all
  //! four edges have one. Exhuming any one of them starts the key grave trail.
  //! @param event
  void on_altar_activated( const Events::AltarActivatedEvent &event );

  //! @brief True if the player has a digging tool equipped with remaining wear
  bool has_digging_tool_equipped();

  //! @brief True while the player's per-entity dig cooldown has not yet elapsed
  bool is_dig_on_cooldown();

  //! @brief Remove Cmp::SelectedPosition from grave entities now that the cooldown has expired
  void clear_stale_grave_selections();

  //! @brief True if any Cmp::Player::Character entity's hitbox intersects the grave
  //! @param grave_cmp
  bool is_player_near( const Cmp::Grave::MultiBlock &grave_cmp );

  void update_grave_halo_particles();

  //! @brief Apply one dig hit: reduce tool wear and grave hp, then play the hit sound or open the grave
  //! @param grave_entity
  //! @param grave_cmp
  //! @param grave_anim_cmp
  void apply_dig_hit( entt::entity grave_entity, Cmp::Grave::MultiBlock &grave_cmp, Cmp::AnimData &grave_anim_cmp );

  //! @brief Outcome of pick_next_trail_grave(), telling the caller which key (if any) to drop
  enum class TrailStep
  {
    ADVANCED,  //!< the trail moved on to a new grave
    EXIT_KEY,  //!< the exit key trail is complete; the trail still moves on if graves remain
    CRYPT_KEY, //!< the crypt key trail is complete and the trail ends
    NO_GRAVES  //!< no un-exhumed graves are left to point at
  };

  //! @brief Point source_entt's Cmp::Grave::TrailMarker arrow at a random un-exhumed grave and give that grave the next Cmp::Grave::TrailMarker
  //! @param source_entt the grave just exhumed
  //! @return TrailStep::EXIT_KEY or TrailStep::CRYPT_KEY if that trail is complete (one or two times the trail size from
  //! Cmp::Persist::GraveTrailSizeMultiplier), the caller drops the key
  //! @return TrailStep::NO_GRAVES if no graves are left to point at, otherwise TrailStep::ADVANCED
  TrailStep pick_next_trail_grave( entt::entity source_entt );

  //! @brief Switch the grave's sprite to its opened state and trigger a random consequence
  //! @param grave_entity
  //! @param grave_anim_cmp
  void open_grave( entt::entity grave_entity, Cmp::AnimData &grave_anim_cmp );

  //! @brief Put the exhumed key straight into the player's inventory, dropping whatever they were holding at their feet
  //! @param key item.exitkey or item.cryptkey
  void grant_grave_key( const Sys::ItemKey &key );

  //! @brief Pick a random item from the pool, drop it as loot, and apply its Cmp::SpawnAction modifiers to the player
  //! @param loot_pool
  void spawn_grave_loot( const std::vector<Sys::ItemKey> &loot_pool );

  bool player_digging{ false };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_GRAVESYSTEM_HPP__
