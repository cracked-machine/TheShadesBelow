#ifndef SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__
#define SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__

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
    //! @brief Drops a relic item.
    RELIC,
    //! @brief Drops jewelry loot.
    JEWELRY,
    //! @brief Drops a curse tablet
    CURSE_TABLET
  };

  //! @param roll Fixed roulette roll in the range [0, 99], decided once at level gen
  Consequence( int roll )
      : m_roll( roll )
  {
  }

  //! @brief Resolve the consequence from the grave's fixed roll and the player's current luck.
  //! @note The roll never changes, so the result only changes when player luck changes.
  //! @param player_luck
  [[nodiscard]] Type get( int player_luck ) const
  {
    // Roulette wheel. We take the inverse of player luck and use both numbers to gate the spawn probablities.
    // For example, if player luck is 50, then the badluck is also 50
    const int good_weight = player_luck;
    const int bad_weight = 100 - good_weight;

    const int tier1_threshold = bad_weight / 2;                   // Tier1 is a roll below 25
    const int tier2_threshold = ( bad_weight / 2 ) + 5;           // Tier2 is a roll between 25 and 30
    const int tier3_threshold = bad_weight;                       // Tier3 is a roll between 30 and 50
    const int tier4_threshold = bad_weight + ( good_weight / 2 ); // Tier4 is a roll between 50 and 75
                                                                  // Remaining rolls between 75 and 100

    if ( m_roll < tier1_threshold ) { return Type::BOMB_TRAP; }
    if ( m_roll < tier2_threshold ) { return Type::CURSE_TABLET; }
    if ( m_roll < tier3_threshold ) { return Type::NPC_TRAP; }
    if ( m_roll < tier4_threshold ) { return Type::RELIC; }
    return Type::JEWELRY;
  }

private:
  int m_roll;
};

//! @brief Lets fmt/spdlog print the consequence type as its underlying value
inline int format_as( Consequence::Type type ) { return static_cast<int>( type ); }

} // namespace Game::Cmp::Grave

#endif // SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__