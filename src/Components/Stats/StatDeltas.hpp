#ifndef SRC_COMPONENTS_STATS_STATDELTAS_HPP__
#define SRC_COMPONENTS_STATS_STATDELTAS_HPP__

//! @file
//! @brief Strongly-typed stat deltas shared by Cmp::BaseAction and Cmp::PlayerStats. Kept separate from
//! BaseAction.hpp so headers that only need these types (e.g. PlayerStats.hpp) don't depend on BaseAction.

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

//! @brief Strongly-typed luck delta, used to disambiguate BaseAction/derived-action constructor arguments.
struct Luck
{
  //! @brief The change applied to the luck stat.
  int value{ 0 };
};

//! @brief Strongly-typed anima delta, used to disambiguate BaseAction/derived-action constructor arguments.
struct Anima
{
  //! @brief The change applied to the anima stat.
  int value{ 0 };
};

//! @brief Strongly-typed tick interval, used to disambiguate BaseAction/derived-action constructor arguments.
struct Tick
{
  //! @brief How often (seconds) the action re-applies, or 0 for a one-shot.
  float value{ 0 };
};

} // namespace Stats

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_STATDELTAS_HPP__
