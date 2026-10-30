#ifndef SRC_SYSTEMS_TELEPORTSYSTEM_HPP__
#define SRC_SYSTEMS_TELEPORTSYSTEM_HPP__

#include <Components/Position.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <entt/entity/fwd.hpp>

// clang-format off
namespace Game::Events { class TeleportEntityEvent; }
// clang-format on

namespace Game::Sys
{

//! @brief Moves actors (player or NPCs) to a new position on Events::TeleportEntityEvent, playing an optional
//! one-shot animation at the departure and arrival positions. Actors with a Cmp::PendingTeleport fade out first;
//! a delayed event is held in that component until the fade completes. A player teleport with an animation also holds the
//! camera on the departure position (Cmp::Player::CameraHold) until that animation has finished.
class TeleportSystem : public BaseSystem
{
public:
  //! @brief Construct a new Teleport System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  TeleportSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Fade out actors with a Cmp::PendingTeleport and perform any delayed teleports that are due, remove any
  //! finished teleport animations, and release the camera once its Cmp::Player::CameraHold runs out.
  //! @param dt
  void update( sf::Time dt );

  //! @brief Teleport the event's entity. Forwards the move to Events::UpdatePlayerPositionEvent or
  //! Events::UpdateNpcPositionEvent, so the navmeshes stay in sync. A non-zero delay instead attaches a
  //! Cmp::PendingTeleport, and the move happens in update() once the entity has faded out.
  //! @param event
  void on_teleport_entity_event( const Game::Events::TeleportEntityEvent &event );

private:
  //! @brief Create a one-shot animation entity centred on `pos_cmp`, tagged with Cmp::TeleportAnim.
  //! @param pos_cmp The teleporting entity's position.
  //! @param anim The sprite to play.
  //! @return entt::entity
  entt::entity create_teleport_anim( const Cmp::Position &pos_cmp, const Sys::SpriteKey &anim );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_TELEPORTSYSTEM_HPP__
