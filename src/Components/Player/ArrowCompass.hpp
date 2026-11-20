#ifndef SRC_COMPONENTS_PLAYER_ARROWCOMPASS_HPP__
#define SRC_COMPONENTS_PLAYER_ARROWCOMPASS_HPP__

#include <Components/Altar/Activated.hpp>
#include <Components/Altar/BloodCount.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/Crypt/Entrance.hpp>
#include <Components/Exit.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/registry.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>

namespace Game::Cmp::Player
{

//! @brief Caches the player's compass arrow state. The compass has several needles, each pointing at its own target:
//!        - an "exit" needle, active while an exit key exists anywhere in the scene (world item or inventory)
//!        - a "crypt" needle, active while a crypt key exists anywhere in the scene (world item or inventory)
//!        - a "held" needle, driven by the item in the player's inventory slot (altar / blood targets). Holding the
//!          exit key also shows this needle, as it can be offered at an altar for the crypt key
//!        Key needles are therefore shown whether or not the player holds the key, and turn off once the key is used
//!        (destroyed). Target resolution is throttled via `m_retarget_timer`, which PlayerSystem advances each frame.
//!        Not transferred between scenes (see RegistryTransfer) so a stale target never carries over.
struct ArrowCompass
{
  //! @brief What a needle is pointing at.
  //!        BLOOD points only at a graveyard altar still taking blood (fullest first, then nearest).
  enum class Mode { NONE, EXIT, CRYPT, ALTAR, BLOOD };

  //! @brief One compass arrow: its target type, label, colour and resolved target.
  struct Needle
  {
    Mode mode{ Mode::NONE };
    //! @brief Text drawn beside the arrow (empty = no label)
    std::string_view label;
    //! @brief Arrow sprite drawn for this needle
    Sys::SpriteKey sprite;
    //! @brief Fill colour of the label
    sf::Color color{ sf::Color::White };
    //! @brief Whether this needle is currently shown
    bool active{ false };
    std::optional<Cmp::Position> target;
  };

  //! @brief Where and how to draw the arrow sprite this frame
  struct Placement
  {
    sf::FloatRect rect;
    sf::Angle angle;
    sf::Vector2f scale;
    sf::Vector2f origin;
    //! @brief Unit vector from the player towards the target
    sf::Vector2f direction;
  };

  //! @brief Colour of the altar/blood needle's label (matches sprite.graveyard.arrowcompass.altar)
  static constexpr sf::Color kAltarColor{ 200, 30, 30 };
  //! @brief Colour of the exit key needle's label (matches sprite.graveyard.arrowcompass.exit)
  static constexpr sf::Color kExitColor{ 0, 200, 0 };
  //! @brief Colour of the crypt key needle's label (matches sprite.graveyard.arrowcompass.crypt)
  static constexpr sf::Color kCryptColor{ 20, 40, 140 };

  //! @brief Inset from the view edge at which the arrow is placed
  static constexpr float kEdgeMargin{ 32.f };

  //! @brief Scale of the view-centered region the target must enter before the arrow is hidden
  static constexpr float kTargetRegionScale{ 0.5f };

  //! @brief Recompute the held needle's mode if the inventory item type or its offering eligibility has changed since the last call.
  //!        The crypt key is ignored here: its needle is driven by the key's existence (see refresh_targets()). The exit
  //!        key also has its own needle, but is offerable, so it shows the altar needle too.
  //! @param item_type The item type currently in the player's inventory slot (empty if none)
  //! @param is_offerable Whether the inventory item can currently be offered at the altar (tools only once spent)
  //! @return true if the mode was recomputed (forces a retarget on the next refresh_targets())
  bool update_mode( const Sys::ItemKey &item_type, bool is_offerable )
  {
    if ( item_type == m_cached_item_type and is_offerable == m_cached_is_offerable ) return false;
    m_cached_item_type = item_type;
    m_cached_is_offerable = is_offerable;

    m_cached_is_blood = not item_type.empty() and Sys::ItemStore::instance().get( item_type ).is_blood();

    // the crypt key has its own needle; the (offerable) exit key falls through to point at the altar as well as the exit
    if ( item_type.contains( "cryptkey" ) )
      m_held.mode = Mode::NONE;
    else if ( m_cached_is_blood )
      m_held.mode = Mode::BLOOD;
    else if ( is_offerable or item_type.contains( "relic" ) )
      m_held.mode = Mode::ALTAR;
    else
      m_held.mode = Mode::NONE;

    m_held.active = m_held.mode != Mode::NONE;
    m_held.target.reset();
    m_retarget_pending = true;
    return true;
  }

  //! @brief Re-resolve which key needles are active and every active needle's target, if a retarget is pending or the
  //!        retarget interval has elapsed.
  //! @param reg The current scene registry
  //! @param player_center The player's center position, used for nearest-target searches
  void refresh_targets( entt::registry &reg, sf::Vector2f player_center )
  {
    if ( not m_retarget_pending and m_retarget_timer < m_retarget_interval ) return;
    m_retarget_pending = false;
    m_retarget_timer = sf::Time::Zero;

    m_exit.active = key_exists( reg, kExitKey );
    m_crypt.active = key_exists( reg, kCryptKey );

    for ( Needle *needle : needles() )
    {
      if ( needle->active )
        resolve( *needle, reg, player_center );
      else
        needle->target.reset();
    }
  }

  //! @brief Visit every active needle.
  //! @param fn Callable taking (const Needle &)
  template <typename Fn>
  void for_each_needle( Fn &&fn ) const
  {
    for ( const Needle *needle : needles() )
    {
      if ( needle->active and needle->mode != Mode::NONE ) fn( *needle );
    }
  }

  //! @brief Point on the view edge (inset by kEdgeMargin) along the ray from the player towards a target.
  //! @param target The target bounds
  //! @param player_center The player's center position
  //! @param world_view The current world view
  //! @param left_inset Extra inset (world units) from the view's left edge, so the point stays clear of the UI panels
  //! @return The edge point and the unit direction from the player to the target, or std::nullopt if the target is inside
  //!         the view-centered kTargetRegionScale region (i.e. the player can already see it)
  [[nodiscard]] static std::optional<std::pair<sf::Vector2f, sf::Vector2f>> edge_point( const sf::FloatRect &target, sf::Vector2f player_center,
                                                                                        const sf::View &world_view, float left_inset = 0.f )
  {
    // Only a reduced region centered in the view counts as "visible", so the indicator stays while the target is near
    // the screen edges or hidden behind the UI panels on the left.
    const auto target_region = Cmp::RectBounds::scaled( Utils::calculate_view_bounds( world_view ), kTargetRegionScale );
    if ( Utils::is_visible_in_view( target_region.getBounds(), target ) ) return std::nullopt;

    const sf::Vector2f offset = target.getCenter() - player_center;
    if ( offset.lengthSquared() == 0.f ) return std::nullopt;
    const sf::Vector2f direction = offset.normalized();

    // view bounds in world coordinates, inset by the edge margin (plus the UI panel width on the left)
    sf::FloatRect view_bounds = Utils::calculate_view_bounds( world_view );
    view_bounds.position += { kEdgeMargin + left_inset, kEdgeMargin };
    view_bounds.size -= { kEdgeMargin * 2.f + left_inset, kEdgeMargin * 2.f };

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

    sf::Vector2f point = player_center;
    if ( t < std::numeric_limits<float>::max() ) { point = player_center + direction * t; }
    return std::pair{ point, direction };
  }

  //! @brief Compute where to draw a needle's arrow: on the view edge (see edge_point()) along the ray from the player to the target.
  //! @param needle The needle to place
  //! @param player_center The player's center position
  //! @param world_view The current world view
  //! @param left_inset Extra inset (world units) from the view's left edge, so the arrow stays clear of the UI panels
  //! @return std::nullopt if there is no target or the target is inside the view-centered kTargetRegionScale region
  [[nodiscard]] std::optional<Placement> placement( const Needle &needle, sf::Vector2f player_center, const sf::View &world_view,
                                                    float left_inset = 0.f ) const
  {
    if ( not needle.target ) return std::nullopt;
    const auto edge = edge_point( *needle.target, player_center, world_view, left_inset );
    if ( not edge ) return std::nullopt;
    const auto &[arrow_position, direction] = *edge;

    // Map sin(time) from [-1, 1] to [min_scale, max_scale]
    const float sine = std::sin( m_freq * m_osc_timer.asSeconds() );
    const float oscillating_scale = m_min_scale + ( ( m_max_scale - m_min_scale ) * ( sine + 1.f ) / 2.f );

    const sf::Vector2f half_grid = Constants::kGridSizePxF / 2.f;
    return Placement{ .rect = { arrow_position - half_grid, Constants::kGridSizePxF },
                      .angle = Utils::Maths::angle( direction ).value_or( sf::Angle::Zero ),
                      .scale = { oscillating_scale, oscillating_scale },
                      .origin = half_grid,
                      .direction = direction };
  }

  //! @brief Accumulated time since the last retarget. Advanced by PlayerSystem::update().
  sf::Time m_retarget_timer{ sf::Time::Zero };

  //! @brief How often the targets (and key needle activity) are re-resolved
  sf::Time m_retarget_interval{ sf::milliseconds( 250 ) };

  //! @brief Time component of the sine wave for the arrow's pulsing scale
  sf::Time m_osc_timer;

private:
  inline static const Sys::ItemKey kExitKey{ "item.exitkey" };
  inline static const Sys::ItemKey kCryptKey{ "item.cryptkey" };

  [[nodiscard]] std::array<Needle *, 3> needles() { return { &m_exit, &m_crypt, &m_held }; }
  [[nodiscard]] std::array<const Needle *, 3> needles() const { return { &m_exit, &m_crypt, &m_held }; }

  //! @brief Whether the given key exists in the scene, either as a world item or in the player's inventory.
  //!        Keys are blast proof and only destroyed when used, so this is "the key has spawned and not been used".
  static bool key_exists( entt::registry &reg, const Sys::ItemKey &key )
  {
    for ( auto [item_entt, item_cmp] : reg.view<Cmp::WorldItem>().each() )
    {
      if ( item_cmp.item_type == key ) return true;
    }
    for ( auto [slot_entt, slot_cmp] : reg.view<Cmp::PlayerInventorySlot>().each() )
    {
      if ( not slot_cmp.empty() and slot_cmp.m_item->item_type == key ) return true;
    }
    return false;
  }

  //! @brief Resolve the needle's target for its mode.
  void resolve( Needle &needle, entt::registry &reg, sf::Vector2f player_center ) const
  {
    switch ( needle.mode )
    {
      case Mode::EXIT: {
        needle.target.reset();
        for ( auto [exit_entt, exit_cmp, exit_pos_cmp] : reg.view<Cmp::Exit, Cmp::Position>().each() )
        {
          needle.target = exit_pos_cmp;
        }
        break;
      }
      case Mode::CRYPT: {
        needle.target = nearest(
            reg.view<Cmp::Crypt::Entrance, Cmp::Position>(), player_center,
            []( entt::entity, const Cmp::Crypt::Entrance &crypt_cmp, const Cmp::Position &crypt_pos_cmp ) -> std::optional<Cmp::Position>
        {
          if ( crypt_cmp.is_open() ) return std::nullopt;
          return crypt_pos_cmp;
        } );
        break;
      }
      case Mode::ALTAR: {
        needle.target = nearest( reg.view<Cmp::Altar::MultiBlock>(), player_center,
                                 []( entt::entity, const Cmp::Altar::MultiBlock &altar_cmp ) -> std::optional<Cmp::Position>
        { return Cmp::Position( altar_cmp.position, altar_cmp.size ); } );
        break;
      }
      case Mode::BLOOD: {
        // mirrors AltarSystem::offering_player_blood(): once any altar is activated, no altar accepts blood. An altar activates
        // the moment its quota fills, so every other graveyard altar is still taking blood.
        needle.target.reset();
        if ( not reg.view<Cmp::Altar::Activated>().empty() ) break;

        int best_count = 0;
        float best_distance = std::numeric_limits<float>::max();
        for ( auto [altar_entt, altar_cmp, blood_cmp] : reg.view<Cmp::Altar::MultiBlock, Cmp::Altar::BloodCount>().each() )
        {
          if ( altar_cmp.get_kind() != Cmp::Altar::Kind::GRAVEYARD ) continue;
          // prefer the altar closest to completion, then the nearest
          const int count = blood_cmp.value();
          const float distance = Utils::Maths::getEuclideanDistance( altar_cmp.position, player_center );
          if ( needle.target and ( count < best_count or ( count == best_count and distance >= best_distance ) ) ) continue;
          best_count = count;
          best_distance = distance;
          needle.target = Cmp::Position( altar_cmp.position, altar_cmp.size );
        }
        break;
      }
      case Mode::NONE:
        needle.target.reset();
        break;
    }
  }

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

  Needle m_exit{ .mode = Mode::EXIT, .label = "exit", .sprite = "sprite.graveyard.arrowcompass.exit", .color = kExitColor };
  Needle m_crypt{ .mode = Mode::CRYPT, .label = "crypt", .sprite = "sprite.graveyard.arrowcompass.crypt", .color = kCryptColor };
  //! @brief Altar and blood targets share the altar arrow
  Needle m_held{ .label = "altar", .sprite = "sprite.graveyard.arrowcompass.altar", .color = kAltarColor };

  Sys::ItemKey m_cached_item_type;
  bool m_cached_is_offerable{ false };
  //! @brief Whether the cached inventory item is a blood vial, which guides the player back to an altar needing blood
  bool m_cached_is_blood{ false };
  bool m_retarget_pending{ true };

  //! @brief Frequency of the sine wave for the arrow's pulsing scale
  float m_freq{ 4.f };
  //! @brief Min scale of the arrow pulse
  float m_min_scale{ 0.5f };
  //! @brief Max scale of the arrow pulse
  float m_max_scale{ 1.5f };
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_ARROWCOMPASS_HPP__
