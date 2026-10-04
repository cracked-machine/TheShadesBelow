#ifndef SRC_SHADERS_DIZZYECHOSHADER_HPP__
#define SRC_SHADERS_DIZZYECHOSHADER_HPP__

#include <SFML/System/Time.hpp>

#include <Shaders/BaseShaderSprite.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sprites
{

//! @brief Full-screen overlay shader that leaves N "echoes" of the player, NPC and world item sprites behind them,
//! spaced one echo delay apart and anchored to where the sprites were in the world. The echoes also sway around
//! that anchor, so they keep moving while the player stands still. Each echo is drawn at diminishing alpha,
//! with an overall strength that rises with the player's vertigo toxicity.
//! See Factory::Shader::add_dizzy_echo.
class DizzyEchoShader : public BaseShaderSprite
{
public:
  //! @brief Upper bound on the number of echoed frames. Must match MAX_ECHO_FRAMES in DizzyEcho.frag.
  static constexpr std::size_t kMaxEchoFrames = 8;

  //! @brief Construct a new Dizzy Echo Shader object and immediately run setup().
  //! @param vert_shader_path Path to the vertex shader file.
  //! @param frag_shader_path Path to the fragment shader file.
  //! @param texture_size Size of the backing render texture, in pixels.
  //! @param frame_count Number of echoes; see set_frame_count().
  //! @param alpha_rolloff Alpha multiplier applied per successive echo; see set_alpha_rolloff().
  //! @param echo_delay Time between successive echoes; see set_echo_delay().
  //! @param sway_radius How far the echoes sway from their anchor; see set_sway_radius().
  //! @param sway_frequency How many times per second the echoes swing side to side; see set_sway_frequency().
  //! @param sway_ratio Vertical sway rate as a multiple of the horizontal rate; see set_sway_ratio().
  DizzyEchoShader( std::filesystem::path vert_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size,
                   std::size_t frame_count = 4, float alpha_rolloff = 0.6f, sf::Time echo_delay = sf::milliseconds( 120 ),
                   float sway_radius = 6.f, float sway_frequency = 0.4f, float sway_ratio = 1.3f );
  ~DizzyEchoShader() override = default;

  //! @brief Clear the render texture to transparent before the sprite layer is drawn into it.
  void pre_setup_texture() override { m_render_texture.clear( sf::Color::Transparent ); }

  //! @brief Set the fixed `resolution` uniform to the render texture size.
  void post_setup_shader() override { m_shader.setUniform( "resolution", sf::Vector2f{ m_render_texture.getSize() } ); }

  //! @brief Push the previous frame's sprite layer into the echo history once per echo delay and refresh the
  //! strength, echo count, roll-off and per-echo world offset uniforms each frame.
  //! @param reg The entt registry, used to source the player's vertigo toxicity stat.
  void update( entt::registry &reg, sf::Time dt ) override;

  //! @brief Resize the render texture and every echo history texture; the stale history is discarded.
  //! @param new_size The new render texture size, in pixels.
  void resize_texture( sf::Vector2u new_size ) override;

  //! @brief This shader samples a layer holding only the player, NPC and world item sprites.
  //! @return Always true; see IShaderSprite::is_sprite_layer() for how this changes composite behaviour.
  [[nodiscard]] bool is_sprite_layer() const override { return true; }

  //! @brief Set how many echoes are drawn.
  //! @param frame_count Number of echoes, clamped to [1, kMaxEchoFrames].
  void set_frame_count( std::size_t frame_count ) { m_frame_count = std::clamp<std::size_t>( frame_count, 1, kMaxEchoFrames ); }

  //! @brief Get how many echoes are drawn.
  [[nodiscard]] std::size_t get_frame_count() const { return m_frame_count; }

  //! @brief Set the alpha roll-off: echo i (0 = the most recent) is blended at alpha
  //! `strength * alpha_rolloff^i`, so lower values make the echoes fade out faster.
  //! @param alpha_rolloff Per-echo alpha multiplier, clamped to [0, 1].
  void set_alpha_rolloff( float alpha_rolloff ) { m_alpha_rolloff = std::clamp( alpha_rolloff, 0.f, 1.f ); }

  //! @brief Get the per-echo alpha multiplier.
  [[nodiscard]] float get_alpha_rolloff() const { return m_alpha_rolloff; }

  //! @brief Set the time between successive echoes. Longer delays spread the echoes further apart; each echo
  //! holds its captured frame for one delay before stepping on, so the trail is deliberately not smooth.
  //! @param echo_delay Time between echoes; negative values are treated as zero (capture every frame).
  void set_echo_delay( sf::Time echo_delay ) { m_echo_delay = std::max( echo_delay, sf::Time::Zero ); }

  //! @brief Get the time between successive echoes.
  [[nodiscard]] sf::Time get_echo_delay() const { return m_echo_delay; }

  //! @brief Set how far the echoes sway from their world anchor at full vertigo. Each echo traces a
  //! Lissajous figure around its anchor, out of phase with the others, so they dance around the sprite even when
  //! nothing is moving.
  //! @param sway_radius Sway distance in world pixels; negative values are treated as zero (no sway).
  void set_sway_radius( float sway_radius ) { m_sway_radius = std::max( sway_radius, 0.f ); }

  //! @brief Get how far the echoes sway from their world anchor at full vertigo, in world pixels.
  [[nodiscard]] float get_sway_radius() const { return m_sway_radius; }

  //! @brief Set how fast the echoes sway around their anchor.
  //! @param sway_frequency Side-to-side swings per second (Hz); negative values are treated as zero (echoes
  //! hold still).
  void set_sway_frequency( float sway_frequency ) { m_sway_frequency = std::max( sway_frequency, 0.f ); }

  //! @brief Get how fast the echoes sway around their anchor, in side-to-side swings per second.
  [[nodiscard]] float get_sway_frequency() const { return m_sway_frequency; }

  //! @brief Set the shape of the Lissajous figure: the vertical sway runs at this multiple of the horizontal
  //! rate. 1 gives a plain circle, 2 a figure-of-eight; values in between (e.g. 1.3) drift through both, so the
  //! figure wanders rather than repeating every swing.
  //! @param sway_ratio Vertical-to-horizontal frequency ratio; negative values are treated as zero.
  void set_sway_ratio( float sway_ratio ) { m_sway_ratio = std::max( sway_ratio, 0.f ); }

  //! @brief Get the vertical-to-horizontal sway frequency ratio.
  [[nodiscard]] float get_sway_ratio() const { return m_sway_ratio; }

private:
  //! @brief Ring buffer of the last kMaxEchoFrames captured sprite layers (one per echo delay), each the same
  //! size as m_render_texture. Render textures (rather than plain textures) so the stored layers keep
  //! m_render_texture's orientation and can be sampled with the same gl_FragCoord-derived UV.
  std::vector<sf::RenderTexture> m_history;
  //! @brief World view centre each m_history slot was rendered with, used to keep its echo anchored to the
  //! world as the camera moves.
  std::array<sf::Vector2f, kMaxEchoFrames> m_history_view_center{};
  //! @brief Index of the m_history slot the next layer will be written to.
  std::size_t m_head{ 0 };
  //! @brief How many m_history slots hold real layers; reset whenever the effect is idle or the textures resize.
  std::size_t m_valid_frames{ 0 };

  //! @brief Number of echoes to draw, see set_frame_count().
  std::size_t m_frame_count{ 4 };
  //! @brief Per-echo alpha multiplier, see set_alpha_rolloff().
  float m_alpha_rolloff{ 0.6f };
  //! @brief Time between successive echoes, see set_echo_delay().
  sf::Time m_echo_delay{ sf::milliseconds( 120 ) };
  //! @brief Sway distance in world pixels at full vertigo, see set_sway_radius().
  float m_sway_radius{ 6.f };
  //! @brief Side-to-side swings per second, see set_sway_frequency().
  float m_sway_frequency{ 0.4f };
  //! @brief Vertical-to-horizontal sway frequency ratio, see set_sway_ratio().
  float m_sway_ratio{ 1.3f };
  //! @brief Current horizontal sway angle of the first echo, in radians; advanced each update by m_sway_frequency.
  float m_sway_phase{ 0.f };
  //! @brief Time since the last layer was captured into m_history.
  sf::Time m_capture_timer{ sf::Time::Zero };

  //! @brief Frame-rate independent exponential smoothing (Utils::Maths::exp_decay) of the raw toxicity stat,
  //! which only changes in coarse discrete steps and would otherwise make the echoes visibly snap in and out.
  float m_smoothed_toxicity{ 0.f };
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_DIZZYECHOSHADER_HPP__
