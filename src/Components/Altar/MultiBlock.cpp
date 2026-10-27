#include <Components/Altar/MultiBlock.hpp>

namespace Game::Cmp::Altar
{

// clang-format off
const std::array<sf::Vector2f, 4> MultiBlock::flame_offsets = { 
  sf::Vector2f( 24.f, 10.f ), 
  sf::Vector2f( 56.f, 10.f ), 
  sf::Vector2f( 24.f, 33.f ),
  sf::Vector2f( 56.f, 33.f ) 
};
// clang-format on

} // namespace Game::Cmp::Altar
