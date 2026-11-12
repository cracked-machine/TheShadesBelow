#ifndef SRC_COMPONENTS_PERSISTENT_PLAYERCORNERASSISTTOLERANCE_HPP__
#define SRC_COMPONENTS_PERSISTENT_PLAYERCORNERASSISTTOLERANCE_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Max perpendicular misalignment (in pixels) the player can have when pushing into a 1-cell gap
//! and still be slid into alignment with it. 0 disables the assist. Values above half a grid cell are
//! meaningless because snap_to_grid (NEAREST) would then pick the neighbouring row/column.
struct PlayerCornerAssistTolerance : BasePersistent<float>
{
  //! @brief Construct a new PlayerCornerAssistTolerance object.
  //! @param value Initial tolerance value in pixels.
  //! @param min Minimum allowed tolerance value.
  //! @param max Maximum allowed tolerance value.
  PlayerCornerAssistTolerance( float value = 0.f, float min = 0.f, float max = 0.f )
      : BasePersistent( value, min, max )
  {
  }
  //! @brief Returns the class name used for identification and JSON serialization.
  //! @return std::string The class name.
  virtual std::string class_name() const override { return "PlayerCornerAssistTolerance"; }
  //! @brief Returns the display detail string for this setting (currently unused/empty).
  //! @return std::string The detail string.
  const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PLAYERCORNERASSISTTOLERANCE_HPP__
