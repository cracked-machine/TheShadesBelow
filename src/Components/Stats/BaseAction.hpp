#ifndef SRC_COMPONENTS_STATS_BASEACTION_HPP__
#define SRC_COMPONENTS_STATS_BASEACTION_HPP__

#include <Components/Toxicity/Toxidrome.hpp>
#include <utility>
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

//! @brief Base class for a player-stat modifier applied when a particular kind of gameplay action
//! occurs (e.g. burying, carrying, colliding with, consuming, destroying, or being hit by something).
//! @details Holds the health/fear/despair/infamy/toxicity deltas, an optional toxidrome affliction, and
//! the tick interval at which the modifier re-applies. Derived classes (BuryAction, CarryAction,
//! CollisionAction, ConsumeAction, DestroyAction, ProjectileAction, ProximityAction, SacrificeAction,
//! SpawnAction) add no behaviour of their own - they exist purely so that Cmp::WorldItem::actions and
//! NPC action maps can key modifiers by the std::type_index of the triggering action kind.
class BaseAction
{
public:
  //! @brief Construct a new Base Action object.
  //! @param health Change applied to the health stat.
  //! @param fear Change applied to the fear stat.
  //! @param despair Change applied to the despair stat.
  //! @param infamy Change applied to the infamy stat.
  //! @param luck Change applied to the luck stat.
  //! @param tick How often (seconds) the action re-applies, or 0 for a one-shot.
  //! @param toxidrome toxidrome affliction applied alongside the stat changes, if any. Each active
  //! toxidrome carries its own toxicity contribution (see Cmp::Toxicity::Toxidrome).
  BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick,
              Cmp::Toxicity::Toxidrome toxidrome = {} )
      : m_health( health.value ),
        m_fear( fear.value ),
        m_despair( despair.value ),
        m_infamy( infamy.value ),
        m_luck( luck.value ),
        m_toxidrome( std::move( toxidrome ) ),
        m_tick( tick.value )
  {
  }
  //! @brief Destroy the Base Action object.
  ~BaseAction() {}

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
  [[nodiscard]] Cmp::Toxicity::Toxidrome toxidrome() const { return m_toxidrome; }

  //! @brief Accumulate another action's stat deltas into this one.
  //! @note Does not accumulate toxidrome.
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
  //! @brief The toxidrome affliction applied alongside the stat changes, if any.
  Cmp::Toxicity::Toxidrome m_toxidrome{};
  //! @brief How often (seconds) the action re-applies, or 0 for a one-shot.
  float m_tick{ 0 };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_BASEACTION_HPP__
