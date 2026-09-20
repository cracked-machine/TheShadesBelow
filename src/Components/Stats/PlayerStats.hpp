#ifndef SRC_COMPONENTS_STATS_PLAYERSTATS_HPP__
#define SRC_COMPONENTS_STATS_PLAYERSTATS_HPP__

#include <Components/Stats/BaseAction.hpp>

namespace Game::Cmp
{

//! @brief Holds the player's core stats (health, fear, despair, infamy, toxicity, toxidrome), each
//! clamped to [0, 100], and applies BaseAction-derived modifiers to them.
class PlayerStats
{
public:
  //! @brief Construct a new Player Stats object.
  //! @param health Initial health value, clamped to [0, 100].
  //! @param fear Initial fear value, clamped to [0, 100].
  //! @param despair Initial despair value, clamped to [0, 100].
  //! @param infamy Initial infamy value, clamped to [0, 100].
  //! @param toxicity Initial toxicity value, clamped to [0, 100].
  //! @param toxidrome Initial toxidrome affliction, if any.
  PlayerStats( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Toxicity toxicity, Stats::Luck luck,
               Cmp::Toxicity::Toxidrome toxidrome = {} )
      : m_health( std::clamp( health.value, 0, 100 ) ),
        m_fear( std::clamp( fear.value, 0, 100 ) ),
        m_despair( std::clamp( despair.value, 0, 100 ) ),
        m_infamy( std::clamp( infamy.value, 0, 100 ) ),
        m_toxicity( std::clamp( toxicity.value, 0, 100 ) ),
        m_luck( std::clamp( luck.value, 0, 100 ) ),
        m_toxidrome( std::move( toxidrome ) )
  {
  }
  //! @brief Destroy the Player Stats object.
  ~PlayerStats() {}

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
  [[nodiscard]] int toxicity() const { return m_toxicity; }
  //! @brief Get the player's current luck.
  //! @return int Luck value, in [0, 100].
  [[nodiscard]] int luck() const { return m_luck; }
  //! @brief Get the player's current toxidrome affliction.
  //! @return Stats::toxidrome The toxidrome type and its tick interval.
  [[nodiscard]] Cmp::Toxicity::Toxidrome toxidrome() const { return m_toxidrome; }

  //! @brief Update the player stats with the BaseAction object.
  //! @note BaseAction: health, fear, despair, infamy, toxicity, luck, toxidrome. toxidrome is only overwritten
  //! if the player is already afflicted (i.e. it does not newly infect a healthy player).
  //! @param action The stat modifier to apply.
  void apply( const BaseAction &action )
  {
    m_health = std::clamp( m_health + action.health(), 0, 100 );
    m_fear = std::clamp( m_fear + action.fear(), 0, 100 );
    m_despair = std::clamp( m_despair + action.despair(), 0, 100 );
    m_infamy = std::clamp( m_infamy + action.infamy(), 0, 100 );
    m_toxicity = std::clamp( m_toxicity + action.toxicity(), 0, 100 );
    m_luck = std::clamp( m_luck + action.luck(), 0, 100 );
    m_toxidrome = action.toxidrome();
  }

private:
  //! @brief The player's current health, in [0, 100].
  int m_health{ 0 };
  //! @brief The player's current fear, in [0, 100].
  int m_fear{ 0 };
  //! @brief The player's current despair, in [0, 100].
  int m_despair{ 0 };
  //! @brief The player's current infamy, in [0, 100].
  int m_infamy{ 0 };
  //! @brief The player's current toxicity, in [0, 100].
  int m_toxicity{ 0 };
  //! @brief The player's current luck, in [0, 100].
  int m_luck{ 50 };
  //! @brief The player's current toxidrome affliction, if any.
  Cmp::Toxicity::Toxidrome m_toxidrome{};
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_STATS_PLAYERSTATS_HPP__
