#ifndef SRC_SHADERS_FLOODWATERSHADER_HPP__
#define SRC_SHADERS_FLOODWATERSHADER_HPP__

#include <SFML/System/Time.hpp>

#include <Shaders/BaseShaderSprite.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sprites
{

//! @brief Full-screen overlay shader that renders rising flood water across the world view,
//! positioned off-sprite (drawn via the shader's own view-space uniforms rather than set_position()).
class FloodWaterShader : public BaseShaderSprite
{
public:
  //! @brief Construct a new Flood Water Shader object and immediately run setup().
  //! @param vert_shader_path Path to the vertex shader file.
  //! @param frag_shader_path Path to the fragment shader file.
  //! @param texture_size Size of the backing render texture, in pixels.
  FloodWaterShader( std::filesystem::path vert_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size )
      : BaseShaderSprite( vert_shader_path, frag_shader_path, texture_size )
  {
    setup();
  }
  //! @brief Destroy the Flood Water Shader object
  ~FloodWaterShader() override = default;

  //! @brief Clear the render texture with the shader's base color.
  void pre_setup_texture() override { m_render_texture.clear( sf::Color( 16, 32, 32 ) ); }

  //! @brief Set the fixed `resolution` uniform to the render texture size.
  void post_setup_shader() override { m_shader.setUniform( "resolution", sf::Vector2f{ m_render_texture.getSize() } ); }

  //! @brief Refresh the view-bounds/time uniforms each frame.
  //! @param reg The entt registry, used to source the display resolution.
  void update( entt::registry &reg, sf::Time dt ) override;
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_FLOODWATERSHADER_HPP__
