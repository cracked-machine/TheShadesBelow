#ifndef SRC_SHADERS_SCREENSHUDDERSHADER_HPP__
#define SRC_SHADERS_SCREENSHUDDERSHADER_HPP__

#include <SFML/System/Time.hpp>

#include <Shaders/BaseShaderSprite.hpp>

namespace Game::Sprites
{

//! @brief Full-screen post-process shader that shudders the already-rendered game frame, like the ground shaking
//! as a heavy stone door grinds open: a fast jolt as it breaks loose, a sustained heavy rumble while it slides,
//! then an ease-out as it settles. Idle (inactive, so skipped by RenderGameSystem) until trigger() is called, and
//! switches itself back off once the duration has elapsed.
//! See Factory::Shader::add_screen_shudder.
//!
//! @code
//! if ( auto shudder = Sys::ShaderSystem::find<Sprites::ScreenShudderShader>( reg(), "ScreenShudder" ) )
//!   shudder->get().trigger( sf::seconds( 2.5f ) ); // e.g. match the door-slide sound length
//! @endcode
class ScreenShudderShader : public BaseShaderSprite
{
public:
  //! @brief Upper bound for set_amplitude(), in world pixels.
  static constexpr float kMaxAmplitude = 32.f;

  //! @brief Construct a new Screen Shudder Shader object, immediately run setup() and leave it inactive.
  //! @param vert_shader_path Path to the vertex shader file.
  //! @param frag_shader_path Path to the fragment shader file.
  //! @param texture_size Size of the backing render texture, in pixels.
  //! @param duration Default shudder length used by trigger(); see set_duration().
  //! @param amplitude Peak shake distance; see set_amplitude().
  //! @param frequency Main rumble rate; see set_frequency().
  ScreenShudderShader( std::filesystem::path vert_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size,
                       sf::Time duration = sf::seconds( 1.5f ), float amplitude = 6.f, float frequency = 18.f )
      : BaseShaderSprite( vert_shader_path, frag_shader_path, texture_size )
  {
    set_duration( duration );
    set_amplitude( amplitude );
    set_frequency( frequency );
    setup();
    active( false );
  }
  ~ScreenShudderShader() override = default;

  //! @brief Clear the render texture to black before the current frame is captured into it.
  void pre_setup_texture() override { m_render_texture.clear( sf::Color::Black ); }

  //! @brief Bind the shader's `texture` uniform to whatever is currently bound (the captured frame) and set the
  //! fixed `resolution` uniform to the render texture size.
  void post_setup_shader() override
  {
    m_shader.setUniform( "texture", sf::Shader::CurrentTexture );
    m_shader.setUniform( "resolution", sf::Vector2f{ m_render_texture.getSize() } );
  }

  //! @brief Advance the shudder and refresh the offset/strength uniforms. Does nothing while idle; deactivates
  //! the shader once the duration has elapsed.
  //! @param reg The entt registry, used to source the display resolution.
  void update( entt::registry &reg, sf::Time dt ) override;

  //! @brief This shader samples the already-rendered frame rather than blending a self-contained texture.
  //! @return Always true; see IShaderSprite::is_post_process() for how this changes composite behaviour.
  [[nodiscard]] bool is_post_process() const override { return true; }

  //! @brief Start (or restart, if already running) the shudder for the given duration, which also becomes the
  //! new default for trigger().
  //! @param duration How long the shudder lasts; see set_duration().
  void trigger( sf::Time duration, float amplitude, float frequency )
  {
    set_duration( duration );
    set_amplitude( amplitude );
    set_frequency( frequency );
    trigger();
  }

  //! @brief Start (or restart, if already running) the shudder for the current duration.
  void trigger();

  //! @brief Stop the shudder immediately and return to idle.
  void stop();

  //! @brief Whether a shudder is currently running.
  [[nodiscard]] bool is_shuddering() const { return active(); }

  //! @brief Set how long a shudder lasts. Changing it mid-shudder stretches/shortens the running one.
  //! @param duration Shudder length; negative values are treated as zero (trigger() then does nothing).
  void set_duration( sf::Time duration ) { m_duration = std::max( duration, sf::Time::Zero ); }

  //! @brief Get how long a shudder lasts.
  [[nodiscard]] sf::Time get_duration() const { return m_duration; }

  //! @brief Set the peak shake distance, reached once the shudder has fully kicked in.
  //! @param amplitude Shake distance in world pixels, clamped to [0, kMaxAmplitude].
  void set_amplitude( float amplitude ) { m_amplitude = std::clamp( amplitude, 0.f, kMaxAmplitude ); }

  //! @brief Get the peak shake distance, in world pixels.
  [[nodiscard]] float get_amplitude() const { return m_amplitude; }

  //! @brief Set the main rumble rate. A slower sub-rumble and faster sideways judder are derived from it.
  //! @param frequency Rumble cycles per second (Hz); negative values are treated as zero.
  void set_frequency( float frequency ) { m_frequency = std::max( frequency, 0.f ); }

  //! @brief Get the main rumble rate, in cycles per second.
  [[nodiscard]] float get_frequency() const { return m_frequency; }

private:
  //! @brief Shudder length, see set_duration().
  sf::Time m_duration{ sf::seconds( 1.5f ) };
  //! @brief Time since trigger() was last called.
  sf::Time m_elapsed{ sf::Time::Zero };
  //! @brief Peak shake distance in world pixels, see set_amplitude().
  float m_amplitude{ 6.f };
  //! @brief Main rumble rate, see set_frequency().
  float m_frequency{ 18.f };
  //! @brief Accumulated phase of the main vertical rumble, in radians.
  float m_rumble_phase{ 0.f };
  //! @brief Accumulated phase of the slow vertical sub-rumble, in radians.
  float m_sub_phase{ 0.f };
  //! @brief Accumulated phase of the horizontal judder, in radians.
  float m_judder_phase{ 0.f };
  //! @brief Time since the grinding jolt target was last re-rolled.
  sf::Time m_jolt_timer{ sf::Time::Zero };
  //! @brief Counter seeding the jolt hash, bumped on every re-roll.
  std::uint32_t m_jolt_seed{ 0 };
  //! @brief Jolt offset (unit range) being blended away from.
  sf::Vector2f m_jolt_from{ 0.f, 0.f };
  //! @brief Jolt offset (unit range) being blended towards.
  sf::Vector2f m_jolt_to{ 0.f, 0.f };
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_SCREENSHUDDERSHADER_HPP__
