
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Position.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Utils/Player.hpp>

#include <Shaders/DrippingBloodShader.hpp>

namespace Game::Sprites
{

void DrippingBloodShader::update( entt::registry &reg, sf::Time dt )
{
  m_timer += dt;
  auto &player_curse = Utils::Player::get_curse( reg );
  auto display_res = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );

  Sprites::UniformBuilder{}
      .set( "alpha", player_curse.shader_alpha.add( 0.01f ) )
      .set( "resolution", display_res )
      .set( "time", m_timer.asSeconds() )
      .apply( &get_shader() );

  set_center_at_position( Utils::Player::get_position( reg ).position );
}

} // namespace Game::Sprites