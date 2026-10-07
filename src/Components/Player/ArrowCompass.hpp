#ifndef SRC_COMPONENTS_PLAYER_ARROWCOMPASS_HPP__
#define SRC_COMPONENTS_PLAYER_ARROWCOMPASS_HPP__

#include <Components/Altar/MultiBlock.hpp>
#include <Components/Crypt/Entrance.hpp>
#include <Components/Exit.hpp>
#include <Components/Position.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/registry.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <tuple>

namespace Game::Cmp::Player
{

//! @brief Caches the player's compass arrow state: what is being pointed at (driven by the inventory item) and the
//!        resolved target position. Target resolution is throttled via `m_retarget_timer`, which PlayerSystem advances
//!        each frame. Not transferred between scenes (see RegistryTransfer) so a stale target never carries over.
struct ArrowCompass
{
  //! @brief What the compass is pointing at, derived from the player's inventory item type
  enum class Mode { NONE, EXIT, CRYPT, ALTAR };

  //! @brief Where and how to draw the arrow sprite this frame
  struct Placement
  {
    sf::FloatRect rect;
    sf::Angle angle;
    sf::Vector2f scale;
    sf::Vector2f origin;
  };

  inline static const Sys::SpriteKey kSpriteType{ "sprite.graveyard.arrow" };

  //! @brief Inset from the view edge at which the arrow is placed
  static constexpr float kEdgeMargin{ 32.f };

  //! @brief Recompute the mode if the inventory item type has changed since the last call.
  //! @param item_type The item type currently in the player's inventory slot (empty if none)
  //! @return true if the mode was recomputed (forces a retarget on the next refresh_target())
  bool update_mode( const Sys::ItemKey &item_type )
  {
    if ( item_type == m_cached_item_type ) return false;
    m_cached_item_type = item_type;

    // precedence matches the original render order: relic > cryptkey > exitkey
    if ( item_type.contains( "relic" ) or item_type.contains( ".drop" ) or item_type.contains( ".forage" ) )
      m_mode = Mode::ALTAR;
    else if ( item_type.contains( "cryptkey" ) )
      m_mode = Mode::CRYPT;
    else if ( item_type.contains( "exitkey" ) )
      m_mode = Mode::EXIT;
    else
      m_mode = Mode::NONE;

    m_target.reset();
    m_retarget_pending = true;
    return true;
  }

  [[nodiscard]] Mode mode() const { return m_mode; }

  //! @brief Re-resolve `m_target` if the mode changed or the retarget interval has elapsed.
  //! @param reg The current scene registry
  //! @param player_center The player's center position, used for nearest-target searches
  void refresh_target( entt::registry &reg, sf::Vector2f player_center )
  {
    if ( not m_retarget_pending and m_retarget_timer < m_retarget_interval ) return;
    m_retarget_pending = false;
    m_retarget_timer = sf::Time::Zero;

    switch ( m_mode )
    {
      case Mode::EXIT: {
        m_target.reset();
        for ( auto [exit_entt, exit_cmp, exit_pos_cmp] : reg.view<Cmp::Exit, Cmp::Position>().each() )
        {
          m_target = exit_pos_cmp;
        }
        break;
      }
      case Mode::CRYPT: {
        m_target = nearest(
            reg.view<Cmp::Crypt::Entrance, Cmp::Position>(), player_center,
            []( entt::entity, const Cmp::Crypt::Entrance &crypt_cmp, const Cmp::Position &crypt_pos_cmp ) -> std::optional<Cmp::Position>
        {
          if ( crypt_cmp.is_open() ) return std::nullopt;
          return crypt_pos_cmp;
        } );
        break;
      }
      case Mode::ALTAR: {
        m_target = nearest( reg.view<Cmp::Altar::MultiBlock>(), player_center,
                            []( entt::entity, const Cmp::Altar::MultiBlock &altar_cmp ) -> std::optional<Cmp::Position>
        {
          if ( altar_cmp.is_exitkey_lockout() ) return std::nullopt;
          return Cmp::Position( altar_cmp.position, altar_cmp.size );
        } );
        break;
      }
      case Mode::NONE:
        m_target.reset();
        break;
    }
  }

  //! @brief Compute where to draw the arrow: on the view edge (inset by kEdgeMargin) along the ray from the player to the target.
  //! @param player_center The player's center position
  //! @param world_view The current world view
  //! @return std::nullopt if there is no target or the target is already visible in the view
  [[nodiscard]] std::optional<Placement> placement( sf::Vector2f player_center, const sf::View &world_view ) const
  {
    if ( not m_target ) return std::nullopt;

    // dont show the compass arrow if the target is on-screen....we can see it
    if ( Utils::is_visible_in_view( world_view, *m_target ) ) return std::nullopt;

    const sf::Vector2f direction = ( m_target->getCenter() - player_center ).normalized();

    // view bounds in world coordinates, inset by the edge margin
    sf::FloatRect view_bounds = Utils::calculate_view_bounds( world_view );
    view_bounds.position += { kEdgeMargin, kEdgeMargin };
    view_bounds.size -= { kEdgeMargin * 2.f, kEdgeMargin * 2.f };

    // distances along the ray to each edge
    const float t_left = ( view_bounds.position.x - player_center.x ) / direction.x;
    const float t_right = ( view_bounds.position.x + view_bounds.size.x - player_center.x ) / direction.x;
    const float t_top = ( view_bounds.position.y - player_center.y ) / direction.y;
    const float t_bottom = ( view_bounds.position.y + view_bounds.size.y - player_center.y ) / direction.y;

    // smallest positive t is the closest edge intersection
    float t = std::numeric_limits<float>::max();
    if ( t_left > 0 ) t = std::min( t, t_left );
    if ( t_right > 0 ) t = std::min( t, t_right );
    if ( t_top > 0 ) t = std::min( t, t_top );
    if ( t_bottom > 0 ) t = std::min( t, t_bottom );

    sf::Vector2f arrow_position = player_center;
    if ( t < std::numeric_limits<float>::max() ) { arrow_position = player_center + direction * t; }

    // Map sin(time) from [-1, 1] to [min_scale, max_scale]
    const float sine = std::sin( m_freq * m_osc_clock.getElapsedTime().asSeconds() );
    const float oscillating_scale = m_min_scale + ( ( m_max_scale - m_min_scale ) * ( sine + 1.f ) / 2.f );

    const sf::Vector2f half_grid = Constants::kGridSizePxF / 2.f;
    return Placement{ .rect = { arrow_position - half_grid, Constants::kGridSizePxF },
                      .angle = Utils::Maths::angle( direction ).value_or( sf::Angle::Zero ),
                      .scale = { oscillating_scale, oscillating_scale },
                      .origin = half_grid };
  }

  //! @brief Accumulated time since the last retarget. Advanced by PlayerSystem::update().
  sf::Time m_retarget_timer{ sf::Time::Zero };

  //! @brief How often the target is re-resolved (nearest crypt/altar depends on player position)
  sf::Time m_retarget_interval{ sf::milliseconds( 250 ) };

private:
  //! @brief Allocation-free nearest-position search over an entt view, skipping entities `to_position` rejects.
  //! @param view The entt view to search
  //! @param from The point to measure distance from
  //! @param to_position Callable taking (entt::entity, const Components&...) returning std::optional<Cmp::Position>
  //! @return The nearest matched Cmp::Position, or std::nullopt if no entity qualified
  template <typename View, typename ToPositionFn>
  static std::optional<Cmp::Position> nearest( View &&view, sf::Vector2f from, ToPositionFn &&to_position )
  {
    std::optional<Cmp::Position> best;
    float best_distance = std::numeric_limits<float>::max();
    for ( auto &&row : view.each() )
    {
      std::optional<Cmp::Position> maybe_pos = std::apply( to_position, row );
      if ( not maybe_pos ) continue;
      const float distance = Utils::Maths::getEuclideanDistance( maybe_pos->position, from );
      if ( distance < best_distance )
      {
        best_distance = distance;
        best = maybe_pos;
      }
    }
    return best;
  }

  Mode m_mode{ Mode::NONE };
  Sys::ItemKey m_cached_item_type;
  std::optional<Cmp::Position> m_target;
  bool m_retarget_pending{ true };

  //! @brief Time component of the sine wave for the arrow's pulsing scale
  sf::Clock m_osc_clock;
  //! @brief Frequency of the sine wave for the arrow's pulsing scale
  float m_freq{ 4.f };
  //! @brief Min scale of the arrow pulse
  float m_min_scale{ 0.5f };
  //! @brief Max scale of the arrow pulse
  float m_max_scale{ 1.5f };
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_ARROWCOMPASS_HPP__
