#ifndef SRC_COMPONENTS_PERSISTENT_GRAVETRAILGUIDECOLOUR_HPP__
#define SRC_COMPONENTS_PERSISTENT_GRAVETRAILGUIDECOLOUR_HPP__

#include <Components/Persistent/BasePersistentColour.hpp>

namespace Game::Cmp::Persist
{

//! @brief Colour of the chevrons guiding the player from the activated altar to the grave trail starts. See Sys::ProcGen::TrailGuideSystem.
class GraveTrailGuideColour : public BasePersistentColour
{
public:
  //! @brief Construct with the default sf::Color (transparent black), set from res/json/persist.json.
  GraveTrailGuideColour()
      : BasePersistentColour( "GraveTrailGuideColour" )
  {
  }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_GRAVETRAILGUIDECOLOUR_HPP__
