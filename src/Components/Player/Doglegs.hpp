#ifndef SRC_COMPONENTS_PLAYER_DOGLEGS_HPP__
#define SRC_COMPONENTS_PLAYER_DOGLEGS_HPP__

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

#include <vector>

namespace Game::Cmp::Player
{

//! @brief Scene-local cache of the dowsing rod guide paths and their chevron markers. Rebuilt every frame by
//! DoglegSystem and drawn by RenderGameSystem. Not transferred between scenes.
class Doglegs
{
public:
  //! @brief An L-shaped path from `source` to `target`, turning once at `corner`.
  struct Dogleg
  {
    sf::Vector2f source;
    sf::Vector2f corner;
    sf::Vector2f target;
    sf::Color color;
    [[nodiscard]] float length() const { return ( corner - source ).length() + ( target - corner ).length(); }
  };
  //! @brief One path per landmark matching the dowsing rod target.
  std::vector<Dogleg> m_doglegs;
  //! @brief Triangle list of the chevron markers travelling along `m_doglegs`, in world coordinates.
  sf::VertexArray m_markers{ sf::PrimitiveType::Triangles };
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_DOGLEGS_HPP__
