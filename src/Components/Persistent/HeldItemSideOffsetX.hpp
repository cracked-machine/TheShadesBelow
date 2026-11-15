#ifndef SRC_COMPONENTS_PERSISTENT_HELDITEMSIDEOFFSETX_HPP__
#define SRC_COMPONENTS_PERSISTENT_HELDITEMSIDEOFFSETX_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Extra horizontal offset (px) of the held inventory item while the player faces east or west, added on
//! top of Cmp::Persist::HeldItemOffset. Applied towards the facing side: +x facing east, -x facing west.
struct HeldItemSideOffsetX : public BasePersistent<float>
{
  //! @brief Construct with an initial offset and optional min/max bounds.
  //! @param value initial offset
  //! @param min_value minimum allowed offset
  //! @param max_value maximum allowed offset
  HeldItemSideOffsetX( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "HeldItemSideOffsetX"
  [[nodiscard]] std::string class_name() const override { return "HeldItemSideOffsetX"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_HELDITEMSIDEOFFSETX_HPP__
