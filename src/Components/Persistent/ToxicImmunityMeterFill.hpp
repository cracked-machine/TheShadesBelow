#ifndef SRC_COMPONENTS_PERSISTENT_TOXICIMMUNITYMETERFILL_HPP__
#define SRC_COMPONENTS_PERSISTENT_TOXICIMMUNITYMETERFILL_HPP__

#include <Components/Persistent/BasePersistentColour.hpp>

namespace Game::Cmp::Persist
{

//! @brief Toxidrome UI meter fill colour while Cmp::Player::ToxicImmunity is active.
class ToxicImmunityMeterFill : public BasePersistentColour
{
public:
  //! @brief Construct with the default sf::Color (transparent black), set from res/json/persist.json.
  ToxicImmunityMeterFill()
      : BasePersistentColour( "ToxicImmunityMeterFill" )
  {
  }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_TOXICIMMUNITYMETERFILL_HPP__
