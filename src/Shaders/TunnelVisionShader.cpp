#include <algorithm>

#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>

#include <Shaders/TunnelVisionShader.hpp>

namespace Game::Sprites
{

namespace
{
// How many "e-foldings" per second the displayed toxicity closes the gap to the real stat by; see
// m_smoothed_toxicity in TunnelVisionShader.hpp for why this exists.
constexpr float kToxicitySmoothingRate = 3.0f;
} // namespace

void TunnelVisionShader::update( entt::registry &reg )
{
  // Tachycardia and Bradycardia are mutually exclusive, so at most one is non-zero
  const auto &toxidrome = Utils::Player::get_stats( reg ).toxidrome();
  auto opt_tachycardia = toxidrome.at<Cmp::Toxicity::Tachycardia>();
  auto opt_bradycardia = toxidrome.at<Cmp::Toxicity::Bradycardia>();
  float toxicity = static_cast<float>( std::max( opt_tachycardia.value_or( 0 ), opt_bradycardia.value_or( 0 ) ) ) / 100.f;
  sf::Time now = elapsed();
  float dt = ( now - m_last_toxicity_update ).asSeconds();
  m_last_toxicity_update = now;
  m_smoothed_toxicity = Utils::Maths::exp_decay( m_smoothed_toxicity, toxicity, kToxicitySmoothingRate, dt );

  // world -> screen UV, y flipped to match gl_FragCoord
  const auto &world_view = Sys::RenderSystem::get_world_view();
  sf::Vector2f view_size = world_view.getSize();
  sf::Vector2f view_top_left = world_view.getCenter() - view_size / 2.f;
  sf::Vector2f player_pos = Utils::Player::get_position( reg ).getCenter();
  sf::Vector2f player_uv{ ( player_pos.x - view_top_left.x ) / view_size.x, 1.f - ( ( player_pos.y - view_top_left.y ) / view_size.y ) };

  auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );
  Sprites::UniformBuilder{}
      .set( "resolution", display_size )
      .set( "player_uv", player_uv )
      .set( "toxicity", m_smoothed_toxicity )
      .apply( &get_shader() );

  set_position( { 0.f, 0.f } );
}

} // namespace Game::Sprites
