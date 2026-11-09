#ifndef SRC_COMPONENTS_PERSISTENT_HELDITEMOFFSET_HPP__
#define SRC_COMPONENTS_PERSISTENT_HELDITEMOFFSET_HPP__

#include <Components/Persistent/BasePersistentVector2f.hpp>

namespace Game::Cmp::Persist
{

//! @brief Render offset (px; negative is left/up) of the held inventory item from the player's top-left.
class HeldItemOffset : public BasePersistentVector2f
{
public:
  //! @brief Construct with a zero offset, set from res/json/persist.json.
  HeldItemOffset()
      : BasePersistentVector2f( "HeldItemOffset" )
  {
  }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_HELDITEMOFFSET_HPP__
