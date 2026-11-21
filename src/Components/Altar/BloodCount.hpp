#ifndef SRC_COMPONENTS_ALTAR_BLOODCOUNT_HPP__
#define SRC_COMPONENTS_ALTAR_BLOODCOUNT_HPP__

#include <algorithm>

namespace Game::Cmp::Altar
{

//! @brief The blood offered at a graveyard altar (poured from blood vials, see Cmp::WorldItem::get_blood_heal()). Once it reaches
//! Cmp::Persist::AltarBloodQuota the altar activates and a crow leads the player to the key grave (see Sys::AltarSystem).
class BloodCount
{
public:
  //! @brief Add offered blood, capped at the quota
  //! @param blood The blood offered
  //! @param quota The altar's blood quota, Cmp::Persist::AltarBloodQuota
  void add( int blood, int quota ) { m_value = std::clamp( m_value + blood, 0, std::max( 0, quota ) ); }

  //! @brief The blood offered so far
  [[nodiscard]] int value() const { return m_value; }

  //! @brief Whether the quota has been reached, so the altar activates
  [[nodiscard]] bool is_full( int quota ) const { return m_value >= quota; }

  //! @brief The fraction of the quota offered, 0 to 1
  [[nodiscard]] float fraction( int quota ) const
  {
    if ( quota <= 0 ) return 1.f;
    return std::clamp( static_cast<float>( m_value ) / static_cast<float>( quota ), 0.f, 1.f );
  }

  //! @brief Empty the altar, so it takes its full quota again
  void reset() { m_value = 0; }

private:
  int m_value{ 0 };
};

} // namespace Game::Cmp::Altar

#endif // SRC_COMPONENTS_ALTAR_BLOODCOUNT_HPP__
