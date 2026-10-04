#ifndef SRC_FACTORY_SHADERFACTORY_HPP__
#define SRC_FACTORY_SHADERFACTORY_HPP__

#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/ZOrderValue.hpp>
#include <Shaders/CircularDistortionShader.hpp>
#include <Shaders/DarkModeShader.hpp>
#include <Shaders/DizzyEchoShader.hpp>
#include <Shaders/DollyZoomShader.hpp>
#include <Shaders/FloodWaterShader.hpp>
#include <Shaders/MistShader.hpp>
#include <Shaders/NightStaticShader.hpp>
#include <Shaders/RedVignetteShader.hpp>
#include <Shaders/TunnelVisionShader.hpp>
#include <Systems/ShaderSystem.hpp>

namespace Game::Factory::Shader
{

//! @brief Register the title screen shader.
//! @param shader_sys
//! @param display_res
void add_title( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res );

//! @brief Register the mist overlay shader, sized to double the map's pixel bounds.
//! @param shader_sys
//! @param map_size_pixel
void add_mist( Sys::ShaderSystem &shader_sys, sf::Vector2f map_size_pixel );

//! @brief Register the flood water shader, sized to double the map's pixel bounds.
//! @param shader_sys
//! @param map_size_pixel
void add_water( Sys::ShaderSystem &shader_sys, sf::Vector2f map_size_pixel );

//! @brief Register the night static/pulsing overlay shader.
//! @param shader_sys
//! @param map_size_pixel
void add_night_static( Sys::ShaderSystem &shader_sys, sf::Vector2f map_size_pixel );

//! @brief Register the dark mode overlay shader.
//! @param shader_sys
//! @param map_size_pixel
void add_dark( Sys::ShaderSystem &shader_sys, sf::Vector2f map_size_pixel );

//! @brief Register the curse (dripping blood) overlay shader.
//! @param shader_sys
//! @param map_size_pixel
void add_curse( Sys::ShaderSystem &shader_sys, sf::Vector2f map_size_pixel );

//! @brief Register the circular distortion full-screen post-process shader, sized to the display
//! resolution. Its intensity is driven each frame by the player's hallucinogen toxicity stat; see
//! Sprites::CircluarDistortionShader::update. Registered via ShaderSystem::add_post_process so it is
//! composited as a final pass rather than drawn inline in the world z-order queue.
//! @param shader_sys
//! @param display_res
void add_circular_distortion( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res );

//! @brief Register the red vignette full-screen post-process shader, sized to the display resolution. Its z-order
//! sits just above add_circular_distortion's, so RenderGameSystem chains it after the distortion pass.
//! Its intensity is driven each frame by the player's hallucinogen toxicity stat; see
//! Sprites::RedVignetteShader::update.
//! @param shader_sys
//! @param display_res
void add_red_vignette( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res );

//! @brief Register the tunnel vision full-screen post-process shader, sized to the display resolution. Its z-order
//! sits just above add_red_vignette's, so RenderGameSystem chains it last. The aperture around the player
//! constricts with the player's tachycardia/bradycardia toxicity stat; see Sprites::TunnelVisionShader::update.
//! @param shader_sys
//! @param display_res
void add_tunnel_vision( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res );

//! @brief Register the dizzy echo overlay shader, sized to the display resolution. It leaves echoes of the player,
//! NPC and world item sprites behind them; its z-order sits above the world sprites but below add_night_static's,
//! so the echoes are darkened by the night like the sprites they trail. The echoes strengthen with the player's
//! vertigo toxicity stat; see Sprites::DizzyEchoShader::update.
//! @param shader_sys
//! @param display_res
void add_dizzy_echo( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res );

//! @brief Register the dolly zoom full-screen post-process shader, sized to the display resolution. Its z-order
//! sits between add_circular_distortion's and add_tunnel_vision's, so the zoom includes the distortion but the
//! tunnel vision aperture and red vignette stay put. The zoom strengthens with the player's vertigo toxicity
//! stat; see Sprites::DollyZoomShader::update.
//! @param shader_sys
//! @param display_res
void add_dolly_zoom( Sys::ShaderSystem &shader_sys, const Cmp::Persist::DisplayResolution &display_res );

} // namespace Game::Factory::Shader

#endif // SRC_FACTORY_SHADERFACTORY_HPP__
