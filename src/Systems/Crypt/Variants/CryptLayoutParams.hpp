#ifndef SRC_SYSTEMS_CRYPT_VARIANTS_CRYPTLAYOUTPARAMS_HPP__
#define SRC_SYSTEMS_CRYPT_VARIANTS_CRYPTLAYOUTPARAMS_HPP__

namespace Game::Sys::Crypt
{

//! @brief Room generation tunables for a crypt variant. Consumed by Sys::CryptSystem::create_initial_closed_rooms().
struct LayoutParams
{
  //! @brief Number of rooms to place (fewer if max_attempts runs out first).
  int room_count{ 20 };
  //! @brief Minimum room width, in grid cells.
  int min_room_w{ 3 };
  //! @brief Maximum room width, in grid cells.
  int max_room_w{ 8 };
  //! @brief Minimum room height, in grid cells.
  int min_room_h{ 3 };
  //! @brief Maximum room height, in grid cells.
  int max_room_h{ 8 };
  //! @brief Minimum gap between rooms, in grid cells.
  int min_room_spacing{ 2 };
  //! @brief Placement attempts before giving up on reaching room_count.
  int max_attempts{ 5000 };
};

} // namespace Game::Sys::Crypt

#endif // SRC_SYSTEMS_CRYPT_VARIANTS_CRYPTLAYOUTPARAMS_HPP__
