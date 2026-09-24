#include <Components/Persistent/DisplayResolution.hpp>
#include <Factory/ShaderFactory.hpp>
#include <Shaders/DrippingBloodShader.hpp>
#include <Shaders/TitleScreenShader.hpp>

namespace Game::Factory::Shader
{

void add_title( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res )
{
  auto title_screen_shader = std::make_unique<Sprites::TitleScreenShader>( "res/shaders/Generic.vert", "res/shaders/TitleScreen.frag", display_res );
  title_screen_shader->set_tag( "TitleShader" );
  shader_sys.add( std::move( title_screen_shader ), Cmp::ZOrderValue( 20000.f ) );
}

void add_mist( Sys::ShaderSystem &shader_sys, sf::Vector2f map_size_pixel )
{
  sf::Vector2u map_size_pixel_2u( map_size_pixel );
  auto mist_shader = std::make_unique<Sprites::MistShader>( "res/shaders/Generic.vert", "res/shaders/MistShader.frag",
                                                            map_size_pixel_2u.componentWiseMul( { 2, 2 } ) );
  mist_shader->set_tag( "MistShader" );
  shader_sys.add( std::move( mist_shader ), Cmp::ZOrderValue( 20000.f ) );
}

void add_water( Sys::ShaderSystem &shader_sys, sf::Vector2f map_size_pixel )
{
  sf::Vector2u map_size_pixel_2u( map_size_pixel );
  auto water_shader = std::make_unique<Sprites::FloodWaterShader>( "res/shaders/Generic.vert", "res/shaders/FloodWater2.frag",
                                                                   map_size_pixel_2u.componentWiseMul( { 2, 2 } ) );
  water_shader->set_tag( "WaterShader" );
  shader_sys.add( std::move( water_shader ), Cmp::ZOrderValue( -20000.f ) );
}

void add_night_static( Sys::ShaderSystem &shader_sys, sf::Vector2f map_size_pixel )
{
  sf::Vector2u map_size_pixel_2u( map_size_pixel );
  auto pulsing_shader = std::make_unique<Sprites::NightStaticShader>( "res/shaders/Generic.vert", "res/shaders/NightStatic.frag",
                                                                      map_size_pixel_2u.componentWiseMul( { 2, 2 } ) );
  pulsing_shader->set_tag( "NightStatic" );
  shader_sys.add( std::move( pulsing_shader ), Cmp::ZOrderValue( 40000.f ) );
}

void add_dark( Sys::ShaderSystem &shader_sys, sf::Vector2f map_size_pixel )
{
  sf::Vector2u map_size_pixel_2u( map_size_pixel );
  auto dark_mode_shader = std::make_unique<Sprites::DarkModeShader>( "res/shaders/Generic.vert", "res/shaders/DarkMode.frag", map_size_pixel_2u );
  dark_mode_shader->set_tag( "DarkShader" );
  shader_sys.add( std::move( dark_mode_shader ), Cmp::ZOrderValue( 20000.f ) );
}

void add_curse( Sys::ShaderSystem &shader_sys, sf::Vector2f map_size_pixel )
{
  sf::Vector2u map_size_pixel_2u( map_size_pixel );
  auto cursed_mode_shader = std::make_unique<Sprites::DrippingBloodShader>( "res/shaders/Generic.vert", "res/shaders/Generic.frag",
                                                                            map_size_pixel_2u );
  shader_sys.add( std::move( cursed_mode_shader ), Cmp::ZOrderValue( 20000.f ) );
}

void add_circular_distortion( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res )
{
  auto circular_distortion_shader = std::make_unique<Sprites::CircularDistortionShader>( "res/shaders/Generic.vert",
                                                                                         "res/shaders/CircularDistortion.frag", display_res );
  circular_distortion_shader->set_tag( "CircularDistortion" );
  shader_sys.add( std::move( circular_distortion_shader ), Cmp::ZOrderValue( 999999.f ) );
}

void add_red_vignette( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res )
{
  auto red_vignette_shader = std::make_unique<Sprites::RedVignetteShader>( "res/shaders/Generic.vert", "res/shaders/RedVignette.frag", display_res );
  red_vignette_shader->set_tag( "RedVignette" );
  shader_sys.add( std::move( red_vignette_shader ), Cmp::ZOrderValue( 1000000.f ) );
}
} // namespace Game::Factory::Shader