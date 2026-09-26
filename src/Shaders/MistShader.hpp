#ifndef SRC_SHADERS_MISTSHADER_HPP__
#define SRC_SHADERS_MISTSHADER_HPP__

#include <SFML/System/Time.hpp>

#include <Shaders/BaseShaderSprite.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sprites
{

//! @brief Full-screen overlay shader that renders a semi-transparent drifting mist across the world
//! view, positioned off-sprite (drawn via the shader's own view-space uniforms rather than set_position()).
class MistShader : public BaseShaderSprite
{
public:
  //! @brief Construct a new Mist Shader object and immediately run setup().
  //! @param vert_shader_path Path to the vertex shader file.
  //! @param frag_shader_path Path to the fragment shader file.
  //! @param texture_size Size of the backing render texture, in pixels.
  MistShader( std::filesystem::path vert_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size )
      : BaseShaderSprite( vert_shader_path, frag_shader_path, texture_size )
  {
    setup();
  }

  //! @brief Destroy the Mist Shader object
  ~MistShader() override = default;

  //! @brief Clear the render texture with the shader's base color.
  void pre_setup_texture() override { m_render_texture.clear( sf::Color( 16, 128, 32 ) ); }

  //! @brief No additional one-time shader configuration is required for this shader.
  void post_setup_shader() override {}

  //! @brief Refresh the view-bounds/alpha/time uniforms each frame.
  //! @param reg The entt registry, used to source the display resolution.
  void update( entt::registry &reg, sf::Time dt ) override;
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_MISTSHADER_HPP__
