#ifndef SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__
#define SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__

#include <algorithm>

namespace Game::Cmp::Grave
{

class Consequence
{
public:
  //! @brief Consequence triggered when a grave is fully dug open
  enum class Type {
    //! @brief Spawns a hostile NPC from the grave.
    NPC_TRAP = 1,
    //! @brief Spawns a bomb trap from the grave.
    BOMB_TRAP,
    //! @brief Drops jewelry loot.
    JEWELRY,
    //! @brief Drops a curse tablet
    CURSE_TABLET,
    //! @brief Drops a talisman that an altar takes.
    TALISMAN
  };

  //! @param roll Fixed roulette roll in the range [0, 99], decided once at level gen
  //! @param min_talisman_chance_pct Lowest chance (percent) of a talisman however bad the player's luck, from
  //! Cmp::Persist::TalismanMinChance at level gen. Matches the chance at twice that luck, so luck only changes the
  //! talisman odds below that.
  Consequence( int roll, int min_talisman_chance_pct )
      : m_roll( roll ),
        m_min_talisman_chance_pct( std::clamp( min_talisman_chance_pct, 0, 100 ) )
  {
  }

  //! @brief Chance (percent) of a grave giving a talisman at this luck: half the luck, but never below the floor
  //! @param player_luck
  //! @param min_talisman_chance_pct see Consequence()
  [[nodiscard]] static constexpr int talisman_chance_pct( int player_luck, int min_talisman_chance_pct )
  {
    return std::max( player_luck / 2, min_talisman_chance_pct );
  }

  //! @brief Resolve the consequence from the grave's fixed roll and the player's current luck.
  //! @note The roll never changes, so the result only changes when player luck changes.
  //! @param player_luck
  [[nodiscard]] Type roll( int player_luck ) const
  {
    // Roulette wheel. We take the inverse of player luck and use both numbers to gate the spawn probablities.
    // For example, if player luck is 50, then the badluck is also 50
    const int good_weight = player_luck;
    const int talisman_weight = talisman_chance_pct( player_luck, m_min_talisman_chance_pct );
    // at very low luck the talisman floor takes its share from the bad tiers, so the wheel still adds up to 100
    const int bad_weight = std::min( 100 - good_weight, 100 - talisman_weight );

    const int tier1_threshold = bad_weight / 2;                // Tier1 is a roll below 25
    const int tier2_threshold = ( bad_weight / 2 ) + 5;        // Tier2 is a roll between 25 and 30
    const int tier3_threshold = bad_weight;                    // Tier3 is a roll between 30 and 50
    const int tier4_threshold = bad_weight + talisman_weight;  // Tier4 is a roll between 50 and 75
                                                               // Remaining rolls between 75 and 100

    if ( m_roll < tier1_threshold ) { return Type::BOMB_TRAP; }
    if ( m_roll < tier2_threshold ) { return Type::CURSE_TABLET; }
    if ( m_roll < tier3_threshold ) { return Type::NPC_TRAP; }
    if ( m_roll < tier4_threshold ) { return Type::TALISMAN; }
    return Type::JEWELRY;
  }

private:
  int m_roll;
  int m_min_talisman_chance_pct;
};

//! @brief Lets fmt/spdlog print the consequence type as its underlying value
inline int format_as( Consequence::Type type ) { return static_cast<int>( type ); }

} // namespace Game::Cmp::Grave

#endif // SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__