#include <Components/Persistent/DisplayResolution.hpp>
#include <Shaders/FloodWaterShader.hpp>
#include <Shaders/MistShader.hpp>
#include <Shaders/UniformBuilder.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <entt/entity/registry.hpp>

namespace Game::Sprites
{

void FloodWaterShader::update( entt::registry &reg )
{
  sf::Vector2u display_size = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg );
  sf::Vector2f view_center = Sys::RenderSystem::get_world_view().getCenter();
  sf::Vector2f view_size = Sys::RenderSystem::get_world_view().getSize();
  sf::Vector2f view_top_left = { view_center.x - view_size.x / 2.f, view_center.y - view_size.y / 2.f };
  sf::Vector2f map_size = sf::Vector2f( get_texture_size() );

  Sprites::UniformBuilder{}
      .set( "resolution", sf::Vector2f{ display_size } )
      .set( "viewTopLeft", view_top_left )
      .set( "viewSize", view_size )
      .set( "mapSize", map_size )
      .set( "time", elapsed().asSeconds() )
      .apply( &get_shader() );

  // clang-format on
  set_position( { -200, -100 } );
}

} // namespace Game::Sprites