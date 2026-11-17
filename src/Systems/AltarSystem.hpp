#ifndef SRC_SYSTEMS_ALTARSYSTEM_HPP__
#define SRC_SYSTEMS_ALTARSYSTEM_HPP__

#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <optional>

// clang-format off
namespace Game::Cmp::Altar { class MultiBlock; }
namespace Game::Cmp { struct ActionReward; struct UUID; }
namespace Game::Events { class PlayerActionEvent; struct GraveTrailCompleteEvent; }
// clang-format on

namespace Game::Sys
{

//! @brief Handles crypt altar offerings, granting the player rewards for items offered up.
class AltarSystem : public BaseSystem
{
public:
  //! @brief Construct a new Altar System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  AltarSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Per-frame tidy up, and send Events::AltarApproachedEvent when the player comes near a graveyard altar
  void update( sf::Time dt );

private:
  //! @brief Capture Events::PlayerActionEvent::GameActions::ACTIVATE. Calls check_player_altar_offering() for the
  //! altar the player is near.
  void on_player_action( Events::PlayerActionEvent ev );

  //! @brief Capture Events::GraveTrailCompleteEvent. Resets the activated altar: puts out its centre column, reverts its
  //! inactive sprite, empties its Cmp::Altar::BloodCount and removes Cmp::Altar::Activated, so it (or any other altar)
  //! takes its blood quota again.
  void on_grave_trail_complete( const Events::GraveTrailCompleteEvent &event );

  //! @brief Send Events::AltarApproachedEvent on the frame the player comes near a graveyard altar still waiting for
  //! blood. Once per approach: the player must leave every such altar before it is sent again.
  void check_player_approached_altar();

  //! @brief Rewards player for offering various items at the altar
  //! @param altar_entity
  //! @param altar_cmp
  //! @return true if the held item was offered (consumed), false if there was nothing to offer or the altar refused it
  bool check_player_altar_offering( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp );

  //! @brief Activate a graveyard altar whose blood quota is filled: lights its centre column, marks it
  //! Cmp::Altar::Activated and sends Events::AltarActivatedEvent so GraveSystem starts the grave trail.
  void activate_graveyard_altar( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid );

  //! @brief Grant the item's "offering_action.reward" for this altar's location (see items.json). An item with no reward
  //! (e.g. one that only spawns an NPC) just plays the default offering animation.
  //! @return false if this altar won't accept the offering in its current state, so the item must not be consumed
  bool grant_offering_reward( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid,
                              const std::optional<Cmp::ActionReward> &reward );

  //! @brief The animation played above the altar following an offering
  enum class OfferingAnimType { DEFAULT, KEY };

  //! @brief Swap the item for its "offering_action.reward.item" (see items.json), at the player by default, else at the
  //! reward's target. Call once the offering has left the inventory.
  //! @param on_created Optional callback receiving each item dropped in the world, e.g. to bind it to a summoned NPC.
  void offering_for_item( const Cmp::Altar::MultiBlock &altar_cmp, const Cmp::ActionReward &reward,
                          const std::function<void( entt::entity )> &on_created = {} );
  //! @brief Swap the item for its "offering_action.reward.wealth" (see items.json), e.g. jewelry or worn out tools
  void offering_for_wealth( const Cmp::Altar::MultiBlock &altar_cmp, int wealth );
  //! @brief Grant the player Cmp::Player::ToxicImmunity for "offering_action.reward.tick" seconds (see items.json), e.g. St John's Wort
  void offering_for_toxic_immunity( const Cmp::Altar::MultiBlock &altar_cmp, int tick );
  //! @brief Banish all ghosts, e.g. witches jar at a graveyard altar
  void offering_for_banish( const Cmp::Altar::MultiBlock &altar_cmp );
  //! @brief Grant the player a Cmp::Player::ExtraLife at the crypt altar: lights the altar, respawn at the crypt start.
  //! @return false if the altar refused the offering, e.g. the player already has an extra life, or it is not the crypt altar
  bool offering_for_extra_life( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid );
  //! @brief Add the offered blood to the altar's Cmp::Altar::BloodCount. The offering that fills
  //! Cmp::Persist::AltarBloodQuota activates the altar, see activate_graveyard_altar().
  //! @return false if the altar refused the offering, e.g. it is not a graveyard altar or an altar is already activated
  bool offering_blood( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid, int blood );
  //! @brief Accept the exit key in exchange for the crypt key, at any graveyard altar whether activated or not. The exit
  //! key's (negative) offering anima is its cost. check_player_altar_offering() puts the crypt key in the player's hand
  //! once the exit key has been destroyed.
  //! @return false if the altar refused the offering, i.e. the player can't afford it
  bool offering_cryptkey( const Cmp::Altar::MultiBlock &altar_cmp );
  //! @brief Whether the player has enough anima to pay the item's (negative) offering anima. If not, flash the anima
  //! meter and send Events::InsufficientAnimaEvent to explain why.
  bool can_afford_offering( const Sys::ItemKey &item_key );
  //! @brief Put an altar reward key (e.g. the crypt key) straight into the player's (empty) hand and flash the inventory UI
  void give_reward_key( const Sys::ItemKey &item_key );

  //! @brief Light the altar: swap to its active sprite, add its centre particles and play the activate sound.
  //! @param particle_tag Factory::Altar::kExtraLifeParticleTag for an extra life (undone by
  //! Factory::Player::remove_player_extra_life()), or Factory::Altar::kActivatedParticleTag for a grave trail (undone
  //! by on_grave_trail_complete()).
  //! @note Does nothing if the altar is already active.
  void activate_altar( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid, const char *particle_tag );
  //! @brief Play the offering animation/sound above the altar and start the activation cooldown
  void play_offering_anim( const Cmp::Altar::MultiBlock &altar_cmp, OfferingAnimType offering_anim_type );

  //! @brief Whether the player was near a graveyard altar waiting for blood last frame, see check_player_approached_altar()
  bool m_player_near_altar{ false };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_ALTARSYSTEM_HPP__
