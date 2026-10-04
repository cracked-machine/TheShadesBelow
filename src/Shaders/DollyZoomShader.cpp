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

#include <Shaders/DollyZoomShader.hpp>

namespace Game::Sprites
{

namespace
{
// How many "e-foldings" per second the displayed toxicity closes the gap to the real stat by; see
// m_smoothed_toxicity in DollyZoomShader.hpp for why this exists.
constexpr float kToxicitySmoothingRate = 3.0f;
// Normalised toxicity (0..1) below which the zoom stays level; the skew only builds from here up
constexpr float kSkewOnsetToxicity = 0.25f;
} // namespace

void DollyZoomShader::update( entt::registry &reg, sf::Time dt )
{
  auto opt_toxicity = Utils::Player::get_stats( reg ).toxidrome().at<Cmp::Toxicity::Vertigo>();
  float toxicity = static_cast<float>( opt_toxicity.value_or( 0 ) ) / 100.f;

  m_smoothed_toxicity = Utils::Maths::exp_decay( m_smoothed_toxicity, toxicity, kToxicitySmoothingRate, dt.asSeconds() );

  // ease-in so the effect creeps in gently at low toxicity and only builds late: 0 -> 0, 0.5 -> 0.25, 1 -> 1
  const float strength = m_smoothed_toxicity * m_smoothed_toxicity;

  // accumulate the cycle positions rather than deriving them from elapsed time, so changing a frequency
  // mid-effect doesn't make the zoom jump
  m_zoom_phase = std::fmod( m_zoom_phase + ( 2.f * std::numbers::pi_v<float> * m_zoom_frequency * dt.asSeconds() ),
                            2.f * std::numbers::pi_v<float> );
  m_skew_phase = std::fmod( m_skew_phase + ( 2.f * std::numbers::pi_v<float> * m_skew_frequency * dt.asSeconds() ),
                            2.f * std::numbers::pi_v<float> );
  // swing from zoomed in by the full amount, through normal, to zoomed out by the same amount, once per cycle
  const float zoom = m_zoom_amount * strength * std::sin( m_zoom_phase );
  // Rock the angle of the zoom from side to side, continuously and on its own cycle, independent of the zoom.
  // Held off until kSkewOnsetToxicity, then eased in over the remaining range the same way as the zoom.
  const float skew_toxicity = std::max( m_smoothed_toxicity - kSkewOnsetToxicity, 0.f ) / ( 1.f - kSkewOnsetToxicity );
  const float skew = m_skew_amount.asRadians() * ( skew_toxicity * skew_toxicity ) * std::sin( m_skew_phase );

  // world -> screen UV, y flipped to match gl_FragCoord
  const auto &world_view = Sys::RenderSystem::get_world_view();
  sf::Vector2f view_size = world_view.getSize();
  sf::Vector2f view_top_left = world_view.getCenter() - view_size / 2.f;
  sf::Vector2f player_pos = Utils::Player::get_position( reg ).getCenter();
  sf::Vector2f player_uv{ ( player_pos.x - view_top_left.x ) / view_size.x, 1.f - ( ( player_pos.y - view_top_left.y ) / view_size.y ) };

  auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );
  Sprites::UniformBuilder{}.set( "resolution", display_size ).set( "player_uv", player_uv ).set( "zoom", zoom ).set( "skew", skew ).apply( &get_shader() );

  set_position( { 0.f, 0.f } );
}

} // namespace Game::Sprites
