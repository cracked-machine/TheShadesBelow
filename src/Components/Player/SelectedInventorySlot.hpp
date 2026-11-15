#ifndef SRC_COMPONENTS_PLAYER_SELECTEDINVENTORYSLOT_HPP__
#define SRC_COMPONENTS_PLAYER_SELECTEDINVENTORYSLOT_HPP__

#include <cstdint>

namespace Game::Cmp::Player
{

//! @brief The index of the player's selected ("in hand") inventory slot. Lives on the player entity, so it is
//! carried across scenes by Scene::RegistryTransfer::xfer_player_entt.
//! @note Read via Utils::Player::get_selected_slot_idx(), which clamps it to the current slot count.
struct SelectedInventorySlot
{
  //! @brief Index of the selected slot, matching Cmp::PlayerInventorySlot::m_slot_idx.
  uint8_t m_idx{ 0 };
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_SELECTEDINVENTORYSLOT_HPP__
