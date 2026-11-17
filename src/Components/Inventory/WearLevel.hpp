#ifndef SRC_COMPONENTS_INVENTORY_WEARLEVEL_HPP__
#define SRC_COMPONENTS_INVENTORY_WEARLEVEL_HPP__

namespace Game::Cmp::Inventory
{

//! @brief Tracks the remaining condition of a wearable/usable item (e.g. a shovel or bow) as a
//! percentage, rendered as a wear-level overlay above the item.
class WearLevel
{
public:
  //! @brief Construct a new Wear Level object.
  //! @param level Initial wear level, as a percentage (0-100).
  WearLevel( float level )
      : m_level( level )
  {
  }

  //! @brief The item's remaining condition, as a percentage (0-100).
  float m_level;
};

} // namespace Game::Cmp::Inventory

#endif // SRC_COMPONENTS_INVENTORY_WEARLEVEL_HPP__
