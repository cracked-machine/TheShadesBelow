#ifndef SRC_COMPONENTS_INVENTORY_INVENTORYDRAG_HPP__
#define SRC_COMPONENTS_INVENTORY_INVENTORYDRAG_HPP__

#include <Components/Inventory/PlayerInventorySlot.hpp>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <array>
#include <cstdint>
#include <optional>

namespace Game::Cmp
{

//! @brief Registry context variable (`reg.ctx()`) shared by Sys::HudOverlaySystem and Sys::SceneInputRouter for
//! dragging items between inventory slots. The HUD records where it drew each slot; the input router hit-tests the
//! mouse against them and records the slot being dragged, which the HUD draws under the cursor.
//! Living in the scene's registry, a drag never carries over to another scene.
struct InventoryDrag
{
  //! @brief Screen rect of each inventory slot from the last HUD frame, indexed by slot; unset for slots not drawn.
  std::array<std::optional<sf::FloatRect>, kMaxInventorySlots> m_slot_rects{};

  //! @brief The slot whose item is being dragged, or std::nullopt if no drag is in progress.
  std::optional<uint8_t> m_from_slot;

  //! @brief The slot under a screen position, if any.
  //! @param pos Screen (window pixel) position, e.g. the mouse position.
  //! @return std::optional<uint8_t> The slot index, or std::nullopt if pos is over no slot.
  [[nodiscard]] std::optional<uint8_t> slot_at( sf::Vector2f pos ) const
  {
    for ( uint8_t idx = 0; idx < m_slot_rects.size(); ++idx )
    {
      if ( m_slot_rects.at( idx ) and m_slot_rects.at( idx )->contains( pos ) ) return idx;
    }
    return std::nullopt;
  }
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_INVENTORY_INVENTORYDRAG_HPP__
