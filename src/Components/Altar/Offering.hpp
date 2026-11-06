#ifndef SRC_COMPONENTS_ALTAR_OFFERING_HPP__
#define SRC_COMPONENTS_ALTAR_OFFERING_HPP__

namespace Game::Cmp::Altar
{

//! @brief Marker component for identifying in-progress altar offering animations
struct Offering
{
  //! @brief Whether this offering animation is currently active/in-progress.
  bool active{ true };
};

} // namespace Game::Cmp::Altar

#endif // SRC_COMPONENTS_ALTAR_OFFERING_HPP__
