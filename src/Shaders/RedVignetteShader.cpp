#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>

#include <Shaders/RedVignetteShader.hpp>

namespace Game::Sprites
{

namespace
{
// How many "e-foldings" per second the displayed toxicity closes the gap to the real stat by; see
// m_smoothed_toxicity in RedVignetteShader.hpp for why this exists.
constexpr float kToxicitySmoothingRate = 3.0f;
} // namespace

void RedVignetteShader::update( entt::registry &reg )
{
  auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );
  float toxicity = 0;
  auto opt_hypoxia = Utils::Player::get_stats( reg ).toxidrome().at<Cmp::Toxicity::Hypoxia>();
  toxicity = static_cast<float>( opt_hypoxia.value_or( 0 ) ) / 100.f;

  sf::Time now = elapsed();
  float dt = ( now - m_last_toxicity_update ).asSeconds();
  m_last_toxicity_update = now;
  m_smoothed_toxicity = Utils::Maths::exp_decay( m_smoothed_toxicity, toxicity, kToxicitySmoothingRate, dt );

  Sprites::UniformBuilder{}.set( "resolution", display_size ).set( "toxicity", m_smoothed_toxicity ).apply( &get_shader() );

  set_position( { 0.f, 0.f } );
}

} // namespace Game::Sprites
