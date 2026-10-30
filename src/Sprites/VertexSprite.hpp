#ifndef SRC_SPRITES_VERTEXSPRITE_HPP__
#define SRC_SPRITES_VERTEXSPRITE_HPP__

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/VertexArray.hpp>

namespace Game::Sys
{

//! @brief Mutable sprite containing a vertex array / texture.
class VertexSprite : public sf::Drawable, public sf::Transformable
{
public:
  //! @brief Construct a new Render Buffer object - used by RenderSystem::safe_render_sprite_to_target()
  //! @param vertices Pass by value to get a mutable copy
  //! @param texture  Reference to the texture to use. Avoid expensive copy.
  VertexSprite( const sf::VertexArray &vertices, const sf::Texture &texture )
      : m_vertices{ vertices },
        m_texture{ texture } {};

  //! @brief Set the alpha component on each vertex in the buffer
  //! @param alpha The alpha value to set (0-255)
  void set_pick_opacity( uint8_t alpha )
  {
    for ( std::size_t idx = 0; idx < m_vertices.getVertexCount(); ++idx )
      m_vertices[idx].color.a = alpha;
  }

  //! @brief Tint each vertex in the buffer: rgb is set from the tint, and alpha is multiplied with the
  //! opacity already set by set_pick_opacity().
  //! @param tint The tint colour (opaque white = no tint)
  void set_tint( sf::Color tint )
  {
    for ( std::size_t idx = 0; idx < m_vertices.getVertexCount(); ++idx )
    {
      auto &colour = m_vertices[idx].color;
      colour.r = tint.r;
      colour.g = tint.g;
      colour.b = tint.b;
      colour.a = static_cast<uint8_t>( colour.a * tint.a / 255 );
    }
  }

  //! @brief Draw the sprite buffer to the target
  //! @param target The render target to draw to
  //! @param states The render states to apply
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override
  {
    // apply the transform
    states.transform *= getTransform();

    // apply the tileset texture
    states.texture = &m_texture;

    // draw the vertex array
    target.draw( m_vertices, states );
  }

private:
  //! @brief The vertex array containing the sprite's geometry
  //! @note Stored by value to allow mutable operations
  sf::VertexArray m_vertices;

  //! @brief Reference to the texture used by the sprite buffer
  //! @note Stored by reference to avoid expensive copies
  const sf::Texture &m_texture;
};

} // namespace Game::Sys

#endif // SRC_SPRITES_VERTEXSPRITE_HPP__
