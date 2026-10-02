#ifndef SRC_UTILS_UTILS_HPP__
#define SRC_UTILS_UTILS_HPP__

#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Constants.hpp>

#include <cstddef>
#include <entt/entity/fwd.hpp>
#include <optional>
#include <string>

// Forward declarations
namespace sf
{
class RenderWindow;
class View;
} // namespace sf

namespace Game::Cmp
{
class Position;
class RectBounds;
class PlayerHealth;
class ZOrderValue;
class System;
class LerpPosition;
class CurrentScene;
} // namespace Game::Cmp

namespace Game::Cmp::Player
{
class Wealth;
class BlastRadius;
class Mortality;
class LastGraveyardPosition;
class RuinLocation;
} // namespace Game::Cmp::Player

namespace Game::Utils
{

//! @brief How a coordinate should be rounded when snapping to the grid.
enum class Rounding {
  //! @brief Truncate towards zero (std::trunc).
  TOWARDS_ZERO,
  //! @brief Round away from zero (ceil for positive values, floor for negative values).
  AWAY_ZERO,
  //! @brief Round to the nearest grid line (std::round).
  NEAREST
};

// Pure utility functions - no registry needed

//! @brief Check whether a rectangle at `position` with size `size` lies fully within a map of `map_grid_size` grid cells.
//! @param position The top-left position of the rectangle to test, in pixels.
//! @param size The size of the rectangle to test, in pixels.
//! @param map_grid_size The size of the map, in grid cells.
//! @return bool true if the rectangle is fully within [0, map_width) x [0, map_height).
bool is_in_bounds( const sf::Vector2f &position, const sf::Vector2f &size, const sf::Vector2u &map_grid_size );

//! @brief Snap a rectangle's position to the nearest grid cell, preserving its size.
//! @param position The rectangle to snap. Only its position is considered; the size is unchanged.
//! @param rounding How to round the position to the grid.
//! @return sf::FloatRect A new rectangle with the snapped position and the original size.
sf::FloatRect snap_to_grid( const sf::FloatRect &position, Rounding rounding = Rounding::NEAREST ) noexcept;

//! @brief Snap a 2D position to the nearest grid cell.
//! @param position The position to snap.
//! @param rounding How to round the position to the grid.
//! @return sf::Vector2f The snapped position.
sf::Vector2f snap_to_grid( const sf::Vector2f &position, Rounding rounding = Rounding::NEAREST ) noexcept;

//! @brief Snap a Cmp::Position's position to the nearest grid cell, preserving its size.
//! @param position The position component to snap. Only its position is considered; the size is unchanged.
//! @param rounding How to round the position to the grid.
//! @return sf::FloatRect A new rectangle with the snapped position and the original size.
sf::FloatRect snap_to_grid( const Cmp::Position &position, Rounding rounding ) noexcept;

//! @brief Get the mouse cursor's world-space bounds within the given game view.
//! @param window The render window the mouse position is read from.
//! @param gameview The view used to map the mouse's pixel position into world coordinates.
//! @return sf::FloatRect A small (2x2 px) bounds centered on the mouse's world position.
sf::FloatRect get_mouse_bounds_in_gameview( const sf::RenderWindow &window, const sf::View &gameview );

//! @brief Find the entity whose Cmp::Position intersects `match`.
//! @param reg reference to the entt registry
//! @param match The position to test for intersection.
//! @return entt::entity The first matching entity, or entt::null if none intersects.
entt::entity get_world_pos_entt( entt::registry &reg, Cmp::Position match );

//! @brief Word-wraps text to fit within max_chars_per_line, preserving existing newlines as hard breaks.
//! @note Approximate (assumes roughly-uniform character width) - fine for a fatal-error debug screen.
//! @param text The text to wrap.
//! @param max_chars_per_line The maximum number of characters allowed per line.
//! @return std::string The word-wrapped text.
std::string wrap_text_to_width( const std::string &text, std::size_t max_chars_per_line );

// Entity/registry utility functions - declarations only

//! @brief Get the pixel position of an entity's Cmp::Position component.
//! @param registry reference to the entt registry
//! @param entity The entity to get the pixel position for.
//! @return std::optional<sf::Vector2f> The entity's position, or std::nullopt if it has no Cmp::Position.
std::optional<sf::Vector2f> get_pixel_position( entt::registry &registry, entt::entity entity );

//! @brief Check whether the graveyard exit is currently locked.
//! @param reg reference to the entt registry
//! @return bool The Cmp::Exit component's locked state.
//! @throws std::runtime_error if no entity has a Cmp::Exit component.
bool is_graveyard_exit_locked( entt::registry &reg );

//! @brief Get the scene setting cmp object
//! @tparam SCENESETTING The scene setting component type to retrieve.
//! @param reg reference to the entt registry
//! @return SCENESETTING& Reference to the singleton scene setting component.
//! @throws std::runtime_error if no entity has a SCENESETTING component.
template <typename SCENESETTING>
SCENESETTING &scene_setting( entt::registry &reg )
{
  entt::entity scene_entt = entt::null;
  SCENESETTING *scene_setting_cmp = nullptr;
  auto scene_setting_view = reg.view<SCENESETTING>();
  if ( scene_setting_view->empty() ) { throw std::runtime_error( "Unable to get scene setting component!" ); }
  scene_entt = scene_setting_view.front();
  if ( scene_entt == entt::null ) { throw std::runtime_error( "Unable to get entity for scene setting component!" ); }
  scene_setting_cmp = reg.try_get<SCENESETTING>( scene_entt );
  return *scene_setting_cmp;
}

//! @brief Get the Grid Position object
//! @tparam Signedness The signedness of the grid position coordinates (int or unsigned int).
//! @param registry reference to the entt registry
//! @param entity The entity to get the grid position for.
//! @return std::optional<sf::Vector2<Signedness>> The entity's position converted to grid coordinates,
//! or std::nullopt if it has no Cmp::Position.
template <typename Signedness>
std::optional<sf::Vector2<Signedness>> get_grid_position( entt::registry &registry, entt::entity entity );

extern template std::optional<sf::Vector2<int>> get_grid_position( entt::registry &, entt::entity );
extern template std::optional<sf::Vector2<unsigned int>> get_grid_position( entt::registry &, entt::entity );

} // namespace Game::Utils

#endif // SRC_UTILS_UTILS_HPP__
