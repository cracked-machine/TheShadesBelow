#ifndef SRC_SYS_ARROWSYSTEM_HPP__
#define SRC_SYS_ARROWSYSTEM_HPP__

#include <Events/PlayerActionEvent.hpp>
#include <Systems/BaseSystem.hpp>

// clang-format off
namespace Game::Events { class PlayerActionEvent; }
// clang-format on

namespace Game::Sys
{

//! @brief Handles bow draw/release, arrow flight, and arrow-NPC collision.
class ArrowSystem : public BaseSystem
{
public:
  //! @brief Construct a new Arrow System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  ArrowSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Arrow actions
  //! @param ev
  void on_player_action_event( Game::Events::PlayerActionEvent ev );

  //! @brief Update the system. Should be called by Scene
  //! @param dt
  void update( sf::Time dt );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Tracks how long the bow has been held drawn, between DRAW_BOW and RELEASE_BOW.
  sf::Clock m_bow_draw_clock;

  //! @brief True while the bow is being drawn (left mouse button held with bow equipped).
  bool m_bow_drawing{ false };

  //! @brief Update the arrow position, angle and parabola arc
  //! @param dt
  void update_arrow_trajectory( sf::Time dt );

  //! @brief Check if NPC is hit by arrow
  void check_npc_arrow_collision();

  //! @brief If player is carrying bow then fire arrow in direction of mouse position
  //! @param charge_fraction How far to the mouse position the arrow should travel, in [0,1]
  void create_player_arrow( float charge_fraction );
};

} // namespace Game::Sys

#endif // SRC_SYS_ARROWSYSTEM_HPP__
