#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>

#include <Shaders/CircularDistortionShader.hpp>

namespace Game::Sprites
{

namespace
{
// How many "e-foldings" per second the displayed fear closes the gap to the real stat by; see
// m_smoothed_toxicity in CircularDistortionShader.hpp for why this exists.
constexpr float kToxicitySmoothingRate = 3.0f;
} // namespace

void CircularDistortionShader::update( entt::registry &reg, sf::Time dt )
{
  m_timer += dt;
  auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );
  auto opt_toxicity = Utils::Player::get_stats( reg ).toxidrome().at<Cmp::Toxicity::Hallucinogen>();
  float toxicity = static_cast<float>( opt_toxicity.value_or( 0 ) ) / 100.f;

  m_smoothed_toxicity = Utils::Maths::exp_decay( m_smoothed_toxicity, toxicity, kToxicitySmoothingRate, dt.asSeconds() );

  // Player's position in the same normalised [0,1] screen space as gl_FragCoord.xy/resolution, so a
  // frag shader (e.g. CircularDistortion.frag) can place effects relative to the player without knowing
  // anything about the world/camera itself. Mirrors how NightStaticShader reconstructs world
  // positions from view_top_left/view_size, just inverted.
  sf::Vector2f view_center = Sys::RenderSystem::get_world_view().getCenter();
  sf::Vector2f view_size = Sys::RenderSystem::get_world_view().getSize();
  sf::Vector2f view_top_left = { view_center.x - ( view_size.x / 2.f ), view_center.y - ( view_size.y / 2.f ) };
  sf::Vector2f player_world_pos = Utils::Player::get_position( reg ).getCenter();
  sf::Vector2f player_uv = { ( player_world_pos.x - view_top_left.x ) / view_size.x, ( player_world_pos.y - view_top_left.y ) / view_size.y };

  Sprites::UniformBuilder{}
      .set( "resolution", display_size )
      .set( "time", m_timer.asSeconds() )
      .set( "toxicity", m_smoothed_toxicity )
      .set( "player_uv", player_uv )
      .apply( &get_shader() );

  set_position( { 0.f, 0.f } );
}

} // namespace Game::Sprites
