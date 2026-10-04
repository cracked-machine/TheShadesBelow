#include <algorithm>
#include <cmath>
#include <numbers>

#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/Toxicity/Vertigo.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>

#include <Shaders/DizzyEchoShader.hpp>

namespace Game::Sprites
{

namespace
{
// How many "e-foldings" per second the displayed toxicity closes the gap to the real stat by; see
// m_smoothed_toxicity in DizzyEchoShader.hpp for why this exists.
constexpr float kToxicitySmoothingRate = 3.0f;
// Smoothed toxicity below this is treated as "not dizzy": no echoes are drawn and no history is recorded
constexpr float kIdleToxicity = 0.001f;
} // namespace

DizzyEchoShader::DizzyEchoShader( std::filesystem::path vert_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size,
                                  std::size_t frame_count, float alpha_rolloff, sf::Time echo_delay,
                                  float sway_radius, float sway_frequency, float sway_ratio )
    : BaseShaderSprite( vert_shader_path, frag_shader_path, texture_size )
{
  set_frame_count( frame_count );
  set_alpha_rolloff( alpha_rolloff );
  set_echo_delay( echo_delay );
  set_sway_radius( sway_radius );
  set_sway_frequency( sway_frequency );
  set_sway_ratio( sway_ratio );

  m_history.reserve( kMaxEchoFrames );
  for ( std::size_t i = 0; i < kMaxEchoFrames; ++i )
  {
    m_history.emplace_back( texture_size );
  }
  setup();
}

void DizzyEchoShader::update( entt::registry &reg, sf::Time dt )
{
  // accumulate the orbit angle rather than deriving it from elapsed time, so changing the frequency mid-effect
  // doesn't make the echoes jump. Not wrapped to a single lap: a fractional sway ratio would jump at the wrap.
  m_sway_phase += 2.f * std::numbers::pi_v<float> * m_sway_frequency * dt.asSeconds();
  const auto &world_view = Sys::RenderSystem::get_world_view();

  auto opt_toxicity = Utils::Player::get_stats( reg ).toxidrome().at<Cmp::Toxicity::Vertigo>();
  float toxicity = static_cast<float>( opt_toxicity.value_or( 0 ) ) / 100.f;

  m_smoothed_toxicity = Utils::Maths::exp_decay( m_smoothed_toxicity, toxicity, kToxicitySmoothingRate, dt.asSeconds() );

  if ( m_smoothed_toxicity < kIdleToxicity )
  {
    // drop the history so stale frames can't reappear when the effect next kicks in
    m_valid_frames = 0;
    m_capture_timer = m_echo_delay; // capture immediately when the effect next kicks in
  }
  else if ( ( m_capture_timer += dt ) >= m_echo_delay )
  {
    m_capture_timer = sf::Time::Zero;
    // ShaderSystem::update runs before the frame is rendered, so m_render_texture still holds the sprite layer
    // that RenderGameSystem::capture_sprite_layer drew for the previous frame, and the world view is still the
    // one it was drawn with: that is the newest echo.
    auto &slot = m_history[m_head];
    slot.clear( sf::Color::Transparent );
    slot.draw( sf::Sprite( m_render_texture.getTexture() ), sf::BlendNone );
    slot.display();
    m_history_view_center[m_head] = world_view.getCenter();
    m_head = ( m_head + 1 ) % kMaxEchoFrames;
    m_valid_frames = std::min( m_valid_frames + 1, kMaxEchoFrames );
  }

  // history[0] is the most recent capture, walking backwards around the ring from m_head
  const std::size_t echo_count = std::min( m_frame_count, m_valid_frames );

  // ease-out so the echoes are already prominent at moderate toxicity: 0 -> 0, 0.5 -> 0.75, 1 -> 1
  const float strength = 1.f - ( ( 1.f - m_smoothed_toxicity ) * ( 1.f - m_smoothed_toxicity ) );

  // UV shift that maps a pixel of the current view onto the same world position in each echo's layer (plus the
  // echo's sway), so the echoes stay put in the world while the camera moves. y is flipped to match gl_FragCoord. The camera is only
  // updated after this, in RenderGameSystem::render_game, so the shift lags it by one frame.
  std::vector<sf::Glsl::Vec2> echo_offsets( kMaxEchoFrames );
  for ( std::size_t i = 0; i < echo_count; ++i )
  {
    const std::size_t slot_idx = ( m_head + kMaxEchoFrames - 1 - i ) % kMaxEchoFrames;
    get_shader().setUniform( "history[" + std::to_string( i ) + "]", m_history[slot_idx].getTexture() );

    // sway each echo around its anchor along a Lissajous figure, spreading the echoes evenly along the
    // horizontal cycle so they never bunch up
    const float sway_angle = m_sway_phase + ( 2.f * std::numbers::pi_v<float> * static_cast<float>( i ) / static_cast<float>( echo_count ) );
    const sf::Vector2f sway = sf::Vector2f{ std::cos( sway_angle ), std::sin( sway_angle * m_sway_ratio ) } * ( m_sway_radius * strength );

    const sf::Vector2f view_delta = world_view.getCenter() - m_history_view_center[slot_idx] + sway;
    echo_offsets[i] = { view_delta.x / world_view.getSize().x, -view_delta.y / world_view.getSize().y };
  }

  auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );
  Sprites::UniformBuilder{}
      .set( "resolution", display_size )
      .set( "frame_count", static_cast<int>( echo_count ) )
      .set( "echo_offset", echo_offsets )
      .set( "alpha_rolloff", m_alpha_rolloff )
      .set( "strength", strength )
      .apply( &get_shader() );

  set_position( { 0.f, 0.f } );
}

void DizzyEchoShader::resize_texture( sf::Vector2u new_size )
{
  BaseShaderSprite::resize_texture( new_size );
  for ( auto &frame : m_history )
  {
    [[maybe_unused]] auto result = frame.resize( new_size );
  }
  m_valid_frames = 0;
}

} // namespace Game::Sprites
