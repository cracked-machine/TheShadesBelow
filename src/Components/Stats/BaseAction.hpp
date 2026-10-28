#ifndef SRC_COMPONENTS_STATS_BASEACTION_HPP__
#define SRC_COMPONENTS_STATS_BASEACTION_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <memory>
#include <optional>
#include <string>

namespace Game::Cmp::Toxicity
{
class Toxidrome;
} // namespace Game::Cmp::Toxicity

namespace Game::Cmp
{
namespace Stats
{

//! @brief Strongly-typed health delta, used to disambiguate BaseAction/derived-action constructor arguments.
struct Health
{
  //! @brief The change applied to the health stat.
  int value{ 0 };
};

//! @brief Strongly-typed fear delta, used to disambiguate BaseAction/derived-action constructor arguments.
struct Fear
{
  //! @brief The change applied to the fear stat.
  int value{ 0 };
};

//! @brief Strongly-typed despair delta, used to disambiguate BaseAction/derived-action constructor arguments.
struct Despair
{
  //! @brief The change applied to the despair stat.
  int value{ 0 };
};

//! @brief Strongly-typed infamy delta, used to disambiguate BaseAction/derived-action constructor arguments.
struct Infamy
{
  //! @brief The change applied to the infamy stat.
  int value{ 0 };
};

//! @brief Strongly-typed toxicity delta, used to disambiguate BaseAction/derived-action constructor arguments.
struct Toxicity
{
  //! @brief The change applied to the toxicity stat.
  int value{ 0 };
};

//! @brief Strongly-typed luck delta, used to disambiguate BaseAction/derived-action constructor arguments.
struct Luck
{
  //! @brief The change applied to the luck stat.
  int value{ 0 };
};

//! @brief Strongly-typed tick interval, used to disambiguate BaseAction/derived-action constructor arguments.
struct Tick
{
  //! @brief How often (seconds) the action re-applies, or 0 for a one-shot.
  float value{ 0 };
};

} // namespace Stats

//! @brief Optional side effect of an action: spawn an NPC at the action's source position.
struct SpawnNpcEffect
{
  //! @brief The NPC to spawn, see res/json/npc.json.
  std::string npc_type;
  //! @brief Chance [0, 1] that the effect fires each time the action is applied.
  float probability{ 0.f };
  //! @brief Max live NPCs spawned by each world item instance, across all of its actions. 0 = unlimited.
  int max_instances{ 0 };
  //! @brief Distance in blocks from the item that the player must be within before the NPC spawn is triggered.
  int trigger_range{ 1 };
  //! @brief Distance from the item to spawned NPC.
  int spawn_radius{ 1 };
  //! @brief Roll this effect's probability.
  //! @return true if the effect should fire.
  [[nodiscard]] bool roll() const;
};

//! @brief Optional side effect of an action: a lightning strike (on a working lightning rod in view, else the player).
struct LightningEffect
{
  //! @brief Chance [0, 1] that the effect fires each time the action is applied.
  float probability{ 0.f };
  //! @brief Roll this effect's probability.
  //! @return true if the effect should fire.
  [[nodiscard]] bool roll() const;
};

//! @brief Optional side effect of an action affecting player movement/NPC pathfinding.
//! @note Placeholder: parsed and shown in the grimoire, but not yet executed.
struct MovementEffect
{
  //! @brief Free-form effect type, e.g. "slow".
  std::string type;
  //! @brief Chance [0, 1] that the effect fires each time the action is applied.
  float probability{ 0.f };
  //! @brief Roll this effect's probability.
  //! @return true if the effect should fire.
  [[nodiscard]] bool roll() const;
};

//! @brief The optional side effects of an action, each rolled independently. See the "side_effects" object in
//! res/json/items.json and res/json/npc.json.
struct SideEffects
{
  //! @brief Spawn (or relocate) an NPC.
  std::optional<SpawnNpcEffect> spawn_npc;
  //! @brief Lightning strike.
  std::optional<LightningEffect> lightning;
  //! @brief Movement/pathfinding effect.
  std::optional<MovementEffect> movement;
  //! @brief Whether there are no effects at all.
  //! @return true if no effect is set.
  [[nodiscard]] bool empty() const { return not spawn_npc and not lightning and not movement; }
};

//! @brief Optional reward granted when an action is applied. See the "reward" object of the sacrifice_action,
//! forage_action, destroy_action and proximity_action entries in res/json/items.json.
//! @details A sacrifice_action reward sets exactly one of item, wealth or special. A forage_action/destroy_action
//! reward only sets item. A proximity_action reward sets exactly one of item or loot, dropped beside the source.
struct ActionReward
{
  //! @brief A special altar effect triggered by sacrificing the item.
  enum class Special {
    //! @brief No special effect.
    NONE,
    //! @brief Light an altar candle (relics).
    ALTAR,
    //! @brief Banish ghosts, or gain an extra life at a crypt altar (witches jar).
    BANISH,
    //! @brief Unlock the crypt key (exit key).
    CRYPT
  };

  //! @brief The item gained. Empty string if unused.
  Sys::ItemKey item;
  //! @brief The wealth gained. Zero if unused.
  int wealth{ 0 };
  //! @brief The special altar effect. NONE if unused.
  Special special{ Special::NONE };
  //! @brief The loot sprite dropped (proximity_action only). Empty string if unused.
  Sys::SpriteKey loot;
  //! @brief Distance in blocks from the source that the player must be within before the reward drops (proximity_action only).
  int trigger_range{ 1 };
  //! @brief Max drops over each source item instance's lifetime, picked up or not; further drops are skipped. 0 = unlimited
  //! (proximity_action only).
  int max_instances{ 0 };
  //! @brief The number of reward fields that are set.
  //! @return int Count of item, wealth, special and loot that are not their unused value.
  [[nodiscard]] int count() const
  {
    return static_cast<int>( not item.empty() ) + static_cast<int>( wealth > 0 ) + static_cast<int>( special != Special::NONE ) +
           static_cast<int>( not loot.empty() );
  }
};

//! @brief Base class for a player-stat modifier applied when a particular kind of gameplay action
//! occurs (e.g. burying, carrying, colliding with, consuming, destroying, or being hit by something).
//! @details Holds the health/fear/despair/infamy/toxicity deltas, an optional toxidrome affliction, and
//! the tick interval at which the modifier re-applies, plus optional side effects (see SideEffects) and an
//! optional reward (see ActionReward).
//! Derived classes (BurnAction, CarryAction, CollisionAction, ConsumeAction, DestroyAction, ForageAction,
//! ProjectileAction, ProximityAction, SacrificeAction, SpawnAction) add no behaviour of their own - they exist purely so that Cmp::WorldItem::actions
//! and NPC action maps can key modifiers by the std::type_index of the triggering action kind.
class BaseAction
{
public:
  //! @brief Construct a new Base Action object with no toxidrome affliction.
  //! @param health Change applied to the health stat.
  //! @param fear Change applied to the fear stat.
  //! @param despair Change applied to the despair stat.
  //! @param infamy Change applied to the infamy stat.
  //! @param luck Change applied to the luck stat.
  //! @param tick How often (seconds) the action re-applies, or 0 for a one-shot.
  BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick );
  //! @brief Construct a new Base Action object.
  //! @param health Change applied to the health stat.
  //! @param fear Change applied to the fear stat.
  //! @param despair Change applied to the despair stat.
  //! @param infamy Change applied to the infamy stat.
  //! @param luck Change applied to the luck stat.
  //! @param tick How often (seconds) the action re-applies, or 0 for a one-shot.
  //! @param toxidrome toxidrome affliction applied alongside the stat changes, if any. Each active
  //! toxidrome carries its own toxicity contribution (see Cmp::Toxicity::Toxidrome).
  //! @param side_effects Optional side effects (NPC spawn, lightning, movement), each with its own probability.
  //! @param reward Optional reward granted when the action is applied (e.g. forage/chop/sacrifice output).
  BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick,
              Cmp::Toxicity::Toxidrome toxidrome, SideEffects side_effects = {}, std::optional<ActionReward> reward = {} );
  //! @brief Destroy the Base Action object.
  ~BaseAction();

  BaseAction( const BaseAction &other );
  BaseAction &operator=( const BaseAction &other );
  BaseAction( BaseAction &&other ) noexcept;
  BaseAction &operator=( BaseAction &&other ) noexcept;

  //! @brief Get the health delta.
  //! @return int The change applied to the health stat.
  [[nodiscard]] int health() const { return m_health; }
  //! @brief Get the fear delta.
  //! @return int The change applied to the fear stat.
  [[nodiscard]] int fear() const { return m_fear; }
  //! @brief Get the despair delta.
  //! @return int The change applied to the despair stat.
  [[nodiscard]] int despair() const { return m_despair; }
  //! @brief Get the infamy delta.
  //! @return int The change applied to the infamy stat.
  [[nodiscard]] int infamy() const { return m_infamy; }
  //! @brief Get the luck delta.
  //! @return int The change applied to the toxicity stat.
  [[nodiscard]] int luck() const { return m_luck; }
  //! @brief Get the re-apply interval.
  //! @return float How often (seconds) the action re-applies, or 0 for a one-shot.
  [[nodiscard]] float interval() const { return m_tick; }
  //! @brief Get the toxidrome affliction associated with this action.
  //! @return Stats::toxidrome The toxidrome type and its own tick interval.
  [[nodiscard]] const Cmp::Toxicity::Toxidrome &toxidrome() const;
  //! @brief Get the optional side effects of this action.
  //! @return const SideEffects& The NPC spawn/lightning/movement effects.
  [[nodiscard]] const SideEffects &side_effects() const { return m_side_effects; }
  //! @brief Get the optional reward of this action.
  //! @return const std::optional<ActionReward>& The reward, or std::nullopt if the action grants none.
  [[nodiscard]] const std::optional<ActionReward> &reward() const { return m_reward; }
  //! @brief Whether this action has no effect, i.e. every stat delta is zero, it applies no toxidrome and has no side effects
  //! or reward.
  //! @return true if applying this action would change nothing.
  [[nodiscard]] bool is_zero() const;

  //! @brief Accumulate another action's stat deltas into this one.
  //! @note Does not accumulate toxidrome, side effects or reward.
  //! @param rhs The action whose deltas are added to this one.
  //! @return BaseAction& Reference to this action, after accumulation.
  BaseAction &operator+=( const BaseAction &rhs )
  {
    m_health += rhs.m_health;
    m_fear += rhs.m_fear;
    m_despair += rhs.m_despair;
    m_infamy += rhs.m_infamy;
    m_luck += rhs.m_luck;
    m_tick += rhs.m_tick;
    return *this;
  }

private:
  //! @brief The change applied to the health stat.
  int m_health{ 0 };
  //! @brief The change applied to the fear stat.
  int m_fear{ 0 };
  //! @brief The change applied to the despair stat.
  int m_despair{ 0 };
  //! @brief The change applied to the infamy stat.
  int m_infamy{ 0 };
  //! @brief The change applied to the luck stat.
  int m_luck{ 0 };
  //! @brief The toxidrome affliction applied alongside the stat changes, if any. Never null; held behind
  //! a pointer only so this header need not include Toxidrome.hpp. Copy/move are hand-written in the
  //! .cpp (deep-copy for copy, pointer-steal for move) to keep BaseAction copy-constructible.
  std::unique_ptr<Cmp::Toxicity::Toxidrome> m_toxidrome;
  //! @brief Optional side effects (NPC spawn, lightning, movement).
  SideEffects m_side_effects;
  //! @brief Optional reward granted when the action is applied.
  std::optional<ActionReward> m_reward;
  //! @brief How often (seconds) the action re-applies, or 0 for a one-shot.
  float m_tick{ 0 };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_BASEACTION_HPP__
