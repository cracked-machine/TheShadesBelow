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

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/core/type_traits.hpp>
#include <entt/entity/fwd.hpp>
#include <vector>

// clang-format off
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
class IShaderSprite;
} // namespace Game::Sprites

namespace Game::Sprites::Containers { class VertexFloor; }
namespace Game::PathFinding { class SpatialHashGrid; }
namespace Game::Cmp 
{
class ZOrderValue;
class Position;
class Armed;
struct FractalCurve;
} // namespace Game::Cmp

namespace Game::Cmp::Particle { class IParticleSprite; }
// clang-format on
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
  //! Cmp::Position-bearing renderable entities - see queue_positioned(). Scenes that don't
  //! populate/pass one (nullptr, the default) fall
  //! back to the unindexed full-registry scan, so this is safe to omit.
  void render_game( sf::Time dt, RenderOverlaySystem &render_overlay_sys,
                    const PathFinding::SpatialHashGridSharedPtr &render_position_grid = nullptr );

  //! @brief Refreshes the Z-order rendering queue
  //! @param render_position_grid See render_game()'s parameter of the same name.
  void refresh_z_order_queue( const PathFinding::SpatialHashGridSharedPtr &render_position_grid = nullptr );

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
  void render_zorder_queue( RenderOverlaySystem &render_overlay_sys );

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
  void render_fractal_curve( const Cmp::FractalCurve &curve );

  //! @brief Used by GraveyardScene when player is struck by lightning
  void render_lightning_strike();

  //! @brief Renders the crack decal sequence on obstacles that have taken damage.
  void render_obstacle_cracks();

  //! @brief Flashes the screen
  //! @param color
  void render_screen_flash( sf::Color color );

  //! @brief Queues every visible multiblock root in the list. The whole multiblock rect is visibility-tested,
  //! preventing pop-in when only part of it is inside the view.
  template <typename... MultiBlock>
  void queue_multiblocks( sf::FloatRect view_bounds, entt::type_list<MultiBlock...> );

  //! @brief Queues every entity with CmpT, without a visibility test. For components with no world bounds.
  template <typename CmpT>
  void queue_all();

  //! @brief Queues world-space particle sprites whose bounds are in view, and all screen-space ones.
  void queue_particles( sf::FloatRect view_bounds );

  //! @brief Queues every visible Cmp::Position entity, other than multiblock roots.
  //! @param render_position_grid Spatial index of static entities, queried instead of scanning the registry;
  //! movers are then scanned separately. nullptr falls back to a full registry scan.
  void queue_positioned( sf::FloatRect view_bounds, const PathFinding::SpatialHashGridSharedPtr &render_position_grid );

  //! @brief Queues visible Cmp::Position entities that also have every component in Filter.
  //! An empty Filter scans every positioned entity.
  template <typename... Filter>
  void queue_positioned_view( sf::FloatRect view_bounds );

  //! @brief Appends an entity to the z-order queue
  void push( entt::entity entity, const Cmp::ZOrderValue &z_order_cmp );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief The z-order queue for rendering
  //! Each frame, this queue is refreshed to ensure correct rendering order
  std::vector<ZOrder> m_zorder_queue_;

  //! @brief Copy of the window contents taken when the z-order loop reaches a post-process shader, which is then
  //! drawn into that shader's render texture. Reused across passes and frames.
  sf::Texture m_frame_capture;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_RENDERGAMESYSTEM_HPP__
