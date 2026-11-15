#ifndef SRC_COMPONENTS_ALTAR_ARTEFACTGROUP_HPP__
#define SRC_COMPONENTS_ALTAR_ARTEFACTGROUP_HPP__

#include <cstddef>
#include <string_view>

namespace Game::Cmp::Altar
{

//! @brief The group an artefact belongs to, from "offering_action.reward.group" in items.json. A graveyard altar takes
//! one artefact of each group; any item in a group fills that group's slot.
//! @details The value is the altar slot: an index into the particle_offsets of sprite.graveyard.altar.inactive, i.e. the
//! corner light lit when an artefact of that group is offered.
enum class ArtefactGroup {
  //! @brief The treasure of the dead.
  TREASURE,
  //! @brief The likeness (face) of the dead.
  LIKENESS,
  //! @brief The name of the dead.
  NAME,
  //! @brief The words of the dead.
  WORDS
};

//! @brief Number of artefact groups, i.e. the number of corner lights on a graveyard altar.
inline constexpr std::size_t kArtefactGroupCount = 4;

//! @brief Human readable group name, for logs.
[[nodiscard]] constexpr std::string_view to_string( ArtefactGroup group )
{
  switch ( group )
  {
    case ArtefactGroup::TREASURE:
      return "Treasure";
    case ArtefactGroup::LIKENESS:
      return "Likeness";
    case ArtefactGroup::NAME:
      return "Name";
    case ArtefactGroup::WORDS:
      return "Words";
  }
  return "Unknown";
}

} // namespace Game::Cmp::Altar

#endif // SRC_COMPONENTS_ALTAR_ARTEFACTGROUP_HPP__
