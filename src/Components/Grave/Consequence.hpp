#ifndef SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__
#define SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

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
    //! @brief Drops altar blood (see Cmp::Altar::BloodCount). Its band is the guaranteed floor, see
    //! Cmp::Persist::BloodMinChance.
    BLOOD,
    //! @brief Drops a curse tablet
    CURSE_TABLET,
    //! @brief Drops a talisman.
    TALISMAN,
    //! @brief Drops a cadaver item.
    CADAVER,
    //! @brief Drops a torch item.
    TORCH,
  };

  //! @brief A consequence's rank on the line, 0 = worst and 100 = best. Below 50 is bad, above 50 is good. A negative
  //! value disables the consequence: make_line() leaves it off the line.
  struct Slot
  {
    Type type;
    int value;
  };

  //! @brief Number of consequence types, and so the number of slots given to make_line()
  static constexpr std::size_t kNumTypes = 7;

  //! @brief One slot per consequence type, in any order, as given to make_line()
  using Slots = std::array<Slot, kNumTypes>;

  //! @brief The drop line, sorted by value, holding only the enabled slots. A roll lands on the slot whose value is
  //! nearest, so each slot owns the range halfway to its neighbours. Slots with equal values form a group that shares
  //! one band; the grave's fixed roll picks which of the group it gets.
  using Line = std::vector<Slot>;

  //! @brief Build a valid line: drops disabled (negative) slots, caps the rest at 100 and sorts by value. The sort is
  //! stable, so equal values keep the order they were given in. Empty when every slot is disabled.
  //! @param slots one slot per consequence type, in any order
  [[nodiscard]] static Line make_line( const Slots &slots )
  {
    Line line;
    for ( const auto &slot : slots )
    {
      if ( slot.value < 0 ) continue;
      line.push_back( Slot{ .type = slot.type, .value = std::min( slot.value, 100 ) } );
    }
    std::ranges::stable_sort( line, []( const Slot &a, const Slot &b ) { return a.value < b.value; } );
    return line;
  }

  //! @brief Lowest and highest skewed values that always resolve to the blood slot's group, whatever the luck tie-break
  [[nodiscard]] static int blood_low_edge( const Line &line ) { return band_edges( line, Type::BLOOD )[0]; }
  [[nodiscard]] static int blood_high_edge( const Line &line ) { return band_edges( line, Type::BLOOD )[1]; }

  //! @brief The largest blood floor the line can guarantee: the width of the blood slot's band. Can be 0 or
  //! less when the neighbouring slots squeeze the blood band to nothing.
  [[nodiscard]] static int max_blood_floor_pct( const Line &line ) { return blood_high_edge( line ) - blood_low_edge( line ) + 1; }

  //! @param roll Fixed roll in the range [0, 100], decided once at level gen
  //! @param min_blood_chance_pct Lowest chance (percent) of blood however bad the player's luck, from
  //! Cmp::Persist::BloodMinChance at level gen. Capped at max_blood_floor_pct().
  //! @param line The drop line at level gen, see make_line(). Must not be empty. Kept so later setting changes only
  //! affect new levels.
  Consequence( int roll, int min_blood_chance_pct, const Line &line )
      : m_line( line ),
        m_roll( roll ),
        m_min_blood_chance_pct( std::clamp( min_blood_chance_pct, 0, std::max( 0, max_blood_floor_pct( line ) ) ) )
  {
  }

  //! @brief Resolve the consequence from the grave's fixed roll and the player's current luck.
  //! @note The roll never changes, so the result only changes when player luck changes.
  //! @param player_luck 0-100, 50 is neutral
  [[nodiscard]] Type roll( int player_luck ) const
  {
    // luck skews the roll towards the good (high) or bad (low) end of the line. The downward skew is limited so the
    // top m_min_blood_chance_pct rolls still land in the blood band.
    const int blood_floor_offset = blood_low_edge( m_line ) + m_min_blood_chance_pct - 1 - 100;
    const int offset = std::max( player_luck - 50, blood_floor_offset );
    const int skewed = std::clamp( m_roll + offset, 0, 100 );
    const bool lucky = player_luck >= 50;
    // compare doubled values so the halfway point stays an integer
    const int twice_skewed = skewed * 2;

    for ( std::size_t first = 0; first < m_line.size(); )
    {
      const std::size_t last = group_last( m_line, first );
      if ( last + 1 == m_line.size() ) { return pick( first, last ); }
      const int twice_midpoint = m_line[first].value + m_line[last + 1].value;
      if ( twice_skewed < twice_midpoint ) { return pick( first, last ); }
      if ( twice_skewed == twice_midpoint ) { return lucky ? pick( last + 1, group_last( m_line, last + 1 ) ) : pick( first, last ); }
      first = last + 1;
    }
    return m_line.back().type;
  }

private:
  //! @brief Index of the last slot in the group of equal values that starts at first
  [[nodiscard]] static std::size_t group_last( const Line &line, std::size_t first )
  {
    std::size_t last = first;
    while ( last + 1 < line.size() and line[last + 1].value == line[first].value )
      ++last;
    return last;
  }

  //! @brief Choose one type from the group [first, last]. A single slot is returned as is. Rolls protected by the
  //! blood floor always get the blood, so the floor still holds; other rolls pick by the grave's fixed roll.
  [[nodiscard]] Type pick( std::size_t first, std::size_t last ) const
  {
    if ( first == last ) return m_line[first].type;
    const bool floor_protected = m_min_blood_chance_pct > 0 and m_roll > 100 - m_min_blood_chance_pct;
    if ( floor_protected )
    {
      for ( std::size_t i = first; i <= last; ++i )
      {
        if ( m_line[i].type == Type::BLOOD ) return Type::BLOOD;
      }
    }
    const auto group_size = static_cast<int>( last - first + 1 );
    return m_line[first + static_cast<std::size_t>( std::max( 0, m_roll ) % group_size )].type;
  }

  //! @brief Inclusive [low, high] skewed values that resolve to this type's group, leaving out the tie points
  [[nodiscard]] static std::array<int, 2> band_edges( const Line &line, Type type )
  {
    for ( std::size_t i = 0; i < line.size(); ++i )
    {
      if ( line[i].type != type ) continue;
      std::size_t first = i;
      while ( first > 0 and line[first - 1].value == line[i].value )
        --first;
      const std::size_t last = group_last( line, first );
      const int cur = line[i].value;
      const int low = ( first == 0 ) ? 0 : ( ( line[first - 1].value + cur ) / 2 ) + 1;
      const int high = ( last + 1 == line.size() ) ? 100 : ( ( cur + line[last + 1].value + 1 ) / 2 ) - 1;
      return { low, high };
    }
    return { 0, -1 };
  }

  Line m_line;
  int m_roll;
  int m_min_blood_chance_pct;
};

//! @brief Lets fmt/spdlog print the consequence type as its underlying value
inline int format_as( Consequence::Type type ) { return static_cast<int>( type ); }

} // namespace Game::Cmp::Grave

#endif // SRC_COMPONENTS_GRAVE_CONSEQUENCE_HPP__
