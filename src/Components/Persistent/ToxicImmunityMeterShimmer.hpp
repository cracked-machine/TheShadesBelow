#ifndef SRC_COMPONENTS_PERSISTENT_TOXICIMMUNITYMETERSHIMMER_HPP__
#define SRC_COMPONENTS_PERSISTENT_TOXICIMMUNITYMETERSHIMMER_HPP__

#include <Components/Persistent/BasePersistentColour.hpp>

namespace Game::Cmp::Persist
{

//! @brief Highlight colour of the frost crystals and glints on the toxidrome UI meters (also used by the unused shimmer sweep
//! helper) while Cmp::Player::ToxicImmunity is active.
class ToxicImmunityMeterShimmer : public BasePersistentColour
{
public:
  //! @brief Construct with the default sf::Color (transparent black), set from res/json/persist.json.
  ToxicImmunityMeterShimmer()
      : BasePersistentColour( "ToxicImmunityMeterShimmer" )
  {
  }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_TOXICIMMUNITYMETERSHIMMER_HPP__
