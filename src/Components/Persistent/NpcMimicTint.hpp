#ifndef SRC_COMPONENTS_PERSISTENT_NPCMIMICTINT_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCMIMICTINT_HPP__

#include <Components/Persistent/BasePersistentColour.hpp>

namespace Game::Cmp::Persist
{

//! @brief Tint multiplied with the Mimic NPC sprite.
class NpcMimicTint : public BasePersistentColour
{
public:
  //! @brief Construct with the default sf::Color (transparent black), set from res/json/persist.json.
  NpcMimicTint()
      : BasePersistentColour( "NpcMimicTint" )
  {
  }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCMIMICTINT_HPP__
