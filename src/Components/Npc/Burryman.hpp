#ifndef SRC_COMPONENTS_NPC_BURRYMAN_HPP__
#define SRC_COMPONENTS_NPC_BURRYMAN_HPP__

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/entity.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <vector>

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Burryman NPC (a hostile NPC type).
//! @details The Burryman steals every world item he can reach, then flees the player. Only an axe or an arrow
//! kills him, dropping everything he carries back into the world.
struct Burryman
{
  //! @brief Scale the carried items are drawn at, round the Burryman's outer edge.
  static constexpr float kCarriedScale = 0.5f;

  //! @brief Offset from the Burryman's body centre to where carried item `index` of `count` is drawn.
  //! @details The items are spaced evenly round his outer edge, starting at the top and going clockwise.
  //! @param index The carried item's index.
  //! @param count The number of carried items.
  //! @param body_size The Burryman's on-screen size.
  //! @return sf::Vector2f The offset from the body centre.
  static sf::Vector2f carried_offset( std::size_t index, std::size_t count, sf::Vector2f body_size )
  {
    if ( count == 0 ) return { 0.f, 0.f };
    const float angle = ( static_cast<float>( index ) * 2.f * std::numbers::pi_v<float> / static_cast<float>( count ) ) -
                        ( std::numbers::pi_v<float> / 2.f );
    const sf::Vector2f dir( std::cos( angle ), std::sin( angle ) );
    const sf::Vector2f half = body_size / 2.f;
    // stretch the direction out to wherever it meets the edge of the body rectangle
    float t = std::numeric_limits<float>::max();
    if ( std::abs( dir.x ) > 0.0001f ) { t = std::min( t, half.x / std::abs( dir.x ) ); }
    if ( std::abs( dir.y ) > 0.0001f ) { t = std::min( t, half.y / std::abs( dir.y ) ); }
    return dir * t;
  }

  sf::Time sfx_cooldown{ sf::Time::Zero };

  //! @brief The world item entities the Burryman is carrying. They keep their own components, minus
  //! Cmp::Position, Cmp::AnimData and Cmp::ZOrderValue, which are restored when he drops them.
  std::vector<entt::entity> carried;
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_BURRYMAN_HPP__
