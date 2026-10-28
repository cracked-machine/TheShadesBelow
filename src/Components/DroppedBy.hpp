#ifndef SRC_COMPONENTS_DROPPEDBY_HPP__
#define SRC_COMPONENTS_DROPPEDBY_HPP__

#include <Components/UUID.hpp>

namespace Game::Cmp
{

//! @brief Links loot to the world item whose action reward dropped it, so the loot can be removed along with its source.
struct DroppedBy
{
  //! @brief The Cmp::UUID of the dropping world item.
  Cmp::UUID m_source;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_DROPPEDBY_HPP__
