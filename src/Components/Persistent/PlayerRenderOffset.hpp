#ifndef SRC_COMPONENTS_PERSISTENT_PLAYERRENDEROFFSET_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLAYERRENDEROFFSET_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Resting vertical render offset (px, negative is up) of the player sprite, applied via Cmp::AbsoluteRenderOffset.
//! Lifts the front-facing sprite so it looks centred on its grid cell. Visual only - the player position is unaffected.
struct PlayerRenderOffset : public BasePersistent<float>
{
  //! @brief Construct with an initial offset and optional min/max bounds.
  //! @param value initial offset
  //! @param min_value minimum allowed offset
  //! @param max_value maximum allowed offset
  PlayerRenderOffset( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "PlayerRenderOffset"
  virtual std::string class_name() const override { return "PlayerRenderOffset"; }
  //! @brief Get the human-readable detail/description text (currently unset).
  //! @return std::string empty detail string
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLAYERRENDEROFFSET_HPP__
