#ifndef SRC_SHADERS_REDVIGNETTESHADER_HPP__
#define SRC_SHADERS_REDVIGNETTESHADER_HPP__

#include <SFML/System/Time.hpp>

#include <Shaders/BaseShaderSprite.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Sprites
{

//! @brief Full-screen post-process shader that darkens the edges of the already-rendered game frame
//! with a reddish vignette whose intensity tracks the player's hallucinogen toxicity stat.
//! Split out of CircularDistortionShader; see Factory::Shader::add_red_vignette.
class RedVignetteShader : public BaseShaderSprite
{
public:
  //! @brief Construct a new Red Vignette Shader object and immediately run setup().
  //! @param vert_shader_path Path to the vertex shader file.
  //! @param frag_shader_path Path to the fragment shader file.
  //! @param texture_size Size of the backing render texture, in pixels.
  RedVignetteShader( std::filesystem::path vert_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size )
      : BaseShaderSprite( vert_shader_path, frag_shader_path, texture_size )
  {
    setup();
  }
  ~RedVignetteShader() override = default;

  //! @brief Clear the render texture to black before the current frame is captured into it.
  void pre_setup_texture() override { m_render_texture.clear( sf::Color::Black ); }

  //! @brief Bind the shader's `texture` uniform to whatever is currently bound (the captured frame) and set the
  //! fixed `resolution` uniform to the render texture size.
  void post_setup_shader() override
  {
    m_shader.setUniform( "texture", sf::Shader::CurrentTexture );
    m_shader.setUniform( "resolution", sf::Vector2f{ m_render_texture.getSize() } );
  }

  //! @brief Refresh the smoothed hallucinogen toxicity level uniform each frame.
  //! @param reg The entt registry, used to source the player's hallucinogen toxicity stat.
  void update( entt::registry &reg ) override;

  //! @brief This shader samples the already-rendered frame rather than blending a self-contained texture.
  //! @return Always true; see IShaderSprite::is_post_process() for how this changes composite behaviour.
  [[nodiscard]] bool is_post_process() const override { return true; }

private:
  //! @brief Frame-rate independent exponential smoothing (Utils::Maths::exp_decay) of the raw hallucinogen
  //! toxicity stat, which only changes in coarse discrete steps; VIGNETTE_LOG_K in RedVignette.frag curves
  //! steeply at low toxicity, so the raw step would otherwise visibly snap.
  float m_smoothed_toxicity{ 0.f };

  //! @brief elapsed() timestamp of the last update() call, used to compute the per-frame dt.
  sf::Time m_last_toxicity_update;
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_REDVIGNETTESHADER_HPP__
