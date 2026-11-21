#ifndef SRC_COMPONENTS_STATS_BASEACTION_HPP__
#define SRC_COMPONENTS_STATS_BASEACTION_HPP__

#include <Components/Altar/Kind.hpp>
#include <Components/Stats/BaseActionFwd.hpp>
#include <Components/Stats/StatDeltas.hpp>
#include <SFML/System/Time.hpp>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace Game::Cmp::Toxicity
{
class Toxidrome;
} // namespace Game::Cmp::Toxicity

namespace Game::Cmp
{

// ActionRewards (std::vector<ActionReward>) is declared in BaseActionFwd.hpp

//! @brief How an item wears while one of its actions is in effect. See "wear" in res/json/items.json: on spawn_action it
//! wears in the world, on carry_action in the inventory, on proximity_action each time the action is applied.
struct ActionWear
{
  //! @brief Seconds for the item to wear out completely while the action is in effect. 0 = no time decay, the item only
  //! wears through use (tools, gloves, foraging).
  float expiry_seconds{ 0.f };
  //! @brief Only wear while the action is active. carry_action: the item is in use (e.g. flute playing, broom flying).
  //! proximity_action: applying the action changed a player stat.
  bool active_only{ false };
};

//! @brief Sound effects played for an action. See the optional "sfx" object of an action in res/json/items.json and
//! res/json/npc.json. An empty name plays nothing.
struct ActionSfx
{
  //! @brief Plays while the action is in progress, e.g. eating/drinking, an intermediate shovel hit on a plant, a plant burning.
  std::string loop;
  //! @brief Plays when the action completes or is applied.
  std::string final;
};

//! @brief Base class for a player-stat modifier applied when a particular kind of gameplay action
//! occurs (e.g. burying, carrying, colliding with, consuming, destroying, or being hit by something).
//! @details Holds the health/fear/despair/infamy/toxicity deltas, an optional toxidrome affliction, and
//! the tick interval at which the modifier re-applies, plus optional side effects (see SideEffects) and an
//! optional reward (see ActionReward).
//! Derived classes (BurnAction, CarryAction, CollisionAction, ConsumeAction, DestroyAction, ForageAction,
//! ProjectileAction, ProximityAction, OfferingAction, SpawnAction) add no behaviour of their own - they exist purely so that Cmp::WorldItem::actions
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
  BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck,
              Stats::Tick tick );
  //! @brief Construct a new Base Action object with no side effects or rewards.
  //! @param toxidrome toxidrome affliction applied alongside the stat changes, if any.
  BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck,
              Stats::Tick tick, Cmp::Toxicity::Toxidrome toxidrome );
  //! @brief Construct a new Base Action object with no rewards.
  //! @param toxidrome toxidrome affliction applied alongside the stat changes, if any.
  //! @param side_effects Optional side effects (NPC spawn, lightning, movement), each with its own probability.
  BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck,
              Stats::Tick tick, Cmp::Toxicity::Toxidrome toxidrome, SideEffects side_effects );
  //! @brief Construct a new Base Action object.
  //! @note Overloads are used instead of default arguments so that this header doesn't need the complete
  //! Toxidrome, SideEffects and ActionReward types.
  //! @param health Change applied to the health stat.
  //! @param fear Change applied to the fear stat.
  //! @param despair Change applied to the despair stat.
  //! @param infamy Change applied to the infamy stat.
  //! @param luck Change applied to the luck stat.
  //! @param tick How often (seconds) the action re-applies, or 0 for a one-shot.
  //! @param toxidrome toxidrome affliction applied alongside the stat changes, if any. Each active
  //! toxidrome carries its own toxicity contribution (see Cmp::Toxicity::Toxidrome).
  //! @param side_effects Optional side effects (NPC spawn, lightning, movement), each with its own probability.
  //! @param rewards Optional rewards granted when the action is applied (e.g. forage/chop/offering output).
  BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck,
              Stats::Tick tick, Cmp::Toxicity::Toxidrome toxidrome, SideEffects side_effects, ActionRewards rewards );
  //! @brief Destroy the Base Action object.
  ~BaseAction();

  //! @brief Copies share the immutable toxidrome, side effects and rewards; only the stat deltas, wear and sfx are copied.
  BaseAction( const BaseAction &other );
  BaseAction &operator=( const BaseAction &other );
  BaseAction( BaseAction &&other ) noexcept;
  BaseAction &operator=( BaseAction &&other ) noexcept;

  //! @brief Get the health delta.
  //! @return int The change applied to the health stat.
  [[nodiscard]] int health() const { return m_health; }
  //! @brief Drop any damage from this action, leaving a health gain and the other stat deltas as they are.
  void remove_damage() { m_health = std::max( m_health, 0 ); }
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
  //! @brief Get how the source item wears while this action is in effect ("wear" in items.json): spawn_action = in the
  //! world, carry_action = in the inventory, proximity_action = each time it is applied to the player.
  //! @return const std::optional<ActionWear>& The wear, or std::nullopt if the item doesn't wear during this action.
  [[nodiscard]] const std::optional<ActionWear> &wear() const { return m_wear; }
  //! @brief Set how the source item wears while this action is in effect, see wear().
  //! @param wear The wear settings.
  void set_wear( ActionWear wear ) { m_wear = wear; }
  //! @brief Get the sound effects of this action ("sfx" in items.json/npc.json).
  //! @return const ActionSfx& The loop and final sound names, empty if unused.
  [[nodiscard]] const ActionSfx &sfx() const { return m_sfx; }
  //! @brief Set the sound effects of this action, see sfx().
  //! @param sfx The sound effect names.
  void set_sfx( ActionSfx sfx ) { m_sfx = std::move( sfx ); }
  //! @brief Get the toxidrome affliction associated with this action.
  //! @return Stats::toxidrome The toxidrome type and its own tick interval.
  [[nodiscard]] const Cmp::Toxicity::Toxidrome &toxidrome() const;
  //! @brief Get the optional side effects of this action.
  //! @return const SideEffects& The NPC spawn/lightning/movement effects.
  [[nodiscard]] const SideEffects &side_effects() const;
  //! @brief Get the optional reward of this action, for actions that have at most one (i.e. not altar specific).
  //! @return std::optional<ActionReward> The first reward, or std::nullopt if the action grants none.
  [[nodiscard]] std::optional<ActionReward> reward() const;
  //! @brief Get the reward of this action granted at the given altar kind.
  //! @param location The kind of altar the action is applied at.
  //! @return std::optional<ActionReward> The reward for that location, else the reward for any location, else std::nullopt.
  [[nodiscard]] std::optional<ActionReward> reward( Altar::Kind location ) const;
  //! @brief Get all rewards of this action.
  //! @return const ActionRewards& The rewards, empty if the action grants none.
  [[nodiscard]] const ActionRewards &rewards() const;
  //! @brief Whether this action has no effect, i.e. every stat delta is zero, it applies no toxidrome and has no side effects
  //! or reward.
  //! @return true if applying this action would change nothing.
  [[nodiscard]] bool is_zero() const;

  //! @brief Accumulate another action's stat deltas into this one.
  //! @note Does not accumulate toxidrome, side effects, reward or sfx.
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
  // The toxidrome, side effects and rewards are set once at construction and never modified, so copies share them.
  // They are held behind pointers so this header needs only forward declarations of their types. Never null, except
  // in a moved-from object.

  //! @brief The toxidrome affliction applied alongside the stat changes, if any.
  std::shared_ptr<const Cmp::Toxicity::Toxidrome> m_toxidrome;
  //! @brief Optional side effects (NPC spawn, lightning, movement).
  std::shared_ptr<const SideEffects> m_side_effects;
  //! @brief Optional rewards granted when the action is applied.
  std::shared_ptr<const ActionRewards> m_rewards;
  //! @brief How often (seconds) the action re-applies, or 0 for a one-shot.
  float m_tick{ 0 };
  //! @brief How the source item wears while the action is in effect, see wear().
  std::optional<ActionWear> m_wear;
  //! @brief Sound effects played for the action, see sfx().
  ActionSfx m_sfx;
};

//! @brief An action with no effect: zero stat deltas, one-shot tick, and no toxidrome, side effects, rewards, wear
//! or sfx. Returned for any action an item or NPC omits from res/json/items.json or res/json/npc.json.
[[nodiscard]] inline BaseAction no_action() { return BaseAction( { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, { 0.f } ); }

//! @brief Pairs a stat-modifier action with the timestamp/duration used to schedule its re-application.
struct ActionTimePair
{
  //! @brief The stat modifier to apply.
  BaseAction action;
  //! @brief The time associated with the action (e.g. last-applied time or remaining duration).
  sf::Time time;
};

//! @brief The actions of an item or NPC keyed by the std::type_index of their concrete type, see Cmp::ActionSet.
//! A struct rather than an alias so it can be forward declared in BaseActionFwd.hpp.
struct ActionMap : std::unordered_map<std::type_index, ActionTimePair>
{
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_BASEACTION_HPP__
