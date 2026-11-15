#ifndef SRC_CMPS_ALTAR_ARTEFACTCOUNT_HPP__
#define SRC_CMPS_ALTAR_ARTEFACTCOUNT_HPP__

#include <Components/Altar/ArtefactGroup.hpp>

#include <array>
#include <cstddef>
#include <vector>

namespace Game::Cmp::Altar
{

//! @brief Which artefact groups have been offered at a graveyard altar, one corner light per group. Once every group is
//! lit the player can pay anima to activate the altar (see Sys::AltarSystem).
//! @details Tracks groups, not item keys, so any item in a group fills that group's slot.
class ArtefactCount
{
public:
  //! @brief Mark the group as offered
  void activate( ArtefactGroup group ) { m_lit[index( group )] = true; }

  //! @brief Whether an artefact of the group has been offered
  [[nodiscard]] bool activated( ArtefactGroup group ) const { return m_lit[index( group )]; }

  //! @brief Number of groups offered
  [[nodiscard]] std::size_t count() const
  {
    std::size_t count = 0;
    for ( const bool lit : m_lit )
    {
      if ( lit ) count++;
    }
    return count;
  }

  //! @brief Number of groups the altar takes
  [[nodiscard]] static constexpr std::size_t size() { return kArtefactGroupCount; }

  //! @brief Whether every group has been offered, so the altar can be activated
  [[nodiscard]] bool all_lit() const { return count() == size(); }

  //! @brief The groups not yet offered, in slot order
  [[nodiscard]] std::vector<ArtefactGroup> missing_groups() const
  {
    std::vector<ArtefactGroup> missing;
    for ( std::size_t idx = 0; idx < m_lit.size(); ++idx )
    {
      if ( not m_lit[idx] ) missing.push_back( static_cast<ArtefactGroup>( idx ) );
    }
    return missing;
  }

  //! @brief Mark every group as not offered, so the altar takes all of them again
  void reset() { m_lit.fill( false ); }

  //! @brief The altar slot of a group: its candle in the altar sprite's particle_offsets
  [[nodiscard]] static constexpr std::size_t index( ArtefactGroup group ) { return static_cast<std::size_t>( group ); }

private:
  std::array<bool, kArtefactGroupCount> m_lit{};
};

} // namespace Game::Cmp::Altar

#endif // SRC_CMPS_ALTAR_ARTEFACTCOUNT_HPP__
