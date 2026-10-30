#ifndef SRC_COMPONENTS_PERSISTENT_REPELNPCPLAYERTINT_HPP__
#define SRC_COMPONENTS_PERSISTENT_REPELNPCPLAYERTINT_HPP__

#include <Components/Persistent/BasePersistentColour.hpp>

namespace Game::Cmp::Persist
{

//! @brief Tint multiplied with the player sprite while Cmp::Player::RepelNpc is active (blood oil stain).
class RepelNpcPlayerTint : public BasePersistentColour
{
public:
  //! @brief Construct with the default sf::Color (transparent black), set from res/json/persist.json.
  RepelNpcPlayerTint()
      : BasePersistentColour( "RepelNpcPlayerTint" )
  {
  }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_REPELNPCPLAYERTINT_HPP__
