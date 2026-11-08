#ifndef SRC_COMPONENTS_HAZARD_SINKHOLECORNERS_HPP__
#define SRC_COMPONENTS_HAZARD_SINKHOLECORNERS_HPP__

#include <cstdint>

namespace Game::Cmp::Hazard
{

//! @brief Inner-corner overlays to draw on top of a sinkhole cell's edge sprite. A set bit means that
//! corner's two adjacent sides are open (sinkhole) but the diagonal cell between them is not.
struct SinkholeCorners
{
  //! @brief Bit flags for each corner of the cell.
  enum Corner : uint8_t { kNorthEast = 1u, kNorthWest = 2u, kSouthEast = 4u, kSouthWest = 8u };

  //! @brief Combination of Corner bits to draw.
  uint8_t mask{ 0 };
};

} // namespace Game::Cmp::Hazard

#endif // SRC_COMPONENTS_HAZARD_SINKHOLECORNERS_HPP__
