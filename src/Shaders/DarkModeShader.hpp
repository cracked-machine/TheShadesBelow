#ifndef SRC_SHADERS_DARKMODESHADER_HPP__
#define SRC_SHADERS_DARKMODESHADER_HPP__

#include <SFML/System/Time.hpp>

#include <Shaders/BaseShaderSprite.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sprites
{

//! @brief Full-screen overlay shader that darkens the scene around the player, revealing an
//! aperture of visibility centered on the player's position within the current world view.
class DarkModeShader : public BaseShaderSprite
{
public:
  //! @brief Construct a new Dark Mode Shader object and immediately run setup().
  //! @param vert_shader_path Path to the vertex shader file.
  //! @param frag_shader_path Path to the fragment shader file.
  //! @param texture_size Size of the backing render texture, in pixels.
  DarkModeShader( std::filesystem::path vert_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size )
      : BaseShaderSprite( vert_shader_path, frag_shader_path, texture_size )
  {
    setup();
  }

  //! @brief Destroy the Dark Mode Shader object
  ~DarkModeShader() override = default;

  //! @brief Clear the render texture with a dark color.
  void pre_setup_texture() override
  {
    // Clear the render texture with a dark color
    m_render_texture.clear( sf::Color( 0, 0, 0 ) );
  }

  //! @brief No additional one-time shader configuration is required for this shader.
  void post_setup_shader() override
  {
    // nothing special to do here
  }

  //! @brief Refresh the darkness/aperture uniforms and center the sprite on the player each frame.
  //! @param reg The entt registry, used to source the player's position and the current world view.
  void update( entt::registry &reg, sf::Time dt ) override;
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_DARKMODESHADER_HPP__
