#include <Components/Exit.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/LerpPosition.hpp>
#include <Components/Loot.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Player/BlastRadius.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/LastGraveyardPosition.hpp>
#include <Components/Player/RuinLocation.hpp>
#include <Components/Player/SpeedPenalty.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/ZOrderValue.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Npc.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>
#include <cmath>
#include <entt/entity/registry.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

namespace Game::Utils
{

bool is_in_bounds( const sf::Vector2f &position, const sf::Vector2f &size, const sf::Vector2u &map_grid_size )
{
  float map_width = static_cast<float>( map_grid_size.x ) * Constants::kGridSizePxF.x;
  float map_height = static_cast<float>( map_grid_size.y ) * Constants::kGridSizePxF.y;

  if ( position.x < 0.f || position.y < 0.f ) return false;
  if ( position.x + size.x > map_width ) return false;
  if ( position.y + size.y > map_height ) return false;

  return true;
}

//! @brief Snaps a rectangle's position to the nearest grid cell.
//!
//! Computes a new rectangle whose top-left corner is moved to the nearest grid
//! intersection based on BaseSystem::kGridSquareSizePixels.x (the function
//! assumes a square grid and uses the x component as the grid cell size).
//! The input rectangle's size is preserved.
//!
//! Rounding uses std::round, so coordinates are mapped to the nearest multiple
//! of the grid size (negative coordinates are handled correctly by std::round).
//!
//! @param position The rectangle to be snapped. Only its position is considered;
//!                 the rectangle's size remains unchanged.
//! @return A new sf::FloatRect with the snapped position and the original size.
//!
//! @note If the grid is not square, only the x component of
//!       BaseSystem::kGridSquareSizePixels is used for both axes.
sf::FloatRect snap_to_grid( const sf::FloatRect &position, Rounding rounding ) noexcept
{
  float grid_size = Constants::kGridSizePx.x; // Assuming square grid
  sf::Vector2f snapped_pos;

  switch ( rounding )
  {
    case Rounding::TOWARDS_ZERO:
      snapped_pos = { std::trunc( position.position.x / grid_size ) * grid_size, std::trunc( position.position.y / grid_size ) * grid_size };
      break;
    case Rounding::AWAY_ZERO: {
      auto away_from_zero = []( float val, float grid )
      {
        float divided = val / grid;
        return ( val >= 0.f ? std::ceil( divided ) : std::floor( divided ) ) * grid;
      };
      snapped_pos = { away_from_zero( position.position.x, grid_size ), away_from_zero( position.position.y, grid_size ) };
      break;
    }
    case Rounding::NEAREST:
    default:
      snapped_pos = { std::round( position.position.x / grid_size ) * grid_size, std::round( position.position.y / grid_size ) * grid_size };
      break;
  }

  return sf::FloatRect( snapped_pos, position.size );
}

sf::FloatRect snap_to_grid( const Cmp::Position &position, Rounding rounding ) noexcept
{
  float grid_size = Constants::kGridSizePx.x; // Assuming square grid
  sf::Vector2f snapped_pos;

  switch ( rounding )
  {
    case Rounding::TOWARDS_ZERO:
      snapped_pos = { std::trunc( position.position.x / grid_size ) * grid_size, std::trunc( position.position.y / grid_size ) * grid_size };
      break;
    case Rounding::AWAY_ZERO: {
      auto away_from_zero = []( float val, float grid )
      {
        float divided = val / grid;
        return ( val >= 0.f ? std::ceil( divided ) : std::floor( divided ) ) * grid;
      };
      snapped_pos = { away_from_zero( position.position.x, grid_size ), away_from_zero( position.position.y, grid_size ) };
      break;
    }
    case Rounding::NEAREST:
    default:
      snapped_pos = { std::round( position.position.x / grid_size ) * grid_size, std::round( position.position.y / grid_size ) * grid_size };
      break;
  }

  return sf::FloatRect( snapped_pos, position.size );
}

//! @brief Snap a given position to the nearest grid square.
//! This function takes a 2D position and rounds its coordinates to the nearest
//! grid square based on the grid size defined in BaseSystem::kGridSquareSizePixels.
//! It assumes the grid squares are of uniform size.
//!
//! @param position The position to snap, as an sf::Vector2f.
//! @return sf::Vector2f The snapped position aligned to the grid.
sf::Vector2f snap_to_grid( const sf::Vector2f &position, Rounding rounding ) noexcept
{
  float grid_size = Constants::kGridSizePx.x; // Assuming square grid

  switch ( rounding )
  {
    case Rounding::TOWARDS_ZERO:
      return { std::trunc( position.x / grid_size ) * grid_size, std::trunc( position.y / grid_size ) * grid_size };
    case Rounding::AWAY_ZERO: {
      auto away_from_zero = []( float val, float grid )
      {
        float divided = val / grid;
        return ( val >= 0.f ? std::ceil( divided ) : std::floor( divided ) ) * grid;
      };
      return { away_from_zero( position.x, grid_size ), away_from_zero( position.y, grid_size ) };
    }
    case Rounding::NEAREST:
    default:
      return { std::round( position.x / grid_size ) * grid_size, std::round( position.y / grid_size ) * grid_size };
  }
}

//! @brief Get the Pixel Position object from an entity's Position component.
//!
//! @param entity The entity to get the pixel position for.
//! @return std::optional<sf::Vector2f>
std::optional<sf::Vector2f> get_pixel_position( entt::registry &registry, entt::entity entity )
{
  auto *pos = registry.try_get<Cmp::Position>( entity );
  if ( pos ) { return ( *pos ).position; }
  return std::nullopt;
}

bool is_graveyard_exit_locked( entt::registry &reg )
{
  auto exit_view = reg.view<Cmp::Exit>();
  for ( auto [exit_entt, exit_cmp] : exit_view.each() )
  {
    return exit_cmp.m_locked;
  }
  throw std::runtime_error( "No exit was found in the game!" );
}

sf::FloatRect get_mouse_bounds_in_gameview( const sf::RenderWindow &window, const sf::View &gameview )
{
  sf::Vector2i mouse_pixel_pos = sf::Mouse::getPosition( window );
  sf::Vector2f mouse_world_pos = window.mapPixelToCoords( mouse_pixel_pos, gameview );

  // Check if the mouse position intersects with the entity's grave multiblock sprite
  auto mouse_position_bounds = sf::FloatRect( mouse_world_pos, sf::Vector2f( 2.f, 2.f ) );

  return mouse_position_bounds;
}

//! @brief Get the Grid Position object
//! @param entity The entity to get the grid position for.
//! @tparam Signedness The signedness of the grid position coordinates (int or unsigned int).
//! @return std::optional<sf::Vector2<Signedness>>
template <typename Signedness>

std::optional<sf::Vector2<Signedness>> get_grid_position( entt::registry &registry, entt::entity entity )
{
  auto *pos = registry.try_get<Cmp::Position>( entity );
  if ( pos )
  {
    return std::optional<sf::Vector2<Signedness>>{ { static_cast<Signedness>( pos->position.x / Constants::kGridSizePx.x ),
                                                     static_cast<Signedness>( pos->position.y / Constants::kGridSizePx.y ) } };
  }
  SPDLOG_ERROR( "Entity {} does not have a Position component", static_cast<int>( entity ) );
  return std::nullopt;
}

template std::optional<sf::Vector2<int>> get_grid_position( entt::registry &, entt::entity );
template std::optional<sf::Vector2<unsigned int>> get_grid_position( entt::registry &, entt::entity );

entt::entity get_world_pos_entt( entt::registry &reg, Cmp::Position match )
{
  for ( auto [world_pos_entt, world_pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    if ( world_pos_cmp.findIntersection( match ) ) return world_pos_entt;
  }
  return entt::null;
}

bool is_plant_depleted( entt::registry &reg, entt::entity plant_entt )
{
  const auto *wearlevel_cmp = reg.try_get<Cmp::Inventory::WearLevel>( plant_entt );
  return wearlevel_cmp and wearlevel_cmp->m_level <= 0;
}

std::string wrap_text_to_width( const std::string &text, std::size_t max_chars_per_line )
{
  std::string result;
  std::istringstream lines( text );
  std::string line;
  bool first_line = true;
  while ( std::getline( lines, line ) )
  {
    if ( not first_line ) result += '\n';
    first_line = false;

    std::size_t line_len = 0;
    std::istringstream words( line );
    std::string word;
    bool first_word = true;
    while ( words >> word )
    {
      if ( not first_word and line_len + 1 + word.size() > max_chars_per_line )
      {
        result += '\n';
        line_len = 0;
        first_word = true;
      }
      if ( not first_word )
      {
        result += ' ';
        ++line_len;
      }
      result += word;
      line_len += word.size();
      first_word = false;
    }
  }
  return result;
}

bool is_current_scene( entt::registry &reg, Cmp::SceneSettings::SceneId scene_id )
{
  for ( auto [scene_entt, scene_cmp] : reg.view<Cmp::SceneSettings::CurrentScene>().each() )
  {
    return scene_cmp.id == scene_id;
  }
  return false;
}

std::optional<sf::Vector2f> find_random_cell_near( const sf::FloatRect &source, int max_rings, const std::function<bool( const sf::Vector2f & )> &is_free )
{
  const auto grid = Constants::kGridSizePxF;
  const sf::FloatRect source_cells = snap_to_grid( source );

  for ( int ring = 1; ring <= max_rings; ++ring )
  {
    const sf::FloatRect outer = Cmp::RectBounds::expanded( source_cells, ring ).getBounds();
    const sf::FloatRect inner = Cmp::RectBounds::expanded( source_cells, ring - 1 ).getBounds();

    // shuffled so the first free cell found is a random pick from this ring's free cells
    std::vector<sf::Vector2f> ring_cells;
    for ( float y = outer.position.y; y < outer.position.y + outer.size.y; y += grid.y )
    {
      for ( float x = outer.position.x; x < outer.position.x + outer.size.x; x += grid.x )
      {
        const sf::Vector2f cell( x, y );
        if ( not inner.contains( cell + grid / 2.f ) ) ring_cells.push_back( cell );
      }
    }
    Cmp::RandomInt rng( 0, 1 );
    std::shuffle( ring_cells.begin(), ring_cells.end(), rng );

    auto free_cell = std::ranges::find_if( ring_cells, is_free );
    if ( free_cell != ring_cells.end() ) return *free_cell;
  }
  return std::nullopt;
}

bool is_free_spawn_area( entt::registry &reg, const sf::FloatRect &area, const SpawnGrids &grids )
{
  if ( not grids.navmesh ) return false;

  // shrink the hitbox slightly so neighbouring cells sharing an edge don't count as overlapping
  const auto area_bounds = Cmp::RectBounds::scaled( area, 0.9f );

  // every cell under the area must be a walkable (non Cmp::Npc::NoPathFinding), unreserved world tile, free of
  // world items/loot that were in place when the navmesh was built
  const auto grid = Constants::kGridSizePxF;
  for ( float y = area.position.y; y < area.position.y + area.size.y; y += grid.y )
  {
    for ( float x = area.position.x; x < area.position.x + area.size.x; x += grid.x )
    {
      const Cmp::Position cell_pos( { x, y }, grid );
      const auto bucket = grids.navmesh->at( cell_pos );
      if ( bucket.empty() ) return false;
      if ( std::ranges::any_of( bucket, [&]( entt::entity e ) { return reg.valid( e ) and reg.any_of<Cmp::WorldItem, Cmp::Loot>( e ); } ) )
        return false;
      if ( grids.reserved_sm and not grids.reserved_sm->at( cell_pos ).empty() ) return false;
    }
  }

  if ( area_bounds.findIntersection( Player::get_position( reg ) ) ) return false;

  // NPCs are bucketed by their top-left, so one mid-lerp from the cell up/left of the area can still overlap it
  const sf::FloatRect npc_query( area.position - grid, area.size + grid );
  auto blocked_by_npc = [&]( const PathFinding::SpatialHashGrid *navmesh )
  {
    if ( not navmesh ) return false;
    for ( auto entt : navmesh->query_rect( npc_query ) )
    {
      if ( not reg.valid( entt ) or not reg.all_of<Cmp::Npc::NPC>( entt ) ) continue;
      const auto npc_bounds = Npc::collision_bounds( reg, entt );
      if ( npc_bounds and area_bounds.findIntersection( *npc_bounds ) ) return true;
    }
    return false;
  };
  if ( blocked_by_npc( grids.navmesh ) or blocked_by_npc( grids.ghost_navmesh ) ) return false;

  if ( grids.loot_sm )
  {
    for ( auto loot_entt : grids.loot_sm->query_rect( area ) )
    {
      if ( not reg.valid( loot_entt ) ) continue;
      const auto *loot_pos = reg.try_get<Cmp::Position>( loot_entt );
      if ( loot_pos and area_bounds.findIntersection( *loot_pos ) ) return false;
    }
  }
  return true;
}

} // namespace Game::Utils
