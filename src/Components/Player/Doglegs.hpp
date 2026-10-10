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
  //! @brief A path from the player to a landmark, as a polyline: `points` holds the player, each turn and the
  //! landmark, in that order. An L-shaped "crow flies" path in the open, or a route along the passages in the crypt.
  struct Dogleg
  {
    std::vector<sf::Vector2f> points;
    sf::Color color;
    [[nodiscard]] float length() const
    {
      float total = 0.f;
      for ( std::size_t i = 0; i + 1 < points.size(); ++i )
        total += ( points[i + 1] - points[i] ).length();
      return total;
    }
  };
  //! @brief One path per landmark matching the dowsing rod target.
  std::vector<Dogleg> m_doglegs;
  //! @brief Triangle list of the chevron markers travelling along `m_doglegs`, in world coordinates.
  sf::VertexArray m_markers{ sf::PrimitiveType::Triangles };
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_DOGLEGS_HPP__
