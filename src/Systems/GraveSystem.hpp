#ifndef SRC_SYSTEMS_GRAVESYSTEM_HPP__
#define SRC_SYSTEMS_GRAVESYSTEM_HPP__

#include <Components/Grave/Consequence.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <string>

#include <vector>

// clang-format off
namespace Game::Cmp { class AnimData; }
namespace Game::Cmp::Player { class Character; }
namespace Game::Cmp::Grave { class Consequence; }
namespace Game::Events { struct AltarActivatedEvent; }
namespace Game::Events { struct ExhumeGraveEvent; }
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

  //! @brief Outcome of pick_next_trail_grave(), telling the caller whether the trail has ended
  enum class TrailStep {
    ADVANCED, //!< the trail moved on to a new grave
    COMPLETE, //!< the trail has run its full length and ends here, at the exit key
    NO_GRAVES //!< no un-exhumed graves are left to point at, so the trail ends here early
  };

  //! @brief Point source_entt's Cmp::Grave::TrailMarker arrow at a random un-exhumed grave and give that grave the next Cmp::Grave::TrailMarker
  //! @details Every other Cmp::Grave::TrailMarker is removed first, so only the latest arrow is shown. The target is
  //! picked from graves within Cmp::Persist::GraveTrailNominalHopDistance plus the level depth grid cells, preferring
  //! ones that don't lie back the way the player came; if none are in range, the nearest grave is used instead.
  //! @param source_entt the grave just exhumed
  //! @return TrailStep::COMPLETE if the trail has run its full length (Cmp::Persist::GraveTrailNominalHopCount plus the
  //! level depth), TrailStep::NO_GRAVES if no graves are left to point at, otherwise TrailStep::ADVANCED. On either of
  //! the first two the caller ends the trail with finish_trail().
  TrailStep pick_next_trail_grave( entt::entity source_entt );

  //! @brief What opened a grave, which decides where its loot/key/bomb ends up
  enum class ExhumedBy {
    PLAYER, //!< dug by the player: loot drops at the player, bombs centre on them
    BOMB    //!< caught in a blast: loot drops beside the grave (spared by the blast), bombs centre on the grave
  };

  //! @brief Handler for Events::ExhumeGraveEvent: open a grave that was not dug by the player, e.g. caught in a bomb blast
  //! @param event
  void on_exhume_grave( const Events::ExhumeGraveEvent &event );

  //! @brief Switch the grave's sprite to its opened state and trigger a random consequence
  //! @param grave_entity
  //! @param grave_anim_cmp
  //! @param by what opened the grave
  void open_grave( entt::entity grave_entity, Cmp::AnimData &grave_anim_cmp, ExhumedBy by );

  //! @brief End the grave trail at grave_entity: drop the exit key beside the grave (blast proof) for the player to pick
  //! up, restart the trail and send Events::GraveTrailCompleteEvent so AltarSystem releases the activated altar.
  //! The player can carry the exit key to the exit, or offer it at an altar for the crypt key.
  //! @param grave_entity the last grave of the trail
  void finish_trail( entt::entity grave_entity );

  //! @brief Pick a random item from the pool, drop it as loot, and apply its Cmp::SpawnAction modifiers to the player.
  //! ExhumedBy::PLAYER drops it at the player; ExhumedBy::BOMB drops it beside the grave, spared by the blast.
  //! @param loot_pool
  //! @param grave_entity the grave the loot came from
  //! @param by what opened the grave
  void spawn_grave_loot( const std::vector<Sys::ItemKey> &loot_pool, entt::entity grave_entity, ExhumedBy exhumedby );

  //! @brief Give a torch dropped by a grave its flame particle sprite. Does nothing for any other item.
  //! @param item_entt the world item just created by Events::CreateItemEvent
  void light_torch_loot( entt::entity item_entt );

  //! @brief Tag of the flame on a torch dropped by a grave, matching the graveyard's scene-load torch flames
  inline static const std::string kTorchFlameTag{ "graveyard.torch.particle.flame" };

  //! @brief Bounds of the grave, as a position for spawning items beside it
  //! @param grave_entity
  //! @return Cmp::Position covering the grave's Cmp::Grave::MultiBlock
  Cmp::Position grave_bounds( entt::entity grave_entity );

  bool player_digging{ false };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_GRAVESYSTEM_HPP__
