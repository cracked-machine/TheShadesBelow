#ifndef SRC_COMPONENTS_STATS_ACTIONREWARD_HPP__
#define SRC_COMPONENTS_STATS_ACTIONREWARD_HPP__

//! @file
//! @brief The optional reward of an action (see BaseAction::rewards()).

#include <Components/Altar/Kind.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <optional>

namespace Game::Cmp
{

//! @brief Optional reward granted when an action is applied. See the "reward" object of the offering_action,
//! forage_action, destroy_action and proximity_action entries in res/json/items.json.
//! @details An offering_action reward sets exactly one of item, wealth or altar, optionally restricted to one altar
//! location. A forage_action/destroy_action reward only sets item. A proximity_action reward sets exactly one of item or
//! loot, dropped beside the source.
//! The item may end with '*' to pick a random item key with that prefix (e.g. "item.jewelry*"), and may set a target
//! (inventory or nearby) and, when nearby, a count of items to drop.
struct ActionReward
{
  //! @brief Where a reward item is placed.
  enum class Target {
    //! @brief The action's default placement, e.g. inventory for an empty-hand forage, beside the plant for a gloves forage.
    DEFAULT,
    //! @brief Added to the player inventory, or dropped nearby if the player's hand isn't free.
    INVENTORY,
    //! @brief Dropped on random free cells beside the source.
    NEARBY
  };

  //! @brief An altar effect triggered by offering the item.
  enum class AltarEffect {
    //! @brief No altar effect.
    NONE,
    //! @brief Light an altar talisman.
    TALISMAN,
    //! @brief Banish all ghosts (witches jar).
    BANISH,
    //! @brief Gain an extra life, respawning the player when they die (witches jar, nightshade).
    EXTRA_LIFE,
    //! @brief Unlock the crypt key (exit key).
    CRYPT_KEY,
    //! @brief Temporary immunity to toxidrome increases (St John's Wort).
    TOXIC_IMMUNITY
  };

  //! @brief The item gained. Empty string if unused.
  Sys::ItemKey item;
  //! @brief The wealth gained. Zero if unused.
  int wealth{ 0 };
  //! @brief The altar effect. NONE if unused.
  AltarEffect altar{ AltarEffect::NONE };
  //! @brief The kind of altar this reward is granted at (offering_action only). std::nullopt means any altar.
  std::optional<Altar::Kind> location;
  //! @brief How long (seconds) a timed altar effect lasts, e.g. TOXIC_IMMUNITY (offering_action only). Zero if unused.
  int tick{ 0 };
  //! @brief The loot sprite dropped (proximity_action only). Empty string if unused.
  Sys::SpriteKey loot;
  //! @brief Distance in blocks from the source that the player must be within before the reward drops (proximity_action only).
  int trigger_range{ 1 };
  //! @brief Max drops over each source item instance's lifetime, picked up or not; further drops are skipped. 0 = unlimited
  //! (proximity_action only).
  int max_instances{ 0 };
  //! @brief Where the reward item is placed.
  Target target{ Target::DEFAULT };
  //! @brief The number of reward items dropped, "count" in items.json (Target::NEARBY only).
  int quantity{ 1 };
  //! @brief Whether the item ends with '*', i.e. is a random pick from the item keys with that prefix.
  [[nodiscard]] bool is_item_wildcard() const { return item.str().ends_with( '*' ); }
  //! @brief The number of reward fields that are set.
  //! @return int Count of item, wealth, altar and loot that are not their unused value.
  [[nodiscard]] int count() const
  {
    return static_cast<int>( not item.empty() ) + static_cast<int>( wealth > 0 ) + static_cast<int>( altar != AltarEffect::NONE ) +
           static_cast<int>( not loot.empty() );
  }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_ACTIONREWARD_HPP__
