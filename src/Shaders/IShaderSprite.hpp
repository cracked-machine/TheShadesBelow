#ifndef SRC_SHADERS_ISHADERSPRITE_HPP__
#define SRC_SHADERS_ISHADERSPRITE_HPP__

namespace Game::Sprites
{

class IShaderSprite;

//! @brief Common interface for SFML fragment-shader-based drawable overlays/post-process effects
//! (e.g. DarkModeShader, MistShader, FearDistortionShader). Implementations own a render texture,
//! sprite and shader, and are driven once per frame by ShaderSystem::update( dt ), called from each scene's do_update().
class IShaderSprite : public sf::Drawable, public sf::Transformable
{
public:
  virtual ~IShaderSprite() = default;

  //! @brief Prepare the render texture before the shader is loaded (e.g. clear it to the effect's base color).
  virtual void pre_setup_texture() = 0;

  //! @brief Configure the shader's static/one-time uniforms once it has been loaded.
  virtual void post_setup_shader() = 0;

  //! @brief Called once per frame to refresh the shader's dynamic uniforms and reposition the sprite.
  //! @param reg The entt registry, used to source gameplay state (player position, stats, etc.) for uniforms.
  virtual void update( entt::registry &reg, sf::Time dt ) = 0;

  //! @brief Set an identifying tag for this shader instance (e.g. for lookup/debugging).
  //! @param tag The tag to assign.
  virtual void set_tag( const std::string &tag ) = 0;

  //! @brief Get this shader instance's identifying tag.
  //! @return The assigned tag.
  [[nodiscard]] virtual std::string get_tag() const = 0;

  //! @brief Resize the underlying render texture and sprite to match a new display/window size.
  //! @param new_size The new render texture size, in pixels.
  virtual void resize_texture( sf::Vector2u new_size ) = 0;

  //! @brief Enable or disable this shader so it is skipped/included by the render pipeline.
  //! @param active True to enable rendering of this shader, false to disable it.
  virtual void active( bool active ) = 0;

  //! @brief Whether this shader is currently enabled.
  //! @return True if the shader is active and should be rendered.
  virtual bool active() const = 0;

  //! @brief Access the render texture this sprite's shader will sample as its `texture` uniform.
  //! Used by RenderGameSystem to redirect a frame's drawing into a post-process shader (e.g.
  //! FearDistortionShader) instead of the flat clear color the other overlay shaders use.
  virtual sf::RenderTexture &get_render_texture() = 0;

  //! @brief Whether this shader samples everything drawn so far (rather than blending its own
  //! self-contained texture onto the scene, like the other overlay shaders do). A GPU can't read
  //! and write the same render target in one draw, so RenderGameSystem's z-order loop handles a
  //! post-process shader differently when it reaches its ZOrderValue-driven position in the queue:
  //! it finalizes whatever's been captured into get_render_texture() so far and composites the
  //! distorted result onto the window, instead of just blending this sprite in place like a normal
  //! overlay. See FearDistortionShader for the concrete example.
  [[nodiscard]] virtual bool is_post_process() const { return false; }

protected:
  //! @brief Access the underlying SFML shader object, for setting uniforms.
  //! @return Reference to the wrapped sf::Shader.
  virtual sf::Shader &get_shader() = 0;
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_ISHADERSPRITE_HPP__
