#ifndef SRC_SHADERS_DOLLYZOOMSHADER_HPP__
#define SRC_SHADERS_DOLLYZOOMSHADER_HPP__

#include <SFML/System/Angle.hpp>
#include <SFML/System/Time.hpp>

#include <Shaders/BaseShaderSprite.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sprites
{

//! @brief Full-screen post-process shader that fakes a dolly zoom ("vertigo effect") on the already-rendered game
//! frame: the player and their immediate surroundings keep their size while the rest of the scene surges in
//! towards them and then recedes past normal, tilting from side to side as it does, more strongly as the player's vertigo
//! toxicity rises.
//! See Factory::Shader::add_dolly_zoom.
class DollyZoomShader : public BaseShaderSprite
{
public:
  //! @brief Upper bound for set_zoom_amount(). Beyond this the zoomed-in warp would fold the image over itself.
  static constexpr float kMaxZoomAmount = 0.5f;

  //! @brief Upper bound for set_skew_amount().
  static constexpr sf::Angle kMaxSkewAmount = sf::degrees( 20.f );

  //! @brief Construct a new Dolly Zoom Shader object and immediately run setup().
  //! @param vert_shader_path Path to the vertex shader file.
  //! @param frag_shader_path Path to the fragment shader file.
  //! @param texture_size Size of the backing render texture, in pixels.
  //! @param zoom_amount How far the background zooms in and out; see set_zoom_amount().
  //! @param zoom_frequency How many in-and-out cycles per second; see set_zoom_frequency().
  //! @param skew_amount How far the zoom tilts to either side; see set_skew_amount().
  //! @param skew_frequency How many side-to-side tilts per second; see set_skew_frequency().
  DollyZoomShader( std::filesystem::path vert_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size,
                   float zoom_amount = 0.35f, float zoom_frequency = 0.25f, sf::Angle skew_amount = sf::degrees( 4.f ),
                   float skew_frequency = 0.4f )
      : BaseShaderSprite( vert_shader_path, frag_shader_path, texture_size )
  {
    set_zoom_amount( zoom_amount );
    set_zoom_frequency( zoom_frequency );
    set_skew_amount( skew_amount );
    set_skew_frequency( skew_frequency );
    setup();
  }
  ~DollyZoomShader() override = default;

  //! @brief Clear the render texture to black before the current frame is captured into it.
  void pre_setup_texture() override { m_render_texture.clear( sf::Color::Black ); }

  //! @brief Bind the shader's `texture` uniform to whatever is currently bound (the captured frame) and set the
  //! fixed `resolution` uniform to the render texture size.
  void post_setup_shader() override
  {
    m_shader.setUniform( "texture", sf::Shader::CurrentTexture );
    m_shader.setUniform( "resolution", sf::Vector2f{ m_render_texture.getSize() } );
  }

  //! @brief Refresh the zoom, skew and player screen position uniforms each frame.
  //! @param reg The entt registry, used to source the player's vertigo toxicity stat and position.
  void update( entt::registry &reg, sf::Time dt ) override;

  //! @brief This shader samples the already-rendered frame rather than blending a self-contained texture.
  //! @return Always true; see IShaderSprite::is_post_process() for how this changes composite behaviour.
  [[nodiscard]] bool is_post_process() const override { return true; }

  //! @brief Set how far the background zooms either side of normal, at full vertigo. At one extreme the edge of
  //! the scene is magnified by 1 / (1 - zoom_amount) and at the other shrunk by 1 / (1 + zoom_amount), e.g. 0.25
  //! swings between roughly 1.33x and 0.8x. Zooming out reveals area beyond the captured frame, which is filled
  //! by mirroring the frame's edges.
  //! @param zoom_amount Zoom amount, clamped to [0, kMaxZoomAmount].
  void set_zoom_amount( float zoom_amount ) { m_zoom_amount = std::clamp( zoom_amount, 0.f, kMaxZoomAmount ); }

  //! @brief Get how far the background zooms either side of normal.
  [[nodiscard]] float get_zoom_amount() const { return m_zoom_amount; }

  //! @brief Set how often the background zooms in and back out.
  //! @param zoom_frequency In-and-out cycles per second (Hz); negative values are treated as zero (the zoom
  //! holds still).
  void set_zoom_frequency( float zoom_frequency ) { m_zoom_frequency = std::max( zoom_frequency, 0.f ); }

  //! @brief Get how often the background zooms in and back out, in cycles per second.
  [[nodiscard]] float get_zoom_frequency() const { return m_zoom_frequency; }

  //! @brief Set how far the angle of the zoom tilts to either side, at full vertigo. The tilt rocks continuously
  //! on its own cycle, independent of the zoom. The scene twists around the player by this angle at its
  //! edge, tapering to nothing at the player, who stays upright.
  //! @param skew_amount Peak tilt, clamped to [0, kMaxSkewAmount]; 0 gives a straight zoom.
  void set_skew_amount( sf::Angle skew_amount ) { m_skew_amount = std::clamp( skew_amount, sf::Angle::Zero, kMaxSkewAmount ); }

  //! @brief Get how far the angle of the zoom tilts to either side.
  [[nodiscard]] sf::Angle get_skew_amount() const { return m_skew_amount; }

  //! @brief Set how fast the tilt rocks from side to side. A rate that is not a whole multiple of the zoom
  //! frequency keeps the tilt drifting in and out of step with the zoom.
  //! @param skew_frequency Side-to-side tilts per second (Hz); negative values are treated as zero.
  void set_skew_frequency( float skew_frequency ) { m_skew_frequency = std::max( skew_frequency, 0.f ); }

  //! @brief Get how fast the tilt rocks from side to side, in tilts per second.
  [[nodiscard]] float get_skew_frequency() const { return m_skew_frequency; }

private:
  //! @brief Peak zoom either side of normal at full vertigo, see set_zoom_amount().
  float m_zoom_amount{ 0.35f };
  //! @brief In-and-out cycles per second, see set_zoom_frequency().
  float m_zoom_frequency{ 0.25f };
  //! @brief Current position in the zoom cycle, in radians; advanced each update by m_zoom_frequency.
  float m_zoom_phase{ 0.f };
  //! @brief Peak tilt at full vertigo, see set_skew_amount().
  sf::Angle m_skew_amount{ sf::degrees( 4.f ) };
  //! @brief Side-to-side tilts per second, see set_skew_frequency().
  float m_skew_frequency{ 0.4f };
  //! @brief Current position in the tilt cycle, in radians; advanced each update by m_skew_frequency.
  float m_skew_phase{ 0.f };

  //! @brief Frame-rate independent exponential smoothing (Utils::Maths::exp_decay) of the raw toxicity stat,
  //! which only changes in coarse discrete steps and would otherwise make the zoom visibly snap.
  float m_smoothed_toxicity{ 0.f };
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_DOLLYZOOMSHADER_HPP__
