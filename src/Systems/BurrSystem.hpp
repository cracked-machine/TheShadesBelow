#ifndef SRC_SYS_BURRSYSTEM_HPP__
#define SRC_SYS_BURRSYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>

// clang-format off
namespace Game::Events { class PlayerActionEvent; struct AddInventoryEvent; struct ItemPickedUpEvent; }
// clang-format on

namespace Game::Sys
{

//! @brief Handles charging/throwing item.burr, burr flight, and burrs sticking to NPCs or obstacles; a missed throw drops the burr as a world item.
//! @note The resulting slowdown is applied by Utils::get_burr_speed_multiplier().
class BurrSystem : public BaseSystem
{
public:
  //! @brief Construct a new Burr System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  BurrSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Burr throw actions
  //! @param ev
  void on_player_action_event( Game::Events::PlayerActionEvent ev );

  //! @brief Start the throw cooldown when a burr is added to the inventory, e.g. foraged or granted as a reward
  //! @param ev
  void on_add_inventory_event( const Events::AddInventoryEvent &ev );

  //! @brief Start the throw cooldown when a burr world item is picked up
  //! @param ev
  void on_item_picked_up_event( const Events::ItemPickedUpEvent &ev );

  //! @brief Update the system. Should be called by Scene
  //! @param dt
  void update( sf::Time dt );

private:
  //! @brief Tracks how long the throw has been charged, between DRAW_BOW and RELEASE_BOW.
  sf::Clock m_throw_charge_clock;

  //! @brief True while the throw is being charged (left mouse button held with item.burr equipped).
  bool m_throw_charging{ false };

  //! @brief Time since a burr entered the inventory or was thrown; throws are ignored until Cmp::Persist::BurrThrowCooldown has passed.
  sf::Clock m_throw_cooldown_clock;

  //! @brief Update the burr position, spin and parabola arc
  //! @param dt
  void update_burr_trajectory( sf::Time dt );

  //! @brief Resolve in-flight burrs that hit an NPC, hit an obstacle, or reached their destination
  void check_burr_collisions();

  //! @brief Expire burrs stuck to NPCs/obstacles, and the burr sprites stuck to obstacle faces
  //! @param dt
  void expire_burrs( sf::Time dt );

  //! @brief If player is carrying a burr then throw it in direction of mouse position
  //! @param charge_fraction How far to the mouse position the burr should travel, in [0,1]
  void create_player_burr( float charge_fraction );
};

} // namespace Game::Sys

#endif // SRC_SYS_BURRSYSTEM_HPP__
