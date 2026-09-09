#ifndef SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEM_HPP__
#define SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEM_HPP__

#include <Components/Hazard/CorruptionCell.hpp>
#include <Components/Hazard/FieldCell.hpp>
#include <Components/Hazard/SinkholeCell.hpp>
#include <Components/Persistent/CorruptionDamage.hpp>
#include <Components/Persistent/CorruptionSeed.hpp>
#include <Components/Persistent/SinkholeSeed.hpp>
#include <Components/Player/Mortality.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>
#include <Utils/Random.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <entt/entity/fwd.hpp>

//! @brief HazardFields are environmental dangers that spread throughout the maze,
//! posing threats to both the player and NPCs. Examples include sinkholes (composed of many
//! Cmp::SinkholeCell) that instantly kill any entity that falls into them, and corruption fields
//! (composed of many Cmp::CorruptionCell) that gradually drain health over time.

namespace Game::Sys
{

template <typename T>
struct HazardTraits;

//! @brief Component requirements for a hazard field cell type: must provide a HazardTraits<T> specialization defining its seed component,
//! mutually-exclusive hazard type, instant-kill flag, mortality state and sprite type.
template <typename T>
concept ValidHazard = requires {
  typename HazardTraits<T>::SeedType;
  typename HazardTraits<T>::ExcludeHazard;
  { HazardTraits<T>::kills_instantly } -> std::convertible_to<bool>;
  { HazardTraits<T>::mortality_state } -> std::convertible_to<Cmp::Player::Mortality::State>;
  { HazardTraits<T>::sprite_type } -> std::convertible_to<std::string_view>;
};

//! @brief A templated system that manages the creation and spread of hazard
//! fields in a procedural maze.
//! @tparam HazardType The type of hazard component that this system will manage.
//! Must satisfy the ValidHazard concept.
template <ValidHazard HazardType>
class HazardFieldSystem : public Sys::BaseSystem
{
  //! @brief Convenience alias for the HazardTraits<HazardType> specialization backing this instantiation.
  using Traits = HazardTraits<HazardType>;

public:
  //! @brief Construct a new Hazard Field System object
  //!
  //! @param reg Smart pointer to the entt registry
  //! @param window Reference to the SFML render window
  //! @param sprite_factory Reference to the sprite factory
  HazardFieldSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief init the weak pointer for the reserved-positions grid.
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm ) { m_reserved_sm = reserved_sm; }

  //! @brief Spread the hazard field, then check for player and NPC collisions with it.
  //! @return The position of any newly added hazard cell, or an empty vector if none was added.
  sf::Vector2f update( sf::Time dt );

  //! @brief Spawn the initial hazard cell for this hazard type at a random valid obstacle position, if one does not already exist.
  //! @return The position of the newly spawned hazard cell, or an empty vector if one already exists or no valid position was found.
  sf::Vector2f init_hazard_field();

  //! @brief event handlers for pausing system clocks
  void on_pause() override;
  //! @brief event handlers for resuming system clocks
  void on_resume() override;

private:
  //! @brief Update the hazard field by spreading it to adjacent positions.
  //! 1. Check if the update interval has elapsed using m_clock.
  //! 2. Get a view of all entities with the hazard field component and position.
  //! 3. For each hazard field entity:
  //!   a. Check adjacent positions for obstacles without hazard components.
  //!   b. Use a random chance (1 in 8) to convert an adjacent obstacle into a new hazard cell.
  //!   c. Count adjacent hazard fields; if surrounded by 2 or more, mark as inactive to stop
  //!   further spreading.
  sf::Vector2f update_hazard_field();

  //! @brief Check for player collision with hazard fields.
  //! If a collision is detected, apply damage or instant death based on hazard type.
  void check_player_hazard_field_collision();
  //! @brief Check for NPC collision with hazard fields.
  //! If a collision is detected, trigger NPC death event.
  void check_npc_hazard_field_collision();

  //! @brief Clock used to track time for hazard field updates.
  //!
  sf::Clock m_spread_update_clock;

  //! @brief Time period for updating hazard fields.
  //!
  sf::Time m_update_period{ sf::seconds( 5.0f ) }; // seconds between adding new hazard fields

  sf::Time m_dmg_timer{ sf::Time::Zero };

  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEM_HPP__
