#ifndef SRC_SYSTEMS_THREATS_HAZARDTRAITS_HPP__
#define SRC_SYSTEMS_THREATS_HAZARDTRAITS_HPP__

#include <Components/Hazard/CorruptionCell.hpp>
#include <Components/Hazard/SinkholeCell.hpp>
#include <Components/Persistent/CorruptionSeed.hpp>
#include <Components/Persistent/SinkholeSeed.hpp>
#include <Components/Player/Mortality.hpp>
#include <Sprites/SpriteSheet.hpp>

namespace Game::Sys
{

//! @brief Traits describing how a HazardFieldSystem should behave for a given hazard cell component type. Specialized per hazard type.
//! @tparam T The hazard cell component type (e.g. Cmp::SinkholeCell, Cmp::CorruptionCell) this specialization describes.
template <typename T>
struct HazardTraits;

//! @brief Traits for Sinkhole hazard field types: kills instantly on contact and excludes corruption cells.
template <>
struct HazardTraits<Cmp::SinkholeCell>
{
  //! @brief Persisted RNG seed component used to pick this hazard's initial spawn position.
  using SeedType = Cmp::Persist::SinkholeSeed;
  //! @brief Hazard component type that must not already be present on a cell for it to become a sinkhole.
  using ExcludeHazard = Cmp::CorruptionCell;
  //! @brief Whether contact with this hazard kills the player/NPC outright rather than applying gradual damage.
  static constexpr bool kills_instantly = true;
  //! @brief Cmp::Player::Mortality::State to transition the player into on contact.
  static constexpr Cmp::Player::Mortality::State mortality_state = Cmp::Player::Mortality::State::FALLING;
  //! @brief AnimData sprite type identifier used when spawning a sinkhole cell.
  static constexpr std::string_view sprite_type = "sprite.graveyard.hazard.sinkhole";
  //! @brief Denominator of the 1-in-N chance used each spread tick to convert an adjacent cell into a new sinkhole cell.
  static constexpr int odds = 2;
  //! @brief Whether the field spreads into open floor as well as obstacles. Reserved positions (multiblocks, walls,
  //! exit, spawn area, world items) and loot/NPC containers are always off limits.
  static constexpr bool spreads_to_open_floor = true;
};

//! @brief Traits for Corruption hazard field types: gradually drains health on contact and excludes sinkhole cells.
template <>
struct HazardTraits<Cmp::CorruptionCell>
{
  //! @brief Persisted RNG seed component used to pick this hazard's initial spawn position.
  using SeedType = Cmp::Persist::CorruptionSeed;
  //! @brief Hazard component type that must not already be present on a cell for it to become corrupted.
  using ExcludeHazard = Cmp::SinkholeCell;
  //! @brief Whether contact with this hazard kills the player/NPC outright rather than applying gradual damage.
  static constexpr bool kills_instantly = false;
  //! @brief Cmp::Player::Mortality::State to transition the player into once health has been drained to zero.
  static constexpr Cmp::Player::Mortality::State mortality_state = Cmp::Player::Mortality::State::DECAYING;
  //! @brief AnimData sprite type identifier used when spawning a corruption cell.
  static constexpr std::string_view sprite_type = "sprite.graveyard.hazard.corruption";
  //! @brief Denominator of the 1-in-N chance used each spread tick to convert an adjacent obstacle into a new corruption cell.
  static constexpr int odds = 7;
  //! @brief Whether the field spreads into open floor as well as obstacles. Corruption only consumes obstacles.
  static constexpr bool spreads_to_open_floor = false;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_HAZARDTRAITS_HPP__
