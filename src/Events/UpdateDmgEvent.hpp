#ifndef SRC_EVENTS_UPDATEDMGEVENT_HPP__
#define SRC_EVENTS_UPDATEDMGEVENT_HPP__

namespace Game::Events
{

//! @brief Apply the wear level modification to inventory or world item.
//! @note Consumed by ItemSystem or InventorySystem. Defaults to subtract.
class UpdateDmgEvent
{
public:
  enum Type { ADD, SUBTRACT };

  UpdateDmgEvent( entt::entity entt, float amount, Type type = Type::SUBTRACT )
      : m_item_entt( entt ),
        m_amount( amount ),
        m_type( type )
  {
  }
  entt::entity m_item_entt;
  float m_amount{ 0 };
  Type m_type;
};

} // namespace Game::Events

#endif // SRC_EVENTS_UPDATEDMGEVENT_HPP__
