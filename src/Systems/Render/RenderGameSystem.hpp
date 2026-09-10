#ifndef SRC_SYSTEMS_RENDER_RENDERGAMESYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_RENDERGAMESYSTEM_HPP__

#include <Components/AnimData.hpp>
#include <Components/Inventory/ScryingBall.hpp>
#include <Components/NoRender.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>
#include <optional>
#include <queue>
#include <tuple>

namespace Game::Sprites
{
class SpriteSheet;
class SpriteFactory;
class FloodWaterShader;
class ViewFragmentShader;
class NightStaticShader;
class MistShader;
class DarkModeShader;
class DrippingBloodShader;
} // namespace Game::Sprites

namespace Game::Sprites::Containers
{
class VertexFloor;
} // namespace Game::Sprites::Containers

namespace Game::PathFinding
{
class SpatialHashGrid;
} // namespace Game::PathFinding

namespace Game::Cmp
{
class ZOrderValue;
class Position;
class Armed;
struct FractalCurve;
class HealingSpringBuildingMultiBlock;
struct Moveable;
struct ObstacleCap;
} // namespace Game::Cmp

namespace Game::Cmp::Player
{
class Character;
} // namespace Game::Cmp::Player

namespace Game::Cmp::Npc
{
class NPC;
} // namespace Game::Cmp::Npc

namespace Game::Cmp::Weapons::Projectiles
{
class Arrow;
} // namespace Game::Cmp::Weapons::Projectiles

namespace Game::Cmp::Altar
{
class MultiBlock;
} // namespace Game::Cmp::Altar

namespace Game::Cmp::Crypt
{
class BuildingMultiBlock;
class InteriorMultiBlock;
} // namespace Game::Cmp::Crypt

namespace Game::Cmp::Grave
{
class MultiBlock;
} // namespace Game::Cmp::Grave

namespace Game::Cmp::Ruin
{
class BuildingMultiBlock;
} // namespace Game::Cmp::Ruin

namespace Game::Cmp::Particle
{
class IParticleSprite;
}

namespace Game::Sys
{

class RenderOverlaySystem;

//! @brief Renders everything that exists in the game world: the z-ordered entity queue (sprites, particles, shaders, floor tiles),
//! player-adjacent effects (shockwaves, lightning, compass arrow), and the camera. Delegates all UI and debug overlay rendering to
//! RenderOverlaySystem.
class RenderGameSystem : public RenderSystem
{
public:
  //! @brief Construct a new Render Game System object
  RenderGameSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief Destroy the Render Game System object
  ~RenderGameSystem();

  //! @brief Entrypoint for rendering the game
  //! @param deltaTime
  //! @param render_overlay_sys anything that is not part of the game world itself. i.e. UI, debug
  //! info, etc..
  //! @param render_player_sys anything that walks about in the game world, i.e. player, NPCs, etc..
  //! as well as death animations/effects
  //! @param render_position_grid Optional spatial index of static (never moved after creation)
  //! Cmp::Position-bearing renderable entities - see add_visible_entity_to_z_order_queue()'s
  //! Cmp::Position specialization. Scenes that don't populate/pass one (nullptr, the default) fall
  //! back to the unindexed full-registry scan, so this is safe to omit.
  void render_game( sf::Time dt, RenderOverlaySystem &render_overlay_sys, PathFinding::SpatialHashGridSharedPtr reserved_grid,
                    PathFinding::SpatialHashGridSharedPtr render_position_grid = nullptr );

  //! @brief Refreshes the Z-order rendering queue
  //! @param render_position_grid See render_game()'s parameter of the same name.
  void refresh_z_order_queue( PathFinding::SpatialHashGridSharedPtr render_position_grid = nullptr );

  //! @brief This should be called by scene "on enter" functions.
  void init_world_view();

  //! @brief Update the camera center on the player using lerp for smooth transform.
  //! @param deltaTime
  void update_camera( sf::Time deltaTime );

private:
  //! @brief Smoothed camera position, lerped towards the player's position each frame.
  sf::Vector2f m_camera_position{ 0.f, 0.f };

  //! @brief Whether `m_camera_position` has been seeded with the player's position yet, to avoid lerping from the origin on the first frame.
  bool m_camera_initialized{ false };

  //! @brief Draws every entity in `m_zorder_queue_` (sprites, particles, shaders, floor tiles), lowest z-order first
  //! @param render_overlay_sys anything that is not part of the game world itself. i.e. UI, debug info, etc..
  //! @param distortion_active Whether this frame's drawing has been redirected into the FearDistortion shader's render
  //! texture; passed through so the post-process shader entry point in the queue knows whether to composite it back
  void render_zorder_queue( RenderOverlaySystem &render_overlay_sys, bool distortion_active );

  //! @brief Used by CryptScene for Priest NPC weapon
  //! @param floormap
  void render_shockwaves();

  //! @brief Used by GraveyardScene when player has key and relic carryitems
  void render_arrow_compass();

  //! @brief Used by GraveyardScene when player places a seeing stone
  void render_seeingstone_doglegs( const Cmp::SeeingStone &stone_cmp, const Cmp::Position &pos_cmp );

  //! @brief Renders the flashing warning square over an armed obstacle in the game world
  void render_armed_indicator( const Cmp::Armed &armed_cmp, const Cmp::Position &pos_cmp );

  //! @brief Draws the zigzag vertex sequence shared by lightning strikes and obstacle cracks: a thick
  //! main line through the zero-index vertex of each row, with thinner lines to/from the other vertices.
  //! @param curve The vertex sequence and lifetime state to draw
  //! @param main_color Colour of the zero-index (main) line
  //! @param aux_color Colour of the non-zero-index (branch) lines
  //! @param main_thickness Thickness of the main line
  //! @param aux_thickness Thickness of the branch lines
  void render_fractal_curve( const Cmp::FractalCurve &curve, sf::Color main_color, sf::Color aux_color, float main_thickness, float aux_thickness );

  //! @brief Used by GraveyardScene when player is struck by lightning
  void render_lightning_strike();

  //! @brief Renders the crack decal sequence on obstacles that have taken damage.
  void render_obstacle_cracks();

  //! @brief Flashes the screen
  //! @param color
  void render_screen_flash( sf::Color color );

  //! @brief Finds the nearest position among a view of entities to a source point, skipping entities the
  //! predicate rejects.
  //! @tparam View An entt view already filtered to the desired component types
  //! @tparam ToPositionFn Callable taking (entt::entity, const Components&...) matching `view`, returning
  //! std::optional<Cmp::Position> — std::nullopt skips that entity
  //! @param view The entt view to search
  //! @param from The point to measure distance from
  //! @param to_position Projects a matched entity's components to its Cmp::Position, or skips it
  //! @return The nearest matched Cmp::Position, or std::nullopt if no entity in the view qualified
  template <typename View, typename ToPositionFn>
  std::optional<Cmp::Position> find_nearest_target( View &&view, sf::Vector2f from, ToPositionFn &&to_position )
  {
    using DistanceQueue = std::priority_queue<std::pair<float, Cmp::Position>, std::vector<std::pair<float, Cmp::Position>>,
                                              Utils::Maths::DistancePositionComparator>;
    DistanceQueue distance_queue;
    for ( auto &&row : view.each() )
    {
      std::optional<Cmp::Position> maybe_pos = std::apply( to_position, row );
      if ( not maybe_pos ) continue;
      auto float_distance = Utils::Maths::getEuclideanDistance( maybe_pos->position, from );
      distance_queue.emplace( float_distance, *maybe_pos );
    }
    return distance_queue.empty() ? std::nullopt : std::optional<Cmp::Position>{ distance_queue.top().second };
  }

  //! @brief Adds visible entities of a specific component type to the Z-order queue
  //! Optimized (single-type view) query on entt components for visibility check and Z-order queue
  //! addition
  //! @note The Component-owning entity must also have a Cmp::ZOrderValue component to be added to
  //! the queue
  //! @tparam Component The component type to check for visibility
  //! @param zorder_queue The Z-order queue to add visible entities to
  //! @param view_bounds The view bounds to check against
  template <typename Component>
  void add_visible_entity_to_z_order_queue( std::vector<ZOrder> &zorder_queue, sf::FloatRect view_bounds )
  {
    // render_zorder_queue() only ever draws a Cmp::Position entity via its all_of<Position, AnimData>
    // branch, so anything lacking AnimData (multiblock segment tiles, per-grid-cell pathfinding
    // placeholder entities, etc.) can never actually be drawn no matter how it sorts here - it's pure
    // dead weight in the queue. Requiring AnimData and Cmp::ZOrderValue through the view itself (rather
    // than a try_get after the fact) also means EnTT drives iteration from whichever of these pools is
    // smallest, instead of visiting every Cmp::Position entity in the entire level - which vastly
    // outnumbers the ones that can ever be visible - just to discard most of them.
    //
    // The 6 multiblock root types below get their own dedicated add_visible_entity_to_z_order_queue
    // pass just above this one in refresh_z_order_queue() (matched on their own MultiBlock component,
    // not Cmp::Position) - but each root entity *also* carries a bare Cmp::Position + Cmp::AnimData,
    // so without this exclude they'd additionally match this pass and get queued (and drawn) a second
    // time at the same position.
    if constexpr ( std::is_same_v<Component, Cmp::Position> )
    {
      if ( m_render_position_grid_ )
      {
        // Fast path: m_render_position_grid_ indexes every *static* (never moved after creation)
        // Cmp::Position + Cmp::AnimData + Cmp::ZOrderValue entity - see
        // Factory::Pathfinding::create_render_position_grid() for the exact population/exclude list,
        // which mirrors the fallback branch below plus the mover exclusions handled separately just
        // after this block. query_rect() only visits buckets overlapping the camera, instead of every
        // such entity in the whole level.
        for ( auto entity : m_render_position_grid_->query_rect( view_bounds ) )
        {
          // reg().valid() guards against a stale bucket entry outliving its entity - e.g. a missed
          // remove() call at some destruction site, or an entity handle EnTT has since recycled for
          // something else entirely. try_get (rather than assuming Position/ZOrderValue are still
          // present) additionally guards against an entity that's still alive but had one of those
          // components stripped without a matching grid removal.
          if ( not reg().valid( entity ) ) continue;
          auto *component = reg().try_get<Component>( entity );
          auto *z_order_cmp = reg().try_get<Cmp::ZOrderValue>( entity );
          if ( not component or not z_order_cmp ) continue;
          if ( not Utils::is_visible_in_view( view_bounds, *component ) ) continue;
          zorder_queue.push_back( ZOrder{ z_order_cmp->getZOrder(), entity } );
        }

        // The grid above only ever indexes static entities (create_render_position_grid excludes all
        // of the mover tags below, and nothing ever inserts them into it either) - each mover
        // population is small (bounded by spawn caps/in-flight counts, not level size) and moves every
        // frame, so it's cheaper and far less risky to just linear-scan them fresh each time than to
        // keep a spatial index in sync with something that mutates constantly.
        add_mover_to_z_order_queue<Cmp::Player::Character>( zorder_queue, view_bounds );
        add_mover_to_z_order_queue<Cmp::Npc::NPC>( zorder_queue, view_bounds );
        add_mover_to_z_order_queue<Cmp::Weapons::Projectiles::Arrow>( zorder_queue, view_bounds );
        add_mover_to_z_order_queue<Cmp::Moveable>( zorder_queue, view_bounds );
        add_mover_to_z_order_queue<Cmp::ObstacleCap>( zorder_queue, view_bounds );
        return;
      }

      // No grid was supplied for the current scene (see render_game()'s render_position_grid
      // parameter) - fall back to the unindexed full scan.
      auto pos_zorder_view = reg().view<Component, Cmp::AnimData, Cmp::ZOrderValue>(
          entt::exclude<Cmp::NoRender, Cmp::Altar::MultiBlock, Cmp::Crypt::BuildingMultiBlock, Cmp::Grave::MultiBlock,
                        Cmp::HealingSpringBuildingMultiBlock, Cmp::Crypt::InteriorMultiBlock, Cmp::Ruin::BuildingMultiBlock> );
      for ( auto entity : pos_zorder_view )
      {
        auto [component, z_order_cmp] = pos_zorder_view.template get<Component, Cmp::ZOrderValue>( entity );
        if ( not Utils::is_visible_in_view( view_bounds, component ) ) continue;
        zorder_queue.push_back( ZOrder{ z_order_cmp.getZOrder(), entity } );
      }
    }
    else
    {
      for ( auto [entity, component] : reg().view<Component>( entt::exclude<Cmp::NoRender> ).each() )
      {
        if constexpr ( std::is_base_of_v<sf::FloatRect, Component> )
        {
          if ( not Utils::is_visible_in_view( view_bounds, component ) ) continue;
        }
        if constexpr ( std::is_same_v<Cmp::Particle::SpriteOwner, Component> )
        {
          if ( component.sprite && component.sprite->get_view_type() == Cmp::Particle::ViewType::WORLD &&
               not Utils::is_visible_in_view( view_bounds, component.sprite->get_bounds() ) )
            continue;
        }

        auto z_order_cmp = reg().try_get<Cmp::ZOrderValue>( entity );
        if ( z_order_cmp ) { zorder_queue.push_back( ZOrder{ z_order_cmp->getZOrder(), entity } ); }
      }
    }
  }

  //! @brief Linear-scans every visible entity tagged with MoverTag (in addition to Position, AnimData
  //! and ZOrderValue) and adds it to the Z-order queue. Used by add_visible_entity_to_z_order_queue()'s
  //! Cmp::Position fast path for the small, bounded-count categories (player, NPCs, arrows, moveable
  //! obstacles/caps) that are deliberately never inserted into m_render_position_grid_, since they move
  //! every frame and keeping a spatial index in sync with that is unnecessary risk for populations this
  //! small.
  //! @tparam MoverTag Marker/filter component identifying the mover category; its value isn't read.
  template <typename MoverTag>
  void add_mover_to_z_order_queue( std::vector<ZOrder> &zorder_queue, sf::FloatRect view_bounds )
  {
    auto mover_view = reg().view<Cmp::Position, Cmp::AnimData, Cmp::ZOrderValue, MoverTag>( entt::exclude<Cmp::NoRender> );
    for ( auto entity : mover_view )
    {
      auto [pos_cmp, z_order_cmp] = mover_view.template get<Cmp::Position, Cmp::ZOrderValue>( entity );
      if ( not Utils::is_visible_in_view( view_bounds, pos_cmp ) ) continue;
      zorder_queue.push_back( ZOrder{ z_order_cmp.getZOrder(), entity } );
    }
  }

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Time component of the sine wave for the compass arrow bouncing movement
  sf::Clock m_compass_osc_clock;

  //! @brief Frequency component of the sine wave for the compass arrow bouncing movement
  float m_compass_freq{ 4.0f };

  //! @brief Min range for the compass arrow dynamic resize
  float m_compass_min_scale{ 0.5f };

  //! @brief Max range for the compass arrow dynamic resize
  float m_compass_max_scale{ 1.5f };

  //! @brief The z-order queue for rendering
  //! Each frame, this queue is refreshed to ensure correct rendering order
  std::vector<ZOrder> m_zorder_queue_;

  //! @brief Set for the duration of refresh_z_order_queue() from its render_position_grid parameter;
  //! see add_visible_entity_to_z_order_queue()'s Cmp::Position specialization. nullptr if the current
  //! scene didn't supply one, in which case that specialization falls back to an unindexed full scan.
  PathFinding::SpatialHashGridSharedPtr m_render_position_grid_;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_RENDERGAMESYSTEM_HPP__
