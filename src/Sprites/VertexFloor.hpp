#ifndef SRC_SPRITES_VERTEXFLOOR_HPP__
#define SRC_SPRITES_VERTEXFLOOR_HPP__

#include <SceneControl/SmartPointers.hpp>

#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>

namespace Game::PathFinding
{
class SpatialHashGrid;
}

namespace Game::Sprites::Containers
{

//! @brief Batches the entire floor tile map into a single sf::VertexArray for efficient drawing.
class VertexFloor : public sf::Drawable, public sf::Transformable
{
public:
  //! @brief Construct an empty VertexFloor. Call create() to populate it.
  VertexFloor() = default;

  //! @brief Draw the sf::VertexArray to the render target (with optional state for shader).
  //! @param target The render target to draw to.
  //! @param states The render states to apply.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;

  //! @brief Remove all floor vertices, leaving the floor empty until create() is called again.
  void clear() { m_vertices.clear(); }

  //! @brief Grid-space offset of this floor's origin within the world.
  sf::Vector2u world_grid_offset{ 0, 0 };

  //! @brief Create the tile map using a single large sf::VertexArray.
  //! @param void_sm Spatial hash grid used to look up which tiles are present.
  //! @param scene_map Scene map providing tile/texture data for the floor.
  void create( const PathFinding::SpatialHashGrid &void_sm, const Scene::SceneMapSharedPtr &scene_map );

  //! @brief Remove the floor tile at the given world position from the vertex array.
  //! @param pos World-space position of the tile to remove.
  void remove( sf::Vector2f pos );

private:
  //! @brief Combined vertex geometry for every floor tile.
  sf::VertexArray m_vertices;
  //! @brief Texture used for the floor tile map.
  sf::Texture m_tileset;
};

} // namespace Game::Sprites::Containers

#endif // SRC_SPRITES_VERTEXFLOOR_HPP__
