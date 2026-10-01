#ifndef SRC_SYSTEMS_THREATS_LIGHTNINGSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_LIGHTNINGSYSTEM_HPP__

#include <Components/FractalCurve.hpp>
#include <Events/LightningEvent.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Cmp
{
class LightningStrike;
}

namespace Game::Sys
{

//! @brief Drives lightning strike events: builds the zig-zagging fractal vertex sequence for a
//! strike, ages and removes expired strikes, and notifies listeners via Events::LightningEvent.
class LightningSystem : public BaseSystem
{
public:
  //! @brief Construct a new Lightning System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  LightningSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Sets `trigger_lightning` flag to true;
  //! @param ev
  void on_lightning_event( Events::LightningEvent ev );

  //! @brief Called every frame by the Graveyard Scene
  //! @param dt
  void update( sf::Time dt );

  //! @brief Checks if their is existing Cmp::LightningStrike entity in the entt registry
  //! @return true
  //! @return false
  bool lightning_strike_exists();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {};
  //! @brief event handlers for resuming system clocks
  void on_resume() override {};

  //! @brief Used to create the lightning strike "zig-zag" sections. Called N times by LightningSystem::create_lightning_strike().
  //! @note It takes each existing segment and divides into two segments. The dividing vertex point is then offset at perpendicular distance
  //! @param ls_seq
  //! @param deviations
  //! @param lines_per_strike
  static void divide_lightning_segments( std::vector<std::vector<sf::Vertex>> &ls_seq, Cmp::LightningStrike::AngleDeviations deviations,
                                         int lines_per_strike );

private:
  //! @brief Set by on_lightning_event(); consumed by update() to trigger creation of a new lightning strike.
  bool trigger_lightning{ false };

  //! @brief Create a lightning strike object
  //! @param dt
  void create_lightning_strike( sf::Time dt );

  //! @brief If the Cmp::LightningStrike timer has expired then delete the entity
  void delete_expired_lightning_strikes();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_LIGHTNINGSYSTEM_HPP__
