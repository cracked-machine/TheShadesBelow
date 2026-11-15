#ifndef SRC_COMPONENTS_PERSISTENT_AUTOPICKUPDROPS_HPP__
#define SRC_COMPONENTS_PERSISTENT_AUTOPICKUPDROPS_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Whether reward drops for the player (grave loot, altar reward items, forage and other item rewards) are picked
//! up straight into the first empty inventory slot, or left on the ground. A drop stays on the ground if every slot is
//! full. See Events::CreateItemEvent::m_auto_pickup and Sys::InventorySystem::on_pickup_world_item_event().
class AutoPickupDrops : public BasePersistent<bool>
{
public:
  //! @brief Construct the auto pickup setting.
  //! @param value true to pick reward drops up into an empty slot, false to leave them on the ground.
  AutoPickupDrops( bool value = false )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "AutoPickupDrops"; }
  //! @brief Returns the display description for this setting (unset; see persist.json).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_AUTOPICKUPDROPS_HPP__
