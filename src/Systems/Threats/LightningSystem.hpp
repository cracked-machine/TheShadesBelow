#ifndef SRC_SYSTEMS_THREATS_LIGHTNINGSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_LIGHTNINGSYSTEM_HPP__

#include <Components/FractalCurve.hpp>
#include <Components/UUID.hpp>
#include <Events/LightningEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <entt/entity/fwd.hpp>

#include <SFML/System/Time.hpp>

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
  //! @param ev If `ev.target_entt` is entt::null, the target is picked by find_lightning_target().
  void on_lightning_event( Events::LightningEvent ev );

  //! @brief Called every frame by the Graveyard Scene
  //! @param dt Time elapsed since the last frame. Advances the worn out lightning rod's break timer.
  void update( sf::Time dt );

  //! @brief Checks if their is existing Cmp::LightningStrike entity in the entt registry
  //! @return true
  //! @return false
  bool lightning_strike_exists();

  //! @brief Used to create the lightning strike "zig-zag" sections. Called N times by LightningSystem::create_lightning_strike().
  //! @note It takes each existing segment and divides into two segments. The dividing vertex point is then offset at perpendicular distance
  //! @param ls_seq
  //! @param deviations
  //! @param lines_per_strike
  static void divide_lightning_segments( std::vector<std::vector<sf::Vertex>> &ls_seq, Cmp::LightningStrike::AngleDeviations deviations,
                                         int lines_per_strike );

private:
  //! @brief Pick the strike target: the first working lightning rod in view, otherwise the player.
  //! @return entt::entity The lightning rod world item or the player entity.
  entt::entity find_lightning_target();

  //! @brief Set by on_lightning_event(); consumed by update() to trigger creation of a new lightning strike.
  bool m_trigger_lightning{ false };

  //! @brief The lightning target: the player or a lightning rod world item
  entt::entity m_target_entt{ entt::null };

  //! @brief UUID of the smoke particles from the last player strike, so they follow the player. Empty if the last strike hit a lightning rod.
  std::optional<Cmp::UUID> m_player_smoke_uuid;

  //! @brief Lightning rod that has been worn out and is playing its glowing animation before it breaks. entt::null if none.
  entt::entity m_breaking_rod_entt{ entt::null };

  //! @brief Time since m_breaking_rod_entt was struck; the rod breaks when this reaches kRodBreakDelay.
  sf::Time m_rod_break_elapsed{ sf::Time::Zero };

  //! @brief How long a worn out lightning rod glows before breaking (4 frames at 1FPS).
  static constexpr sf::Time kRodBreakDelay = sf::seconds( 4.f );

  //! @brief Break a worn out lightning rod: spawn the copper debris, play the sound and swap it for "item.lightningrod.broken".
  //! @note Does nothing if `rod_entt` is no longer a lightning rod world item, e.g. it was picked up while glowing.
  //! @param rod_entt The lightning rod world item entity.
  void break_lightning_rod( entt::entity rod_entt );

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
