#ifndef SRC_COMPONENTS_PERSISTENT_TOXICIMMUNITYMETERTINT_HPP__
#define SRC_COMPONENTS_PERSISTENT_TOXICIMMUNITYMETERTINT_HPP__

#include <Components/Persistent/BasePersistentColour.hpp>

namespace Game::Cmp::Persist
{

//! @brief Base colour of the frost overlay drawn over the toxidrome UI meters (flat wash if shaders are unavailable) while
//! Cmp::Player::ToxicImmunity is active.
class ToxicImmunityMeterTint : public BasePersistentColour
{
public:
  //! @brief Construct with the default sf::Color (transparent black), set from res/json/persist.json.
  ToxicImmunityMeterTint()
      : BasePersistentColour( "ToxicImmunityMeterTint" )
  {
  }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_TOXICIMMUNITYMETERTINT_HPP__
