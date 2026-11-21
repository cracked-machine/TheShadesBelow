#ifndef SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEMIMPL_HPP__
#define SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEMIMPL_HPP__

#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/Exit.hpp>
#include <Components/Hazard/CorruptionCell.hpp>
#include <Components/Hazard/SinkholeCorners.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Loot.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Npc/Container.hpp>
#include <Components/Npc/Crow.hpp>
#include <Components/Npc/ElderMother.hpp>
#include <Components/Npc/Ghost.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/HazardBalance.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Wall.hpp>
#include <Components/WorldTile.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/DestroyLootEvent.hpp>
#include <Events/DestroyNpcEvent.hpp>
#include <Events/DestroyitemEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlantFactory.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Threats/HazardFieldSystem.hpp>
#include <Systems/Threats/HazardTraits.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <random>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Game::Sys
{

namespace Detail
{
//! @brief Always false, but depends on T so static_assert(dependent_false<T>) only fires when the
//! enclosing `if constexpr` branch is actually instantiated, not for every instantiation of the template.
template <typename T>
inline constexpr bool dependent_false = false;

//! @brief Bit flags for a sinkhole cell's open sides - a set bit means the neighbour on that side is also a sinkhole.
enum SinkholeSide : unsigned { kNorth = 1u, kSouth = 2u, kEast = 4u, kWest = 8u };

//! @brief Sinkhole edge sprite keys indexed by SinkholeSide bitmask (N=1, S=2, E=4, W=8).
inline constexpr std::array<std::string_view, 16> kSinkholeSprites{
    "sprite.graveyard.hazard.sinkhole_all_sides",           // 0
    "sprite.graveyard.hazard.sinkhole_no_north",            // N
    "sprite.graveyard.hazard.sinkhole_no_south",            // S
    "sprite.graveyard.hazard.sinkhole_no_north_south",      // N|S
    "sprite.graveyard.hazard.sinkhole_no_east",             // E
    "sprite.graveyard.hazard.sinkhole_no_east_north",       // N|E
    "sprite.graveyard.hazard.sinkhole_no_south_east",       // S|E
    "sprite.graveyard.hazard.sinkhole_no_north_south_east", // N|S|E
    "sprite.graveyard.hazard.sinkhole_no_west",             // W
    "sprite.graveyard.hazard.sinkhole_no_west_north",       // N|W
    "sprite.graveyard.hazard.sinkhole_no_south_west",       // S|W
    "sprite.graveyard.hazard.sinkhole_no_north_south_west", // N|S|W
    "sprite.graveyard.hazard.sinkhole_no_east_west",        // E|W
    "sprite.graveyard.hazard.sinkhole_no_north_east_west",  // N|E|W
    "sprite.graveyard.hazard.sinkhole_no_south_east_west",  // S|E|W
    "sprite.graveyard.hazard.sinkhole_no_sides",            // N|S|E|W
};

//! @brief Cardinal grid offsets paired with the SinkholeSide bit they set.
inline const std::array<std::pair<sf::Vector2f, unsigned>, 4> kSinkholeNeighbourOffsets{ {
    { { 0.f, -Constants::kGridSizePxF.y }, kNorth },
    { { 0.f, Constants::kGridSizePxF.y }, kSouth },
    { { Constants::kGridSizePxF.x, 0.f }, kEast },
    { { -Constants::kGridSizePxF.x, 0.f }, kWest },
} };

//! @brief Diagonal grid offsets, each paired with the two SinkholeSide bits that must be open for an inner
//! corner and the Cmp::Hazard::SinkholeCorners bit set when the diagonal itself is not a sinkhole.
struct SinkholeDiagonal
{
  sf::Vector2f offset;
  unsigned sides;
  uint8_t corner;
};
inline const std::array<SinkholeDiagonal, 4> kSinkholeDiagonalOffsets{ {
    { { Constants::kGridSizePxF.x, -Constants::kGridSizePxF.y }, kNorth | kEast, Cmp::Hazard::SinkholeCorners::kNorthEast },
    { { -Constants::kGridSizePxF.x, -Constants::kGridSizePxF.y }, kNorth | kWest, Cmp::Hazard::SinkholeCorners::kNorthWest },
    { { Constants::kGridSizePxF.x, Constants::kGridSizePxF.y }, kSouth | kEast, Cmp::Hazard::SinkholeCorners::kSouthEast },
    { { -Constants::kGridSizePxF.x, Constants::kGridSizePxF.y }, kSouth | kWest, Cmp::Hazard::SinkholeCorners::kSouthWest },
} };
//! @brief The components that keep a world position out of HazardFieldSystem::candidate_view(); shared with
//! HazardFieldSystem::in_candidate_view() so the view and the grid lookup can't drift apart.
template <typename HazardType, typename ExcludeHazard>
using CandidateExclude = entt::exclude_t<Cmp::ObstacleCap, Cmp::Wall, Cmp::LootContainer, Cmp::Npc::Container, HazardType, ExcludeHazard>;

//! @brief True if `entity` has any of the components in an entt::exclude_t list.
template <typename... Excluded>
bool has_any_excluded( const entt::registry &reg, entt::entity entity, entt::exclude_t<Excluded...> )
{
  return reg.any_of<Excluded...>( entity );
}

} // namespace Detail

//! @copydoc HazardFieldSystem::HazardFieldSystem(entt::registry&,sf::RenderWindow&,Audio::SoundBank&)
template <ValidHazard HazardType>
HazardFieldSystem<HazardType>::HazardFieldSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : Sys::BaseSystem( reg, window, sound_bank )
{
}

//! @copydoc HazardFieldSystem::update()
template <ValidHazard HazardType>
sf::Vector2f HazardFieldSystem<HazardType>::update( sf::Time dt )
{
  auto add_hazard_cell = update_hazard_field( dt );
  check_npc_hazard_field_collision();

  m_dmg_timer += dt;
  static constexpr sf::Time dmg_timeout = sf::seconds( 0.2f );
  if ( m_dmg_timer > dmg_timeout )
  {
    if ( Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled ) { check_player_hazard_field_collision(); }
    m_dmg_timer = sf::Time::Zero;
  }

  return add_hazard_cell;
}

//! @copydoc HazardFieldSystem::candidate_view()
template <ValidHazard HazardType>
auto HazardFieldSystem<HazardType>::candidate_view()
{
  // every world tile (Cmp::WorldTile) is a candidate. Obstacle caps sit over a neighbouring world position and are
  // removed implicitly along with their obstacle by Factory::Obstacle::remove_obstacle(); loot/NPC containers and walls
  // are never swallowed. Multiblocks, the exit, the spawn area and world items are excluded via reserved_sm by the callers.
  return reg().template view<Cmp::WorldTile, Cmp::Position>( Detail::CandidateExclude<HazardType, typename Traits::ExcludeHazard>{} );
}

//! @copydoc HazardFieldSystem::in_candidate_view()
template <ValidHazard HazardType>
bool HazardFieldSystem<HazardType>::in_candidate_view( entt::entity entity )
{
  return reg().valid( entity ) and reg().template all_of<Cmp::WorldTile, Cmp::Position>( entity ) and
         not Detail::has_any_excluded( reg(), entity, Detail::CandidateExclude<HazardType, typename Traits::ExcludeHazard>{} );
}

//! @copydoc HazardFieldSystem::is_candidate()
template <ValidHazard HazardType>
bool HazardFieldSystem<HazardType>::is_candidate( entt::entity entity )
{
  // obstacles: only the main block (caps are handled by candidate_view()); open floor: only if this hazard type spreads onto it
  if ( reg().template all_of<Cmp::Obstacle>( entity ) )
  {
    const auto *anim_cmp = reg().template try_get<Cmp::AnimData>( entity );
    return anim_cmp and anim_cmp->m_sprite_type.contains( ".main" );
  }
  return Traits::spreads_to_open_floor;
}

//! @copydoc HazardFieldSystem::init_hazard_field()
template <ValidHazard HazardType>
std::vector<sf::Vector2f> HazardFieldSystem<HazardType>::init_hazard_field( std::span<const std::vector<sf::Vector2f>> candidate_sets )
{
  // keep the field index in sync with the registry: re-entering the scene keeps existing cells,
  // a new level starts with none
  rebuild_field_sm();

  std::vector<sf::Vector2f> seeded_positions;
  auto hazard_field_view = reg().template view<HazardType>( entt::exclude<typename Traits::ExcludeHazard> );
  if ( hazard_field_view.size_hint() > 0 ) { return seeded_positions; }

  auto reserved_sm = m_reserved_sm.lock();
  const unsigned long seed = Sys::PersistSystem::get<typename Traits::SeedType>( reg() ).get_value();
  std::mt19937 rng( seed > 0 ? static_cast<std::mt19937::result_type>( seed ) : std::random_device{}() );

  // world tiles (Cmp::WorldTile) that the field may start on, indexed by grid cell - the same rules as spreading
  const auto cell_key = []( sf::Vector2f p )
  {
    return ( static_cast<std::int64_t>( std::floor( p.x / Constants::kGridSizePxF.x ) ) << 32 ) bitor
           static_cast<std::uint32_t>( static_cast<std::int32_t>( std::floor( p.y / Constants::kGridSizePxF.y ) ) );
  };
  std::unordered_map<std::int64_t, entt::entity> seed_cells;
  auto seed_view = candidate_view();
  for ( auto entity : seed_view )
  {
    if ( is_candidate( entity ) ) { seed_cells.emplace( cell_key( seed_view.template get<Cmp::Position>( entity ).position ), entity ); }
  }

  for ( const auto &candidate_set : candidate_sets )
  {
    std::vector<sf::Vector2f> candidates( candidate_set.begin(), candidate_set.end() );
    std::ranges::shuffle( candidates, rng );

    bool seeded = false;
    for ( const auto &candidate : candidates )
    {
      const Cmp::Position candidate_pos( candidate, Constants::kGridSizePxF );
      if ( reserved_sm and not reserved_sm->at( candidate_pos ).empty() ) continue;
      if ( holds_key_item( candidate_pos ) ) continue;

      const auto it = seed_cells.find( cell_key( candidate ) );
      if ( it == seed_cells.end() ) continue;
      const auto entity = it->second;
      // a cell seeded by an earlier set in this call is now a hazard cell
      if ( reg().template any_of<HazardType, typename Traits::ExcludeHazard>( entity ) ) continue;

      // copy: promote_to_hazard_cell() may swap-and-pop the Cmp::Position storage
      const Cmp::Position pos = reg().template get<Cmp::Position>( entity );
      promote_to_hazard_cell( entity, pos, reserved_sm );
      SPDLOG_INFO( "Added {} to position [{}, {}].", std::string( Traits::sprite_type ), pos.position.x, pos.position.y );
      seeded_positions.push_back( pos.position );
      seeded = true;
      break;
    }
    if ( not seeded ) { SPDLOG_WARN( "No valid position found to seed {} from {} candidates", std::string( Traits::sprite_type ), candidates.size() ); }
  }

  return seeded_positions;
}

//! @copydoc HazardFieldSystem::promote_to_hazard_cell()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::promote_to_hazard_cell( entt::entity entity, const Cmp::Position pos,
                                                            const PathFinding::SpatialHashGridSharedPtr &reserved_sm )
{
  Factory::Obstacle::remove_obstacle( reg(), entity, Factory::Obstacle::DeleteExtras::Yes, reserved_sm );
  // grass doesn't reserve its cell, so a hazard can seed or spread onto it: the hazard replaces it
  Factory::Plant::destroy_grass_in( reg(), pos );
  // index the hazard cell; BombSystem reads this to only mark (not arm) hazard cells
  if ( auto hazard_sm = m_hazard_sm.lock() ) { hazard_sm->insert( entity, pos ); }
  // open floor is in the NPC navmesh; drop the whole bucket like Cmp::Npc::NoPathFinding does at navmesh build time
  if ( auto npc_navmesh = m_npc_navmesh.lock() ) { npc_navmesh->remove_all( pos ); }
  // clang-format off
  reg().template emplace_or_replace<HazardType>( entity );
  reg().template emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{
        .sprite_type =  Sys::SpriteKey( std::string( Traits::sprite_type ) ),
        .enabled = true
  });
  // clang-format on
  reg().template emplace_or_replace<Cmp::ZOrderValue>( entity, pos.position.y - 1.f );
  reg().template emplace_or_replace<Cmp::Npc::NoPathFinding>( entity );

  m_field_sm.insert( entity, pos );
  if constexpr ( std::is_same_v<HazardType, Cmp::SinkholeCell> ) { retile_sinkhole_cell_and_neighbours( entity, pos ); }

  // the field swallows any dropped loot on this cell. at() returns a copy, so LootSystem removing the
  // loot from loot_sm while handling the event can't disturb this loop.
  if ( auto loot_sm = m_loot_sm.lock() )
  {
    for ( auto loot_entt : loot_sm->at( pos ) )
    {
      if ( not reg().valid( loot_entt ) or not reg().template all_of<Cmp::Loot>( loot_entt ) ) continue;
      get_systems_event_queue().trigger( Events::DestroyLootEvent( loot_entt ) );
    }
  }

  // ...and any dropped world item. Keys never get here: holds_key_item() keeps the field off their cells.
  if ( auto item_sm = m_item_sm.lock() )
  {
    for ( auto item_entt : item_sm->at( pos ) )
    {
      if ( not reg().valid( item_entt ) or not reg().template all_of<Cmp::WorldItem>( item_entt ) ) continue;
      get_systems_event_queue().trigger( Events::DestroyItemEvent( item_entt ) );
    }
  }
}

//! @copydoc HazardFieldSystem::holds_key_item()
template <ValidHazard HazardType>
bool HazardFieldSystem<HazardType>::holds_key_item( const Cmp::Position &pos )
{
  const auto item_sm = m_item_sm.lock();
  if ( not item_sm ) return false;
  return std::ranges::any_of( item_sm->at( pos ),
                              [&]( entt::entity item_entt )
                              {
                                if ( not reg().valid( item_entt ) ) return false;
                                const auto *item_cmp = reg().template try_get<Cmp::WorldItem>( item_entt );
                                return item_cmp and ( item_cmp->item_type == "item.exitkey" or item_cmp->item_type == "item.cryptkey" );
                              } );
}

//! @copydoc HazardFieldSystem::rebuild_field_sm()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::rebuild_field_sm()
{
  m_field_sm.clear();
  for ( auto [entity, hazard_cmp, pos_cmp] : reg().template view<HazardType, Cmp::Position>().each() )
  {
    m_field_sm.insert( entity, pos_cmp );
  }
}

//! @copydoc HazardFieldSystem::has_hazard_at()
template <ValidHazard HazardType>
bool HazardFieldSystem<HazardType>::has_hazard_at( const Cmp::Position &pos, sf::Vector2f offset ) const
{
  for ( auto entity : m_field_sm.at( Cmp::Position( pos.position + offset, Constants::kGridSizePxF ) ) )
  {
    if ( reg().valid( entity ) and reg().template all_of<HazardType>( entity ) ) return true;
  }
  return false;
}

//! @copydoc HazardFieldSystem::update_sinkhole_sprite()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::update_sinkhole_sprite( entt::entity entity, const Cmp::Position &pos )
{
  unsigned mask = 0;
  for ( const auto &[offset, side] : Detail::kSinkholeNeighbourOffsets )
  {
    if ( has_hazard_at( pos, offset ) ) mask |= side;
  }

  uint8_t corner_mask = 0;
  for ( const auto &diagonal : Detail::kSinkholeDiagonalOffsets )
  {
    if ( ( mask & diagonal.sides ) == diagonal.sides and not has_hazard_at( pos, diagonal.offset ) ) corner_mask |= diagonal.corner;
  }
  if ( corner_mask != 0 ) { reg().template emplace_or_replace<Cmp::Hazard::SinkholeCorners>( entity, corner_mask ); }
  else { reg().template remove<Cmp::Hazard::SinkholeCorners>( entity ); }

  if ( auto *anim_cmp = reg().template try_get<Cmp::AnimData>( entity ) )
  {
    anim_cmp->m_sprite_type = Sys::SpriteKey( std::string( Detail::kSinkholeSprites[mask] ) );
  }
}

//! @copydoc HazardFieldSystem::retile_sinkhole_cell_and_neighbours()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::retile_sinkhole_cell_and_neighbours( entt::entity entity, const Cmp::Position &pos )
{
  update_sinkhole_sprite( entity, pos );

  auto retile_at = [&]( sf::Vector2f offset )
  {
    for ( auto neighbour : m_field_sm.at( Cmp::Position( pos.position + offset, Constants::kGridSizePxF ) ) )
    {
      if ( neighbour == entity or not reg().valid( neighbour ) ) continue;
      const auto *neighbour_pos = reg().template try_get<Cmp::Position>( neighbour );
      if ( not neighbour_pos or not reg().template all_of<HazardType>( neighbour ) ) continue;
      update_sinkhole_sprite( neighbour, *neighbour_pos );
    }
  };
  // side neighbours gain an open edge; diagonal neighbours may lose an inner corner
  for ( const auto &[offset, side] : Detail::kSinkholeNeighbourOffsets ) retile_at( offset );
  for ( const auto &diagonal : Detail::kSinkholeDiagonalOffsets ) retile_at( diagonal.offset );
}

//! @copydoc HazardFieldSystem::update_hazard_field()
template <ValidHazard HazardType>
sf::Vector2f HazardFieldSystem<HazardType>::update_hazard_field( sf::Time dt )
{
  m_spread_update_timer += dt;
  const sf::Time spread_period = sf::seconds( Sys::PersistSystem::get<typename Traits::SpreadPeriodType>( reg() ).get_value() );
  if ( m_spread_update_timer < spread_period ) return {};
  m_spread_update_timer = sf::Time::Zero;

  auto reserved_sm = m_reserved_sm.lock();
  auto world_tile_sm = m_world_tile_sm.lock();
  auto hazard_view = reg().template view<HazardType, Cmp::Position>();
  auto candidate_view = this->candidate_view();

  Cmp::RandomInt hazard_spread_picker( 0, Traits::odds ); // 1-in-(odds + 1) chance per eligible adjacent cell

  // reused per hazard cell: the 3x3 neighbourhood from the world-tile / field grids
  std::vector<entt::entity> nearby_entts;

  for ( auto [hazard_entity, hazard_cmp, position_cmp] : hazard_view.each() )
  {
    SPDLOG_DEBUG( "Hazard {} active: {}", static_cast<uint32_t>( hazard_entity ), hazard_cmp.active );
    if ( not hazard_cmp.active ) continue;

    // make the hazard field hitbox slightly larger to find adjacent obstacles
    auto hazard_hitbox = sf::FloatRect( position_cmp.position - Constants::kGridSizePxF, Constants::kGridSizePxF * 3.f );
    int adjacent_hazard_fields = 0;

    // Roll for one candidate; returns true if it was promoted to a new hazard cell.
    const auto try_spread_to = [&]( entt::entity candidate_entity, const Cmp::Position &candidate_pos_cmp ) -> bool
    {
      if ( not hazard_hitbox.findIntersection( candidate_pos_cmp ) ) return false;
      if ( reserved_sm and not reserved_sm->at( candidate_pos_cmp ).empty() ) return false;
      if ( holds_key_item( candidate_pos_cmp ) ) return false;
      if ( not is_candidate( candidate_entity ) ) return false;
      SPDLOG_DEBUG( "Hazard candidate cell {}", static_cast<uint32_t>( candidate_entity ) );

      auto hazard_pick = hazard_spread_picker.gen();
      SPDLOG_DEBUG( "hazard_pick:{}", hazard_pick );
      return hazard_pick == 0;
    };

    // add new hazard cell
    if ( world_tile_sm )
    {
      // only the 3x3 cells around this hazard cell, shuffled so no direction is favoured by bucket order
      nearby_entts = world_tile_sm->neighbours( position_cmp );
      std::ranges::shuffle( nearby_entts, hazard_spread_picker );
      for ( auto candidate_entity : nearby_entts )
      {
        if ( not in_candidate_view( candidate_entity ) ) continue;
        // copy: promote_to_hazard_cell() can swap-and-pop the Cmp::Position storage (see its doc)
        const Cmp::Position candidate_pos_cmp = reg().template get<Cmp::Position>( candidate_entity );
        if ( not try_spread_to( candidate_entity, candidate_pos_cmp ) ) continue;
        promote_to_hazard_cell( candidate_entity, candidate_pos_cmp, reserved_sm );
        SPDLOG_DEBUG( "New hazard field created at entity {}", static_cast<uint32_t>( candidate_entity ) );
        return candidate_pos_cmp.position; // only add one hazard cell per update period
      }
    }
    else
    {
      for ( auto candidate_entity : candidate_view )
      {
        const auto &candidate_pos_cmp = candidate_view.template get<Cmp::Position>( candidate_entity );
        if ( not try_spread_to( candidate_entity, candidate_pos_cmp ) ) continue;
        // copy: promote_to_hazard_cell() can swap-and-pop the Cmp::Position storage (see its doc)
        const Cmp::Position new_cell_pos = candidate_pos_cmp;
        promote_to_hazard_cell( candidate_entity, new_cell_pos, reserved_sm );
        SPDLOG_DEBUG( "New hazard field created at entity {}", static_cast<uint32_t>( candidate_entity ) );
        return new_cell_pos.position; // only add one hazard cell per update period
      }
    }

    // update adjacent hazard cell count from this system's own hazard-cell grid (3x3 around this cell),
    // instead of comparing against every hazard cell. De-duplicate in case an entity was inserted twice.
    nearby_entts = m_field_sm.neighbours( position_cmp );
    std::ranges::sort( nearby_entts );
    const auto duplicates = std::ranges::unique( nearby_entts );
    nearby_entts.erase( duplicates.begin(), duplicates.end() );
    for ( auto adj_hazard_entity : nearby_entts )
    {
      if ( hazard_entity == adj_hazard_entity or not reg().valid( adj_hazard_entity ) ) continue;
      if ( not reg().template all_of<HazardType>( adj_hazard_entity ) ) continue;
      const auto *adj_pos_cmp = reg().template try_get<Cmp::Position>( adj_hazard_entity );
      if ( adj_pos_cmp and hazard_hitbox.findIntersection( *adj_pos_cmp ) ) { adjacent_hazard_fields++; }
    }

    // if the hazard field is surrounded by hazard fields, then we can exclude it from future
    // searches
    SPDLOG_DEBUG( "Hazard {} adjacent count: {}", static_cast<uint32_t>( hazard_entity ), adjacent_hazard_fields );
    if ( adjacent_hazard_fields >= 4 ) { hazard_cmp.active = false; }
  }
  return {};
}

//! @copydoc HazardFieldSystem::check_player_hazard_field_collision()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::check_player_hazard_field_collision()
{
  auto hazard_view = reg().template view<HazardType, Cmp::Position>();
  auto player_view = reg().template view<Cmp::Player::Character, Cmp::PlayerStats, Cmp::Player::Mortality, const Cmp::Position>();
  const auto &player_position = Utils::Player::get_position( reg() );
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [pc_entt, player_cmp, player_stats_cmp, player_mort_cmp, player_pos_cmp] : player_view.each() )
  {
    if ( not Utils::is_visible_in_view( view_bounds, player_pos_cmp ) ) continue;

    // dont spam death events if the player is already dead
    if ( player_mort_cmp.state == Cmp::Player::Mortality::State::DEAD ) continue;

    if constexpr ( Traits::sprite_type == "sprite.graveyard.hazard.corruption" )
    {
      // default to the corruption-field footstep SFX; a hazard hit below overrides it with MUD
      reg().template emplace_or_replace<Cmp::Player::Footstep>( Utils::Player::get_entity( m_reg ), Cmp::Player::Footstep::Type::GRASS );
    }

    // flying over hazards - no falling/decay/damage
    if ( Utils::Player::is_flying( reg() ) ) continue;

    // balancing over a lethal hazard's edge: PlayerSystem::update_hazard_balance() decides whether they fall into that
    // cell, but a new cell opening under them still kills them below
    [[maybe_unused]] const auto *balance_cmp = reg().template try_get<Cmp::Player::HazardBalance>( pc_entt );

    for ( auto [hazard_entt, hazard_cmp, hazard_pos_cmp] : hazard_view.each() )
    {

      if constexpr ( Traits::sprite_type == "sprite.graveyard.hazard.sinkhole" )
      {
        if ( balance_cmp and balance_cmp->hazard == hazard_entt ) continue;
        // centre probe, matching PlayerSystem's balance window (Cmp::Player::HazardBalance) which holds the player half over the edge
        if ( Utils::Collision::lethal_hazard_probe( reg(), player_pos_cmp ).findIntersection( hazard_pos_cmp ) )
        {
          // trigger death animation
          get_systems_event_queue().trigger( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::FALLING, player_position ) );
          return;
        }
      }
      else if constexpr ( Traits::sprite_type == "sprite.graveyard.hazard.corruption" )
      {
        // normal size hitbox for corruption for full area
        if ( hazard_pos_cmp.findIntersection( player_pos_cmp ) )
        {
          // Set the default footstep SFX for this scene
          reg().template emplace_or_replace<Cmp::Player::Footstep>( Utils::Player::get_entity( m_reg ), Cmp::Player::Footstep::Type::MUD );
          auto corruption_dmg = Sys::PersistSystem::get<Cmp::Persist::CorruptionDamage>( reg() ).get_value();
          player_stats_cmp.apply( { Cmp::Stats::Health{ -corruption_dmg }, {}, {}, {}, {}, {} } );
          SPDLOG_DEBUG( "Applying corruption damage {}", corruption_dmg );
          // trigger death animation
          if ( player_stats_cmp.health() <= 0 )
          {
            get_systems_event_queue().trigger( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::DECAYING, player_position ) );
          }
          return;
        }
      }
      else
      {
        // new HazardType added without a matching sprite_type branch here - fails to compile instead of
        // silently skipping player collision handling for it
        static_assert( Detail::dependent_false<HazardType>, "check_player_hazard_field_collision: unhandled Traits::sprite_type" );
      }
    }
  }
}

//! @copydoc HazardFieldSystem::check_npc_hazard_field_collision()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::check_npc_hazard_field_collision()
{
  auto hazard_view = reg().template view<HazardType, Cmp::Position>();
  auto npc_view = reg().template view<Cmp::Npc::NPC, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : npc_view.each() )
  {
    // optimization
    if ( not Utils::is_visible_in_view( view_bounds, npc_pos_cmp ) ) continue;
    // crows can't be killed (see NpcSystem::on_destroy_npc_event); skipped here too, as the first kill ends this check
    if ( reg().template any_of<Cmp::Npc::ElderMother, Cmp::Npc::Ghost, Cmp::Npc::Crow>( npc_entt ) ) continue;

    for ( auto [hazard_entt, hazard_cmp, hazard_pos_cmp] : hazard_view.each() )
    {
      if ( not npc_pos_cmp.findIntersection( hazard_pos_cmp ) ) continue;

      get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entt ) );
      SPDLOG_DEBUG( "NPC fell into a hazard field at position ({}, {})!", hazard_pos_cmp.x, hazard_pos_cmp.y );
      return;
    }
  }
}

//! @brief Concrete derived HazardFieldSystem using CRTP
class SinkHoleHazardSystem final : public HazardFieldSystem<Cmp::SinkholeCell>
{
public:
  using HazardFieldSystem<Cmp::SinkholeCell>::HazardFieldSystem;
};

//! @brief Concrete derived HazardFieldSystem using CRTP
class CorruptionHazardSystem final : public HazardFieldSystem<Cmp::CorruptionCell>
{
public:
  using HazardFieldSystem<Cmp::CorruptionCell>::HazardFieldSystem;

  // Add corruption-specific methods here if needed
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEMIMPL_HPP__
