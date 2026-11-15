#ifndef SRC_SYSTEMS_RENDER_RENDERGAMESYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_RENDERGAMESYSTEM_HPP__

#include <Components/AnimData.hpp>
#include <Components/NoRender.hpp>
#include <Components/Player/ArrowCompass.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>

#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/core/type_traits.hpp>
#include <entt/entity/fwd.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

// clang-format off
namespace Game::Render
{
class UiLayout;
} // namespace Game::Render

namespace Game::Sprites
{
class SpriteSheet;
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
class BurrCount;
struct FractalCurve;
struct AnimData;
class WorldItem;
} // namespace Game::Cmp

namespace Game::Cmp::Hazard { struct SinkholeCorners; }

namespace Game::Cmp::Npc { struct Burryman; struct Crow; }
namespace Game::Cmp::Particle { class IParticleSprite; struct SpriteOwner; }
namespace Game::Cmp::Shader { struct SpriteOwner; }
// clang-format on
namespace Game::Sys
{

//! @brief Renders everything that exists in the game world: the z-ordered entity queue (sprites, particles, shaders, floor tiles),
//! player-adjacent effects (shockwaves, lightning, compass arrow), and the camera. UI and debug rendering are done
//! separately by the overlay systems, see OverlayArbitrationSystem.
class RenderGameSystem : public RenderSystem
{
public:
  //! @brief Construct a new Render Game System object
  RenderGameSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Destroy the Render Game System object
  ~RenderGameSystem();

  //! @brief Entrypoint for rendering the game world. Draws into the frame that SceneManager::update() clears beforehand
  //! and presents afterwards; OverlayArbitrationSystem::update draws the overlays on top of it.
  //! @param dt
  //! @param render_position_grid Optional spatial index of static (never moved after creation)
  //! Cmp::Position-bearing renderable entities - see ZOrderQueue::refresh(). Scenes that don't
  //! populate/pass one (nullptr, the default) fall
  //! back to the unindexed full-registry scan, so this is safe to omit.
  void render_game( sf::Time dt, const PathFinding::SpatialHashGridSharedPtr &render_position_grid = nullptr );

  //! @brief This should be called by scene "on enter" functions.
  void init_world_view();

private:
  //! @brief Draws every entity in `s_zorder_queue` (sprites, particles, shaders, floor tiles), lowest z-order first
  void render_zorder_queue();

  //! @brief Update the camera center on the player using lerp for smooth transform.
  //! @param deltaTime
  void update_camera( sf::Time dt );

  //! @brief Draws a Cmp::Position + Cmp::AnimData sprite, applying any Absolute* overrides, then its decorations
  //! (wear level, armed indicator)
  void draw_animated_sprite( entt::entity entity );

  //! @brief Lazily load res/shaders/Greyscale.frag (Cmp::Greyscale sprites) on first use.
  //! @return true if the shader is usable, false if sprites should be drawn in colour instead
  bool load_greyscale_shader();

  //! @brief Draws a shader sprite. Post-process shaders composite everything drawn before them in the queue;
  //! sprite-layer shaders composite a layer of just the player, NPC and world item sprites.
  void draw_shader_sprite( Cmp::Shader::SpriteOwner &shader_owner );

  //! @brief Redraws only the queued player, NPC and world item sprites into the shader's render texture, leaving
  //! the rest transparent. See IShaderSprite::is_sprite_layer().
  void capture_sprite_layer( Sprites::IShaderSprite &sprite );

  //! @brief Draws a world-space particle sprite. Screen-space particles are drawn by HudOverlaySystem and ShopOverlaySystem.
  void draw_particle_sprite( Cmp::Particle::SpriteOwner &particle_owner );

  //! @brief Draws a floor tile set at its world grid offset
  void draw_vertex_floor( Sprites::Containers::VertexFloor &floor_tiles );

  //! @brief Used by CryptScene for Priest NPC weapon
  //! @param floormap
  void render_shockwaves();

  //! @brief Used by GraveyardScene when player has key and relic carryitems
  void render_arrow_compass();

  //! @brief Draw "<key> <tiles left>" at the screen edge towards each off-screen courier crow (see NpcSystem::on_crow_delivery_event).
  void render_crow_delivery_indicators();

  //! @brief Build a world-space label: large glyphs scaled down into world units, black outline, origin centred.
  sf::Text make_world_label( const std::string &str, sf::Color fill ) const;

  //! @brief Centre a text's origin on its local bounds (call again after setString()).
  static void centre_text_origin( sf::Text &text );

  //! @brief Draw a compass needle's label beside its arrow, in world space.
  void render_compass_label( std::string_view label, sf::Color color, const Cmp::Player::ArrowCompass::Placement &placement );

  //! @brief Used by GraveyardScene to point each Cmp::Grave::TrailMarker grave towards its target grave
  void render_trail_markers();

  //! @brief Draw the burrs stuck to an NPC, one burr sprite per Cmp::BurrCount burr, where each struck.
  //! @param burr_cmp The NPC's burrs.
  //! @param body_rect The NPC's on-screen bounds.
  void render_attached_burrs( const Cmp::BurrCount &burr_cmp, const sf::FloatRect &body_rect );

  //! @brief Draw the world items a Burryman is carrying, scaled down and spaced evenly round his outer edge.
  //! @param burryman_cmp The Burryman's carried items.
  //! @param body_rect The Burryman's on-screen bounds.
  void render_carried_items( const Cmp::Npc::Burryman &burryman_cmp, const sf::FloatRect &body_rect );

  //! @brief Draw the world item a courier crow is carrying, scaled down and hanging under its body.
  //! @param crow_cmp The crow, see Cmp::Npc::Crow::carried.
  //! @param body_rect The crow's on-screen bounds.
  void render_carried_items( const Cmp::Npc::Crow &crow_cmp, const sf::FloatRect &body_rect );

  //! @brief The sprite frame to draw a carried world item at.
  //! @param item_entt The carried item entity.
  //! @param item_cmp The carried item's WorldItem component.
  //! @return The entity's AnimData frame offset (e.g. a curse tablet's random variant), else the item's sprite.index.
  [[nodiscard]] std::size_t carried_item_frame( entt::entity item_entt, const Cmp::WorldItem &item_cmp );

  //! @brief Draw the item in the player's inventory slot on the player, at the player's top-left and scale plus
  //! Cmp::Persist::HeldItemOffset, shifted by Cmp::Persist::HeldItemSideOffsetX towards the facing side when facing east/west.
  //! @param body_rect The player's on-screen bounds.
  //! @param facing The player's Cmp::LastDirection.
  //! @param scale The player's Cmp::AbsoluteScale (without any mirroring); multiplied by Cmp::Persist::HeldItemScale.
  //! @param alpha The player's alpha, so the item fades with the player.
  void render_held_item( const sf::FloatRect &body_rect, sf::Vector2f facing, sf::Vector2f scale, uint8_t alpha );

  //! @brief Draw a small wear-level bar above an item, filled proportionally to `wearlevel`.
  //! @param wearlevel
  //! @param pos
  void render_wear_level( float wearlevel, const Cmp::Position &pos );

  //! @brief Draw a semi-transparent ground shadow at the flying player's logical position
  //! (see FlyingSystem::shadow_bounds). Drawn from render_zorder_queue() at FlyingSystem::shadow_zorder().
  void render_flight_shadow();

  //! @brief Renders the flashing warning square over an armed obstacle in the game world
  void render_armed_indicator( const Cmp::Armed &armed_cmp, const Cmp::Position &pos_cmp );

  //! @brief Draw the inner-corner overlay sprites flagged in `corners_cmp` on top of a sinkhole cell.
  //! @param corners_cmp Which corners to draw.
  //! @param pos_cmp The sinkhole cell's position.
  void render_sinkhole_corners( const Cmp::Hazard::SinkholeCorners &corners_cmp, const Cmp::Position &pos_cmp );

  //! @brief Draws the zigzag vertex sequence shared by lightning strikes and obstacle cracks: a thick
  //! main line through the zero-index vertex of each row, with thinner lines to/from the other vertices.
  //! @param curve The vertex sequence and lifetime state to draw
  //! @param main_color Colour of the zero-index (main) line
  //! @param aux_color Colour of the non-zero-index (branch) lines
  //! @param main_thickness Thickness of the main line
  //! @param aux_thickness Thickness of the branch lines
  void render_fractal_curve( const Cmp::FractalCurve &curve );

  //! @brief Used by GraveyardScene when player is struck by lightning
  void render_lightning_strike( sf::Time dt );

  //! @brief Renders the crack decal sequence on obstacles that have taken damage.
  void render_obstacle_cracks();

  //! @brief Flashes the screen
  //! @param color
  void render_screen_flash( sf::Color color );

  //! @brief The main HUD layout (res/ui/ui.json), kept so the compass inset follows display resolution changes.
  std::unique_ptr<Render::UiLayout> m_hud_ui_layout;

  //! @brief Right edge (screen pixels) of the widest ui_outline in res/ui/ui.json. The compass arrow is kept to the right
  //! of this so it isn't lost among the UI panels on the left of the screen.
  float m_compass_left_inset_px{ 0.f };

  //! @brief Rescale the HUD layout to the current window size and recompute `m_compass_left_inset_px` (no-op if unchanged).
  void update_compass_left_inset();

  //! @brief Persistent compass label texts keyed by label, so their glyph geometry isn't regenerated every frame.
  std::unordered_map<std::string_view, sf::Text> m_compass_label_texts;
  //! @brief Persistent crow delivery distance texts keyed by carried item type; only re-strung when the distance changes.
  std::unordered_map<std::string, sf::Text> m_crow_delivery_texts;

  //! @brief Smoothed camera position, lerped towards the player's position each frame.
  sf::Vector2f m_camera_position{ 0.f, 0.f };

  //! @brief Whether `m_camera_position` has been seeded with the player's position yet, to avoid lerping from the origin on the first frame.
  bool m_camera_initialized{ false };

  //! @brief Copy of the window contents taken when the z-order loop reaches a post-process shader, which is then
  //! drawn into that shader's render texture. Reused across passes and frames.
  sf::Texture m_frame_capture;

  //! @brief Desaturating shader for Cmp::Greyscale sprites, see load_greyscale_shader()
  sf::Shader m_greyscale_shader;
  //! @brief Result of the lazy greyscale shader load; unset until first use
  std::optional<bool> m_greyscale_shader_ok;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_RENDERGAMESYSTEM_HPP__
