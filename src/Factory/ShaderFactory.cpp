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
  shader_sys.add( std::move( circular_distortion_shader ), Cmp::ZOrderValue( 1000000.f ) );
}

void add_red_vignette( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res )
{
  auto red_vignette_shader = std::make_unique<Sprites::RedVignetteShader>( "res/shaders/Generic.vert", "res/shaders/RedVignette.frag", display_res );
  red_vignette_shader->set_tag( "RedVignette" );
  shader_sys.add( std::move( red_vignette_shader ), Cmp::ZOrderValue( 3000000.f ) );
}

void add_tunnel_vision( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res )
{
  auto tunnel_vision_shader = std::make_unique<Sprites::TunnelVisionShader>( "res/shaders/Generic.vert", "res/shaders/TunnelVision.frag",
                                                                             display_res );
  tunnel_vision_shader->set_tag( "TunnelVision" );
  shader_sys.add( std::move( tunnel_vision_shader ), Cmp::ZOrderValue( 2000000.f ) );
}

void add_dizzy_echo( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res )
{

  constexpr std::size_t frame_count = 4;
  constexpr float alpha_rolloff = 0.6f;
  constexpr sf::Time echo_delay = sf::milliseconds( 120 );
  constexpr float sway_radius = 40.f;
  constexpr float sway_frequency = 0.1f;
  constexpr float sway_ratio = 1.3f;
  auto dizzy_echo_shader = std::make_unique<Sprites::DizzyEchoShader>( "res/shaders/Generic.vert", "res/shaders/DizzyEcho.frag", display_res,
                                                                       frame_count, alpha_rolloff, echo_delay, sway_radius, sway_frequency,
                                                                       sway_ratio );

  dizzy_echo_shader->set_tag( "DizzyEcho" );
  shader_sys.add( std::move( dizzy_echo_shader ), Cmp::ZOrderValue( 30000.f ) );
}

void add_dolly_zoom( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res )
{

  constexpr float zoom_amount = 0.15f;
  constexpr float zoom_frequency = 0.15f;
  constexpr sf::Angle skew_amount = sf::degrees( 2.5f );
  constexpr float skew_frequency = 0.2f;
  auto dolly_zoom_shader = std::make_unique<Sprites::DollyZoomShader>( "res/shaders/Generic.vert", "res/shaders/DollyZoom.frag", display_res,
                                                                       zoom_amount, zoom_frequency, skew_amount, skew_frequency );

  dolly_zoom_shader->set_tag( "DollyZoom" );
  shader_sys.add( std::move( dolly_zoom_shader ), Cmp::ZOrderValue( 1500000.f ) );
}
} // namespace Game::Factory::Shader