#ifndef SRC_SHADERS_TITLESCREENSHADER_HPP__
#define SRC_SHADERS_TITLESCREENSHADER_HPP__

#include <SFML/System/Time.hpp>

#include <Shaders/BaseShaderSprite.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sprites
{

//! @brief Full-screen post-process shader applying a pixelation/dissolve effect over the title screen.
class TitleScreenShader : public BaseShaderSprite
{
public:
  //! @brief Construct a new Title Screen Shader object and immediately run setup().
  //! @param vert_shader_path Path to the vertex shader file.
  //! @param frag_shader_path Path to the fragment shader file.
  //! @param texture_size Size of the backing render texture, in pixels.
  TitleScreenShader( std::filesystem::path vert_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size )
      : BaseShaderSprite( vert_shader_path, frag_shader_path, texture_size )
  {
    setup();
  }
  //! @brief Destroy the Title Screen Shader object
  ~TitleScreenShader() override = default;

  //! @brief Clear the render texture with the shader's base color.
  void pre_setup_texture() override
  {
    m_render_texture.clear( sf::Color( 128, 128, 128 ) );
    // std::ignore = m_texture.resize(texture_size);
  }

  //! @brief Bind the shader's `texture` uniform to whatever is currently bound.
  void post_setup_shader() override { m_shader.setUniform( "texture", sf::Shader::CurrentTexture ); }

  //! @brief Refresh the pixelation/time/resolution uniforms each frame.
  //! @param reg The entt registry, used to source the display resolution.
  void update( entt::registry &reg, sf::Time dt ) override;
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_TITLESCREENSHADER_HPP__
