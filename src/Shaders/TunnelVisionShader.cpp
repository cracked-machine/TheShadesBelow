#include <algorithm>

#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Player/HeartBeat.hpp>
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
// How quickly the heartbeat pulse falls off across one beat interval (higher = sharper thump)
constexpr float kHeartbeatPulseDecay = 6.0f;
} // namespace

void TunnelVisionShader::update( entt::registry &reg, sf::Time dt )
{
  const auto &toxidrome = Utils::Player::get_stats( reg ).toxidrome();
  auto opt_hypoxia = toxidrome.at<Cmp::Toxicity::Hypoxia>();
  auto opt_tachycardia = toxidrome.at<Cmp::Toxicity::Tachycardia>();
  auto opt_bradycardia = toxidrome.at<Cmp::Toxicity::Bradycardia>();

  // all three will increase tunnel vision so use whichever is the highest
  if ( opt_tachycardia.has_value() or opt_bradycardia.has_value() or opt_hypoxia.has_value() )
  {
    float toxicity = static_cast<float>( std::max( { opt_tachycardia.value_or( 0 ), opt_bradycardia.value_or( 0 ), opt_hypoxia.value_or( 0 ) } ) ) /
                     100.f;

    m_smoothed_toxicity = Utils::Maths::exp_decay( m_smoothed_toxicity, toxicity, kToxicitySmoothingRate, dt.asSeconds() );
  }

  // world -> screen UV, y flipped to match gl_FragCoord
  const auto &world_view = Sys::RenderSystem::get_world_view();
  sf::Vector2f view_size = world_view.getSize();
  sf::Vector2f view_top_left = world_view.getCenter() - view_size / 2.f;
  sf::Vector2f player_pos = Utils::Player::get_position( reg ).getCenter();
  sf::Vector2f player_uv{ ( player_pos.x - view_top_left.x ) / view_size.x, 1.f - ( ( player_pos.y - view_top_left.y ) / view_size.y ) };

  // only for Tachycardia and Bradycardia cause the tunnel vision aperture to pulse
  float heartbeat_pulse = 0;
  if ( opt_tachycardia.has_value() or opt_bradycardia.has_value() )
  {
    // Pulse spikes as each beat fires (phase 0) and decays over the beat interval; none at resting rate
    const auto &heartbeat = Utils::Player::get_heartbeat( reg );
    heartbeat_pulse = heartbeat.is_resting() ? 0.f : std::exp( -kHeartbeatPulseDecay * heartbeat.phase() );
  }

  auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );
  Sprites::UniformBuilder{}
      .set( "resolution", display_size )
      .set( "player_uv", player_uv )
      .set( "toxicity", m_smoothed_toxicity )
      .set( "heartbeat_pulse", heartbeat_pulse )
      .apply( &get_shader() );

  set_position( { 0.f, 0.f } );
}

} // namespace Game::Sprites
