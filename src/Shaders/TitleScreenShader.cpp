
#include <Components/Persistent/DisplayResolution.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Utils/Constants.hpp>

#include <Shaders/TitleScreenShader.hpp>

namespace Game::Sprites
{

void TitleScreenShader::update( entt::registry &reg, sf::Time dt )
{
  m_timer += dt;
  auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg ) );
  const auto mouse_pos = sf::Vector2f( Constants::kFallbackDisplaySize );
  Sprites::UniformBuilder{}
      .set( "time", m_timer.asSeconds() )
      .set( "pixel_threshold", ( mouse_pos.x + mouse_pos.y ) / 30 )
      .set( "mouse_cursor", mouse_pos )
      .set( "resolution", display_size )
      .set( "time", m_timer.asSeconds() )
      .apply( &get_shader() );

  set_position( { 0, 0 } );
}

} // namespace Game::Sprites