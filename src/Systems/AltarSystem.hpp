#ifndef SRC_SYSTEMS_ALTARSYSTEM_HPP__
#define SRC_SYSTEMS_ALTARSYSTEM_HPP__

#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <optional>

// clang-format off
namespace Game::Cmp::Altar { class MultiBlock; }
namespace Game::Cmp { struct ActionReward; struct UUID; }
namespace Game::Events { class PlayerActionEvent; }
// clang-format on

namespace Game::Sys
{

//! @brief Handles crypt altar sacrifices, granting the player rewards for items offered up.
class AltarSystem : public BaseSystem
{
public:
  //! @brief Construct a new Altar System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  AltarSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Per-frame tidy up
  void update( sf::Time dt );

private:
  //! @brief Capture Events::PlayerActionEvent::GameActions::ACTIVATE. Calls check_player_altar_activation().
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Rewards player for sacrificing various items at the altar
  //! @param altar_entity
  //! @param altar_cmp
  void check_player_altar_sacrifice( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp );

  //! @brief Grant the item's "sacrifice_action.reward" for this altar's location (see items.json). An item with no reward
  //! (e.g. one that only spawns an NPC) just plays the default sacrifice animation.
  //! @return false if this altar won't accept the offering in its current state, so the item must not be consumed
  bool grant_sacrifice_reward( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid,
                               const std::optional<Cmp::ActionReward> &reward );

  //! @brief The animation played above the altar following a sacrifice
  enum class SacrificeAnimType { DEFAULT, KEY };

  //! @brief Swap the item for its "sacrifice_action.reward.item" (see items.json), at the player by default, else at the
  //! reward's target. Call once the offering has left the inventory.
  //! @param on_created Optional callback receiving each item dropped in the world, e.g. to bind it to a summoned NPC.
  void sacrifice_for_item( const Cmp::Altar::MultiBlock &altar_cmp, const Cmp::ActionReward &reward,
                           const std::function<void( entt::entity )> &on_created = {} );
  //! @brief Swap the item for its "sacrifice_action.reward.wealth" (see items.json), e.g. jewelry or worn out tools
  void sacrifice_for_wealth( const Cmp::Altar::MultiBlock &altar_cmp, int wealth );
  //! @brief Grant the player Cmp::Player::ToxicImmunity for "sacrifice_action.reward.tick" seconds (see items.json), e.g. St John's Wort
  void sacrifice_for_toxic_immunity( const Cmp::Altar::MultiBlock &altar_cmp, int tick );
  //! @brief Banish all ghosts, e.g. witches jar at a graveyard altar
  void sacrifice_for_banish( const Cmp::Altar::MultiBlock &altar_cmp );
  //! @brief Grant the player a Cmp::Player::ExtraLife. Crypt altar: lights the altar, respawn at the crypt start.
  //! Graveyard altar: respawn back at this altar.
  //! @return false if the altar refused the offering, e.g. the player already has an extra life
  bool sacrifice_for_extra_life( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid );
  //! @brief Light an altar candle per relic. Drops an exitkey once the exitkey threshold is reached (all candles lit).
  //! @return false if the altar refused the offering
  bool sacrifice_relic( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid );
  //! @brief Turn the altar flames blue. Drops a cryptkey once the cryptkey threshold is reached.
  //! @return false if the altar refused the offering
  bool sacrifice_exitkey( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid );

  //! @brief Show the altar's extra life: swap to its active sprite, add the extra life particles and play the activate
  //! sound. Undone by Factory::Player::remove_player_extra_life() when the extra life is spent.
  //! @note Does nothing if the altar is already active.
  void activate_altar( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid );
  //! @brief Play the sacrifice animation/sound above the altar and start the activation cooldown
  void play_sacrifice_anim( const Cmp::Altar::MultiBlock &altar_cmp, SacrificeAnimType sacrifice_anim_type );
  //! @brief Drop an item at the player position and flash the inventory UI
  void drop_reward_item( const Sys::ItemKey &item_key );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_ALTARSYSTEM_HPP__
