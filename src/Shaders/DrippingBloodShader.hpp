#ifndef SRC_SHADERS_DRIPPINGBLOODSHADER_HPP__
#define SRC_SHADERS_DRIPPINGBLOODSHADER_HPP__

#include <SFML/System/Time.hpp>

#include <Shaders/BaseShaderSprite.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sprites
{

//! @brief Overlay shader that renders dripping blood streaks across the screen, faded in/out via
//! the player's Cmp::Player::Curse alpha and centered on the player's position.
class DrippingBloodShader : public BaseShaderSprite
{
public:
  //! @brief Construct a new Dripping Blood Shader object and immediately run setup().
  //! @param vert_shader_path Path to the vertex shader file.
  //! @param frag_shader_path Path to the fragment shader file.
  //! @param texture_size Size of the backing render texture, in pixels.
  DrippingBloodShader( std::filesystem::path vert_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size )
      : BaseShaderSprite( vert_shader_path, frag_shader_path, texture_size )
  {
    setup();
  }
  //! @brief Destroy the Dripping Blood Shader object
  ~DrippingBloodShader() override = default;

  //! @brief Clear the render texture with the shader's base color.
  void pre_setup_texture() override
  {
    m_render_texture.clear( sf::Color( 16, 32, 32 ) );
    // std::ignore = m_texture.resize(texture_size);
  }

  //! @brief Set the fixed `resolution` uniform to the render texture size.
  void post_setup_shader() override { m_shader.setUniform( "resolution", sf::Vector2f{ m_render_texture.getSize() } ); }

  //! @brief Refresh the curse-alpha/time uniforms and center the sprite on the player each frame.
  //! @param reg The entt registry, used to source the player's curse state and position.
  void update( entt::registry &reg, sf::Time dt ) override;
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_DRIPPINGBLOODSHADER_HPP__
