#ifndef SRC_SYSTEMS_THREATS_LIGHTNINGSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_LIGHTNINGSYSTEM_HPP__

#include <Components/FractalCurve.hpp>
#include <Components/UUID.hpp>
#include <Events/LightningEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <entt/entity/fwd.hpp>

#include <optional>

namespace Game::Cmp
{
class LightningStrike;
class Position;
} // namespace Game::Cmp

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

  //! @brief Sets `m_trigger_lightning` flag to true and stores the strike target.
  //! @param ev
  void on_lightning_event( Events::LightningEvent ev );

  //! @brief Called every frame by the Graveyard Scene
  void update();

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
  bool m_trigger_lightning{ false };

  //! @brief The lightning target: the player or a lightning rod world item
  entt::entity m_target_entt{ entt::null };

  //! @brief UUID of the smoke particles from the last player strike, so they follow the player. Empty if the last strike hit a lightning rod.
  std::optional<Cmp::UUID> m_player_smoke_uuid;

  //! @brief Damage the player and create the lightning strike and smoke at the player position
  void strike_player();

  //! @brief Create a lightning strike object
  //! @param target_pos
  void create_lightning_strike( const Cmp::Position &target_pos );

  //! @brief If the Cmp::LightningStrike timer has expired then delete the entity
  void delete_expired_lightning_strikes();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_LIGHTNINGSYSTEM_HPP__
