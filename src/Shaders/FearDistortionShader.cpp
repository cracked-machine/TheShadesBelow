#include <Components/Persistent/DisplayResolution.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>

#include <Shaders/FearDistortionShader.hpp>

namespace Game::Sprites
{

namespace
{
// How many "e-foldings" per second the displayed fear closes the gap to the real stat by; see
// m_smoothed_fear in FearDistortionShader.hpp for why this exists.
constexpr float kFearSmoothingRate = 3.0f;
} // namespace

void FearDistortionShader::update( entt::registry &reg )
{
  auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );
  float target_fear = static_cast<float>( Utils::Player::get_stats( reg ).fear() ) / 100.f;

  sf::Time now = elapsed();
  float dt = ( now - m_last_fear_update ).asSeconds();
  m_last_fear_update = now;
  m_smoothed_fear = Utils::Maths::exp_decay( m_smoothed_fear, target_fear, kFearSmoothingRate, dt );

  // Player's position in the same normalised [0,1] screen space as gl_FragCoord.xy/resolution, so a
  // frag shader (e.g. FearRandomHaze.frag) can place effects relative to the player without knowing
  // anything about the world/camera itself. Mirrors how NightStaticShader reconstructs world
  // positions from view_top_left/view_size, just inverted.
  sf::Vector2f view_center = Sys::RenderSystem::get_world_view().getCenter();
  sf::Vector2f view_size = Sys::RenderSystem::get_world_view().getSize();
  sf::Vector2f view_top_left = { view_center.x - ( view_size.x / 2.f ), view_center.y - ( view_size.y / 2.f ) };
  sf::Vector2f player_world_pos = Utils::Player::get_position( reg ).getCenter();
  sf::Vector2f player_uv = { ( player_world_pos.x - view_top_left.x ) / view_size.x, ( player_world_pos.y - view_top_left.y ) / view_size.y };

  Sprites::UniformBuilder{}
      .set( "resolution", display_size )
      .set( "time", now.asSeconds() )
      .set( "fear", m_smoothed_fear )
      .set( "player_uv", player_uv )
      .apply( &get_shader() );

  set_position( { 0.f, 0.f } );
}

} // namespace Game::Sprites
