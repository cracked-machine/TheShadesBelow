#ifndef SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__
#define SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__

#include <algorithm>
#include <array>
#include <cstddef>

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
    TALISMAN,
    //! @brief Drops a torch item.
    TORCH
  };

  //! @brief A consequence's place on the line, 0 = worst and 100 = best. Below 50 is bad, above 50 is good.
  struct Slot
  {
    Type type;
    int value;
  };

  //! @brief The odds table. A roll lands on the slot whose value is nearest, so each slot owns the range halfway to
  //! its neighbours. Keep it sorted by value. To add a consequence, add a row.
  // clang-format off
  static constexpr std::array k_line{
      Slot{ .type=Type::CURSE_TABLET, .value=10 }, 
      Slot{ .type=Type::BOMB_TRAP,    .value=30 },    
      Slot{ .type=Type::TORCH,        .value=35 },        
      Slot{ .type=Type::NPC_TRAP,     .value=40 },     
      Slot{ .type=Type::TALISMAN,     .value=60 },     
      Slot{ .type=Type::JEWELRY,      .value=61 },      
  };
  // clang-format on

  //! @brief True when every value is in [0, 100] and the table is strictly increasing
  [[nodiscard]] static constexpr bool line_is_valid()
  {
    for ( std::size_t i = 0; i < k_line.size(); ++i )
    {
      if ( k_line[i].value < 0 or k_line[i].value > 100 ) return false;
      if ( i > 0 and k_line[i - 1].value >= k_line[i].value ) return false;
    }
    return true;
  }

  //! @brief Lowest and highest skewed values that always resolve to a talisman, whatever the luck tie-break
  [[nodiscard]] static constexpr int talisman_low_edge() { return band_edges( Type::TALISMAN )[0]; }
  [[nodiscard]] static constexpr int talisman_high_edge() { return band_edges( Type::TALISMAN )[1]; }

  //! @brief The largest talisman floor the table can guarantee: the width of the talisman band
  [[nodiscard]] static constexpr int max_talisman_floor_pct() { return talisman_high_edge() - talisman_low_edge() + 1; }

  //! @param roll Fixed roll in the range [0, 100], decided once at level gen
  //! @param min_talisman_chance_pct Lowest chance (percent) of a talisman however bad the player's luck, from
  //! Cmp::Persist::TalismanMinChance at level gen. Capped at max_talisman_floor_pct().
  Consequence( int roll, int min_talisman_chance_pct )
      : m_roll( roll ),
        m_min_talisman_chance_pct( std::clamp( min_talisman_chance_pct, 0, max_talisman_floor_pct() ) )
  {
  }

  //! @brief Resolve the consequence from the grave's fixed roll and the player's current luck.
  //! @note The roll never changes, so the result only changes when player luck changes.
  //! @param player_luck 0-100, 50 is neutral
  [[nodiscard]] Type roll( int player_luck ) const
  {
    // luck skews the roll towards the good (high) or bad (low) end of the line. The downward skew is limited so the
    // top m_min_talisman_chance_pct rolls still land in the talisman band.
    const int talisman_floor_offset = talisman_low_edge() + m_min_talisman_chance_pct - 1 - 100;
    const int offset = std::max( player_luck - 50, talisman_floor_offset );
    const int skewed = std::clamp( m_roll + offset, 0, 100 );
    const bool lucky = player_luck >= 50;

    for ( std::size_t i = 0; i + 1 < k_line.size(); ++i )
    {
      // compare doubled values so the halfway point stays an integer
      const int twice_skewed = skewed * 2;
      const int twice_midpoint = k_line[i].value + k_line[i + 1].value;
      if ( twice_skewed < twice_midpoint ) { return k_line[i].type; }
      if ( twice_skewed == twice_midpoint ) { return lucky ? k_line[i + 1].type : k_line[i].type; }
    }
    return k_line.back().type;
  }

private:
  //! @brief Inclusive [low, high] skewed values that resolve to this type, leaving out the tie points
  [[nodiscard]] static constexpr std::array<int, 2> band_edges( Type type )
  {
    for ( std::size_t i = 0; i < k_line.size(); ++i )
    {
      if ( k_line[i].type != type ) continue;
      const int cur = k_line[i].value;
      const int low = ( i == 0 ) ? 0 : ( k_line[i - 1].value + cur ) / 2 + 1;
      const int high = ( i + 1 == k_line.size() ) ? 100 : ( cur + k_line[i + 1].value + 1 ) / 2 - 1;
      return { low, high };
    }
    return { 0, -1 };
  }

  int m_roll;
  int m_min_talisman_chance_pct;
};

static_assert( Consequence::line_is_valid(), "Consequence::k_line values must be in [0, 100] and strictly increasing" );
static_assert( Consequence::max_talisman_floor_pct() > 0, "Consequence::k_line must contain a TALISMAN slot with a non-empty band" );

//! @brief Lets fmt/spdlog print the consequence type as its underlying value
inline int format_as( Consequence::Type type ) { return static_cast<int>( type ); }

} // namespace Game::Cmp::Grave

#endif // SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__
