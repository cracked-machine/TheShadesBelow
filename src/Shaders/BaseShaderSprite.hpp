#ifndef SRC_SHADERS_BASESHADERSPRITE_HPP__
#define SRC_SHADERS_BASESHADERSPRITE_HPP__

#include <Shaders/IShaderSprite.hpp>

namespace Game::Sprites
{

//! @brief Base class for fragment shader-based drawable objects in SFML.
//!
//! This class provides a framework for creating drawable objects that apply fragment shaders
//! to render textures.
//!
//! The class manages a render texture, sprite, and shader, providing a standardized
//! initialization sequence and interface for derived classes to implement custom
//! shader effects.
//!
//! @note This class is move-only (copy operations are deleted).
//! @note Derived classes must implement the pure virtual functions to define
//!       texture setup, shader configuration, and runtime updates.
//!
//! Usage pattern:
//! 1. Create derived class implementing pure virtual functions
//! 2. Call setup() to initialize the shader system;
//!    You can do this from the constructor if you wish
//! 3. Use set_position() to position the drawable
//! 4. Call update() in your render loop for dynamic effects
//! 5. Draw using SFML's standard drawing mechanisms
//!
//! @note If you need to update with additional uniforms,
//!     create a new update function in your derived class
class BaseShaderSprite : public IShaderSprite
{
public:
  //! @brief Construct a new Base Shader Sprite object. Does not call setup(); derived classes are
  //! responsible for calling it once their pre_setup_texture()/post_setup_shader() overrides are usable.
  //! @param vertex_shader_path Path to the vertex shader file.
  //! @param frag_shader_path Path to the fragment shader file.
  //! @param texture_size Size of the backing render texture, in pixels.
  BaseShaderSprite( std::filesystem::path vertex_shader_path, std::filesystem::path frag_shader_path, sf::Vector2u texture_size );

  //! @brief Deleted: this class is move-only.
  BaseShaderSprite( const BaseShaderSprite & ) = delete;
  //! @brief Deleted: this class is move-only.
  BaseShaderSprite &operator=( const BaseShaderSprite & ) = delete;
  //! @brief Default move constructor.
  BaseShaderSprite( BaseShaderSprite && ) = default;
  //! @brief Default move assignment.
  BaseShaderSprite &operator=( BaseShaderSprite && ) = default;

  //! @brief polymorphic destructor for derived classes
  ~BaseShaderSprite() override = default;

  //! @brief Initializes and configures the base fragment shader.
  //!
  //! This method performs the necessary setup operations for the fragment shader:
  //! 1. pre_setup_texture()
  //! 2. m_texture.display()
  //! 3. post_setup_shader()
  //!
  //! @throws std::runtime_error if shader compilation or linking fails
  void setup();

  //! @brief Set the position of the renderable sprite.
  //! @param position The new position, in the coordinate space this sprite is drawn into.
  void set_position( const sf::Vector2f &position );

  //! @brief Overrides sf::Transformable::setPosition, which has no meaning in the context of this
  //! class, to avoid unexpected behavior where the position of the sprite is not set correctly.
  //! @param position The new position, forwarded to set_position().
  void setPosition( const sf::Vector2f &position );

  //! @brief Position the sprite so its render texture is centered on the given world position.
  //! @param pos The world position to center the sprite on.
  void set_center_at_position( sf::Vector2f pos );

  //! @brief Set the view used when rendering into the internal render texture.
  //! @param view_update The new view.
  void set_texture_view( sf::View view_update );

  //! @brief Internal draw function called by SFML; draws the sprite with this shader bound.
  //! @param target The render target to draw into.
  //! @param states Render states; the shader is attached to these before drawing.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;

  //! @brief Get the size of the underlying render texture.
  //! @return The render texture size, in pixels.
  auto get_texture_size() const { return m_render_texture.getSize(); }

  //! @brief Resize the underlying render texture and sprite to match a new display/window size.
  //! @param new_size The new render texture size, in pixels.
  void resize_texture( sf::Vector2u new_size ) override;

  //! @brief Set an identifying tag for this shader instance.
  //! @param tag The tag to assign.
  void set_tag( const std::string &tag ) override { m_tag = tag; }

  //! @brief Get this shader instance's identifying tag.
  //! @return The assigned tag.
  std::string get_tag() const override { return m_tag; }

  //! @brief Enable or disable this shader so it is skipped/included by the render pipeline.
  //! @param active True to enable rendering of this shader, false to disable it.
  void active( bool active ) override { m_active = active; }

  //! @brief Whether this shader is currently enabled.
  //! @return True if the shader is active and should be rendered.
  bool active() const override { return m_active; }

  //! @brief Access the render texture this sprite's shader samples/renders into.
  //! @return Reference to the render texture.
  sf::RenderTexture &get_render_texture() override { return m_render_texture; }

protected:
  //! @brief Access the underlying SFML shader object, for setting uniforms.
  //! @return Reference to the wrapped sf::Shader.
  sf::Shader &get_shader() override { return m_shader; }

  //! @brief The pallette texture that the shader will be applied to.
  sf::RenderTexture m_render_texture;
  //! @brief The sprite that uses m_render_texture.
  sf::Sprite m_sprite{ m_render_texture.getTexture() };
  //! @brief The vertex/fragment shader to be applied to the sprite.
  sf::Shader m_shader;
  //! @brief Time accumulated by derived classes from the dt passed to update() (`m_timer += dt;`), used to drive time-based uniforms.
  sf::Time m_timer{ sf::Time::Zero };

private:
  //! @brief Filesystem path to the vertex shader source, used by load_shader_files().
  std::filesystem::path m_vert_shader_path;
  //! @brief Filesystem path to the fragment shader source, used by load_shader_files().
  std::filesystem::path m_frag_shader_path;

  //! @brief Load and compile m_vert_shader_path/m_frag_shader_path into m_shader.
  //! @throws std::filesystem::filesystem_error if either shader file does not exist.
  //! @throws std::runtime_error if shader compilation or linking fails.
  void load_shader_files();

  //! @brief Identifying tag for this shader instance, set/read via set_tag()/get_tag().
  std::string m_tag;
  //! @brief Whether this shader is currently enabled, set/read via active().
  bool m_active{ true };
};

} // namespace Game::Sprites

#endif // SRC_SHADERS_BASESHADERSPRITE_HPP__
