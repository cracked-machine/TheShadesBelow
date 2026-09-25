#ifndef SRC_COMPONENTS_STATS_PLAYERSTATS_HPP__
#define SRC_COMPONENTS_STATS_PLAYERSTATS_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <algorithm>
#include <memory>

namespace Game::Cmp::Toxicity
{
class Toxidrome;
} // namespace Game::Cmp::Toxicity

namespace Game::Cmp
{

//! @brief Holds the player's core stats (health, fear, despair, infamy, toxicity, toxidrome), each
//! clamped to [0, 100], and applies BaseAction-derived modifiers to them.
class PlayerStats
{
public:
  //! @brief Construct a new Player Stats object with no toxidrome affliction.
  //! @param health Initial health value, clamped to [0, 100].
  //! @param fear Initial fear value, clamped to [0, 100].
  //! @param despair Initial despair value, clamped to [0, 100].
  //! @param infamy Initial infamy value, clamped to [0, 100].
  PlayerStats( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck );
  //! @brief Construct a new Player Stats object.
  //! @param health Initial health value, clamped to [0, 100].
  //! @param fear Initial fear value, clamped to [0, 100].
  //! @param despair Initial despair value, clamped to [0, 100].
  //! @param infamy Initial infamy value, clamped to [0, 100].
  //! @param toxidrome Initial toxidrome affliction.
  PlayerStats( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck,
               Cmp::Toxicity::Toxidrome toxidrome );
  //! @brief Destroy the Player Stats object.
  ~PlayerStats();

  PlayerStats( const PlayerStats &other );
  PlayerStats &operator=( const PlayerStats &other );
  PlayerStats( PlayerStats &&other ) noexcept;
  PlayerStats &operator=( PlayerStats &&other ) noexcept;

  //! @brief Get the player's current health.
  //! @return int Health value, in [0, 100].
  [[nodiscard]] int health() const { return m_health; }
  //! @brief Get the player's current fear.
  //! @return int Fear value, in [0, 100].
  [[nodiscard]] int fear() const { return m_fear; }
  //! @brief Get the player's current despair.
  //! @return int Despair value, in [0, 100].
  [[nodiscard]] int despair() const { return m_despair; }
  //! @brief Get the player's current infamy.
  //! @return int Infamy value, in [0, 100].
  [[nodiscard]] int infamy() const { return m_infamy; }
  //! @brief Get the player's current toxicity.
  //! @return int Toxicity value, in [0, 100].
  [[nodiscard]] int luck() const { return m_luck; }
  //! @brief Get the player's current toxidrome affliction.
  //! @return Stats::toxidrome The toxidrome type and its tick interval.
  [[nodiscard]] const Cmp::Toxicity::Toxidrome &toxidrome() const;

  //! @brief Cures the player's toxidromes over time (e.g. while resting at a healing spring). Reduces
  //! every active toxidrome's own toxicity contribution by up to `amount`, removing any that reach
  //! zero, and reduces the toxicity stat by the total actually removed.
  //! @param amount Maximum reduction applied to each active toxidrome's own toxicity value.
  void decay_all_toxidrome( int amount );

  //! @brief Update the player stats with the BaseAction object.
  //! @note BaseAction: health, fear, despair, infamy, luck, toxidrome. Each toxidrome carried by the
  //! action is merged into the player's existing set individually (an excluded one instead fights the
  //! opposing toxidrome down and takes over once it's gone, per Cmp::Toxicity::Toxidrome::add), rather
  //! than replacing the player's whole set outright. The player's toxicity stat is bumped by exactly
  //! the toxidromes that were actually added.
  //! @param action The stat modifier to apply.
  void apply( const BaseAction &action );

private:
  //! @brief The player's current health, in [0, 100].
  int m_health{ 0 };
  //! @brief The player's current fear, in [0, 100].
  int m_fear{ 0 };
  //! @brief The player's current despair, in [0, 100].
  int m_despair{ 0 };
  //! @brief The player's current infamy, in [0, 100].
  int m_infamy{ 0 };
  //! @brief The player's current luck, in [0, 100].
  int m_luck{ 50 };
  //! @brief The player's current toxidrome affliction. Never null; held behind a pointer only so this
  //! header need not include Toxidrome.hpp. Copy/move are hand-written in the .cpp (deep-copy for
  //! copy, pointer-steal for move) to keep PlayerStats copy-constructible (RegistryTransfer relies on
  //! generically copying this component across scene transitions).
  std::unique_ptr<Cmp::Toxicity::Toxidrome> m_toxidrome;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_PLAYERSTATS_HPP__
