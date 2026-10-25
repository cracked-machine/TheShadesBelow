#ifndef SRC_EVENTS_LIGHTNINGEVENT_HPP__
#define SRC_EVENTS_LIGHTNINGEVENT_HPP__

#include <entt/entity/entity.hpp>

namespace Game::Events
{

//! @brief Triggers a lightning strike on `target_entt` (the player or a lightning rod), e.g. via the debug F12 key, causing the lightning
//! system to fire on its next update.
struct LightningEvent
{
  //! @brief The strike target. If entt::null, LightningSystem picks a working lightning rod in view, otherwise the player.
  entt::entity target_entt{ entt::null };
};

} // namespace Game::Events

#endif // SRC_EVENTS_LIGHTNINGEVENT_HPP__
