
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Position.hpp>
#include <Shaders/NightStaticShader.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>

#include <Shaders/DarkModeShader.hpp>

namespace Game::Sprites
{

void DarkModeShader::update( entt::registry &reg, sf::Time dt )
{
  m_timer += dt;
  auto display_res = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );
  sf::Vector2f aperture_half_size( Constants::kGridSizePxF * 4.f );

  Sprites::UniformBuilder{}
      .set( "local_resolution", Sys::RenderSystem::get_world_view().getSize() )
      .set( "display_resolution", display_res )
      .set( "aperture_half_size", aperture_half_size )
      .set( "time", m_timer.asSeconds() )
      .apply( &get_shader() );

  set_center_at_position( Utils::Player::get_position( reg ).position );
}

} // namespace Game::Sprites