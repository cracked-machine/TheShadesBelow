#ifndef SRC_SYSTEMS_ALTARSYSTEM_HPP__
#define SRC_SYSTEMS_ALTARSYSTEM_HPP__

#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/StoreKey.hpp>

// clang-format off
namespace Game::Cmp::Altar { class MultiBlock; }
namespace Game::Cmp { class WorldItem; struct UUID; }
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
  void check_player_altar_activation( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp );

  //! @brief The animation played above the altar following a sacrifice
  enum class SacrificeAnimType { DEFAULT, KEY };

  //! @brief Swap the item for its "sacrifice_action.reward.item" (see items.json)
  void sacrifice_for_item( const Cmp::Altar::MultiBlock &altar_cmp, const Sys::ItemKey &reward_item );
  //! @brief Swap the item for its "sacrifice_action.reward.wealth" (see items.json), e.g. jewelry or worn out tools
  void sacrifice_for_wealth( const Cmp::Altar::MultiBlock &altar_cmp, int wealth );
  //! @brief Grant the player Cmp::Player::ToxicImmunity for "sacrifice_action.reward.tick" seconds (see items.json), e.g. St John's Wort
  void sacrifice_for_toxic_immunity( const Cmp::Altar::MultiBlock &altar_cmp, int tick );
  //! @brief Crypt altar: grants an extra life. Graveyard altar: banishes all ghosts.
  void sacrifice_witches_jar( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid, const Cmp::WorldItem &item );
  //! @brief Light an altar flame per relic. Drops an exitkey once the exitkey threshold is reached.
  void sacrifice_relic( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid );
  //! @brief Turn the altar flames blue. Drops a cryptkey once the cryptkey threshold is reached.
  void sacrifice_exitkey( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid );

  //! @brief Play the sacrifice animation/sound above the altar and start the activation cooldown
  void play_sacrifice_anim( const Cmp::Altar::MultiBlock &altar_cmp, SacrificeAnimType sacrifice_anim_type );
  //! @brief Drop an item at the player position and flash the inventory UI
  void drop_reward_item( const Sys::ItemKey &item_key );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_ALTARSYSTEM_HPP__
