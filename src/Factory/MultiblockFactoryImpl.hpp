#ifndef SRC_FACTORY_MULTIBLOCKFACTORYIMPL_HPP__
#define SRC_FACTORY_MULTIBLOCKFACTORYIMPL_HPP__

// Template definitions for Game::Factory. Only ever included by the
// MultiblockFactory*.cpp translation units that explicitly instantiate these
// templates - never by consumers of MultiblockFactory.hpp - so that editing a
// function body only forces a handful of instantiation files to recompile.
#include <Components/Altar/Segment.hpp>
#include <Components/Grave/PlantSegment.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Particle/BlockParticle.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/PendingNoPath.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Utils/Constants.hpp>

namespace Game::Factory::Multiblock
{

namespace detail
{

//! @brief Create the multiblock "root" entity: emplaces the MULTIBLOCK bounds, anim, z-order, UUID and position.
//! @tparam MULTIBLOCK Multiblock bounds component type.
//! @param reg
//! @param entity Entity to emplace the multiblock components onto.
//! @param uuid Identifier shared with the multiblock's segment entities.
//! @param pos
//! @param ss
//! @param ss_index Sprite frame index within `ss`.
template <typename MULTIBLOCK>
  requires IsMB<MULTIBLOCK>
void create_multiblock( entt::registry &reg, entt::entity entity, const Cmp::UUID &uuid, Cmp::Position pos, const Sprites::SpriteSheet &ss,
                        size_t ss_index )
{
  reg.emplace_or_replace<MULTIBLOCK>( entity, pos.position, ss.get_px_size() );
  // clang-format off
  reg.emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{
        .sprite_type = ss.get_sprite_type(),
        .frame_index_offset = ss_index,
        .enabled = true
  });
  // clang-format on
  reg.emplace_or_replace<Cmp::ZOrderValue>( entity, pos.position.y );
  // Deliberately not reserved here: reserving the root's own origin cell before
  // create_multiblock_segments() scans for candidate tiles would make that cell fail its own
  // reserved_sm check, permanently skipping the segment there (e.g. the top cell of a 1x2 plant).
  // The origin cell gets reserved along with every other covered cell once its segment is created.
  reg.emplace_or_replace<Cmp::UUID>( entity, uuid );
  reg.emplace_or_replace<Cmp::Position>( entity, pos.position, ss.get_px_size() );

  [[maybe_unused]] auto zorder_cmp = reg.get<Cmp::ZOrderValue>( entity );
  // clang-format off
  SPDLOG_DEBUG( "Placed {} at position ({}, {}). Grid size: {}x{} at z-order {} - {}",
    ss.get_sprite_type(),
    pos.position.x,
    pos.position.y,
    ss.get_grid_size().x,
    ss.get_grid_size().y,
    zorder_cmp.getZOrder(),
    uuid.str()
  );
  // clang-format on
}

//! @brief Recompute solid mask, z-order and pathfinding-blocking components for every MBSEGMENT inside `mb_cmp`'s bounds.
//! @tparam MULTIBLOCK Multiblock bounds component type.
//! @tparam MBSEGMENT Multiblock segment component type.
//! @param reg
//! @param ss Sprite sheet providing the solid mask and z-order lookup tables.
//! @param mb_entt
//! @param mb_cmp Multiblock bounds used to find owned segments and their relative grid position.
template <typename MULTIBLOCK, typename MBSEGMENT>
  requires IsMB<MULTIBLOCK> && IsMBSegment<MBSEGMENT>
void update_segments( entt::registry &reg, const Sprites::SpriteSheet &ss, [[maybe_unused]] entt::entity mb_entt, MULTIBLOCK mb_cmp )
{
  auto solid_masks = ss.get_solid_mask();

  for ( auto [entity, segment_cmp, pos_cmp] : reg.view<MBSEGMENT, Cmp::Position>().each() )
  {
    if ( not pos_cmp.findIntersection( mb_cmp ) ) continue;

    float rel_x = pos_cmp.position.x - mb_cmp.position.x;
    float rel_y = pos_cmp.position.y - mb_cmp.position.y;

    int rel_grid_x = static_cast<int>( rel_x / Constants::kGridSizePx.x );
    int rel_grid_y = static_cast<int>( rel_y / Constants::kGridSizePx.y );

    std::size_t calculated_grid_index = ( rel_grid_y * ss.get_grid_size().x ) + rel_grid_x;

    bool new_solid_mask = true;
    if ( not solid_masks.empty() && solid_masks.size() > calculated_grid_index ) { new_solid_mask = solid_masks.at( calculated_grid_index ); }

    segment_cmp.set_solid_mask( new_solid_mask );
    reg.emplace_or_replace<Cmp::ZOrderValue>( entity, pos_cmp.position.y + ss.get_zorder( calculated_grid_index ) );
    SPDLOG_DEBUG( "{} {} Zorder is set {} for y {}", static_cast<uint32_t>( entity ), ss.get_sprite_type(),
                  pos_cmp.position.y + ss.get_zorder( calculated_grid_index ), pos_cmp.position.y );

    if ( new_solid_mask )
    {
      reg.emplace_or_replace<Cmp::Npc::NoPathFinding>( entity );
      if constexpr ( std::is_same_v<MBSEGMENT, Cmp::PlantSegment> )
      {
        // A plant can be replanted directly under the player - defer real blocking until they step
        // off, otherwise it traps them immediately. See PlayerSystem::promote_pending_no_path.
        reg.emplace_or_replace<Cmp::Player::PendingNoPath>( entity );
      }
      else
      {
        reg.emplace_or_replace<Cmp::Player::NoPath>( entity );
        // Altar segments are handled specially by AltarSystem and shouldn't also block particles.
        if constexpr ( not std::is_same_v<MBSEGMENT, Cmp::Altar::Segment> ) { reg.emplace_or_replace<Cmp::Particle::BlockParticle>( entity ); }
      }
    }
    else
    {
      reg.remove<Cmp::Npc::NoPathFinding>( entity );
      reg.remove<Cmp::Player::NoPath>( entity );
      reg.remove<Cmp::Player::PendingNoPath>( entity );
      reg.remove<Cmp::Particle::BlockParticle>( entity );
    }
  }
}

//! @brief Create one MBSEGMENT entity per world tile inside the multiblock's bounds, tagging the door tile
//! with the type-appropriate entrance/exit component.
//! @tparam MULTIBLOCK Multiblock bounds component type.
//! @tparam MBSEGMENT Multiblock segment component type.
//! @param reg
//! @param multiblock_entity
//! @param uuid Identifier shared with the multiblock root entity.
//! @param mb_pos_cmp
//! @param ss Sprite sheet providing the door position and grid size.
//! @return The newly created segment entities.
template <typename MULTIBLOCK, typename MBSEGMENT>
  requires IsMB<MULTIBLOCK> && IsMBSegment<MBSEGMENT>
std::vector<entt::entity> create_multiblock_segments( entt::registry &reg, entt::entity multiblock_entity, const Cmp::UUID &uuid,
                                                      Cmp::Position mb_pos_cmp, const Sprites::SpriteSheet &ss,
                                                      PathFinding::SpatialHashGrid *reserved_sm )
{

  MULTIBLOCK new_multiblock_bounds = reg.get<MULTIBLOCK>( multiblock_entity );
  std::size_t door_grid_index = static_cast<std::size_t>( ( ss.get_door_position().y * ss.get_grid_size().x ) + ss.get_door_position().x );

  // track which grid positions are already assigned segments
  PathFinding::SpatialHashGrid segment_map;

  // cache upfront so we dont add mutate the view whilst iterating it.
  // Only accept genuine world tiles (created with Cmp::Armable): a bare Cmp::Position view
  // also matches the player, NPCs and dropped items standing inside the bounds, which would
  // get segments created at their (possibly off-grid) positions and be reserved.
  std::vector<entt::entity> world_pos_entt_list;
  for ( auto [pos_entity, pos_cmp] : reg.view<Cmp::Position>().each() )
  {
    if ( not pos_cmp.findIntersection( new_multiblock_bounds ) ) continue;
    if ( not reg.all_of<Cmp::Armable>( pos_entity ) ) continue;
    if ( reg.any_of<MULTIBLOCK, MBSEGMENT, Cmp::Player::Character, Cmp::Npc::NPC, Cmp::WorldItem>( pos_entity ) ) continue;
    // Plants may be grown over an already-reserved tile (e.g. an obstacle) - every other
    // multiblock type still needs the tile free to avoid overlapping another structure.
    if constexpr ( not std::is_same_v<MBSEGMENT, Cmp::PlantSegment> )
    {
      if ( ( reserved_sm != nullptr ) && not reserved_sm->at( pos_cmp ).empty() ) continue;
    }
    world_pos_entt_list.push_back( pos_entity );
  }

  std::vector<entt::entity> created_entts;
  for ( auto world_pos_entity : world_pos_entt_list )
  {

    // mark this world entt as reserved to prevent repeat visits
    if ( reserved_sm != nullptr ) reserved_sm->insert( world_pos_entity, reg.get<Cmp::Position>( world_pos_entity ) );

    // Calculate relative pixel positions within the large obstacle grid
    auto pos_cmp = reg.get<Cmp::Position>( world_pos_entity );
    float rel_x = pos_cmp.position.x - mb_pos_cmp.position.x;
    float rel_y = pos_cmp.position.y - mb_pos_cmp.position.y;

    // Convert to relative grid coordinates
    int rel_grid_x = static_cast<int>( rel_x / Constants::kGridSizePx.x );
    int rel_grid_y = static_cast<int>( rel_y / Constants::kGridSizePx.y );

    std::size_t calculated_grid_index = ( rel_grid_y * ss.get_grid_size().x ) + rel_grid_x;
    SPDLOG_DEBUG( "  - Creating segment at ({}, {}) with sprite_index {}", pos_cmp.position.x, pos_cmp.position.y, calculated_grid_index );

    // Don't allow duplicate segments at a given position
    if ( not segment_map.at( pos_cmp ).empty() ) continue;

    auto new_segment_entt = reg.create();
    reg.emplace_or_replace<MBSEGMENT>( new_segment_entt, true );
    reg.emplace_or_replace<Cmp::Armable>( new_segment_entt );
    reg.emplace_or_replace<Cmp::UUID>( new_segment_entt, uuid );
    reg.emplace_or_replace<Cmp::Position>( new_segment_entt, pos_cmp.position, pos_cmp.size );
    if ( reserved_sm != nullptr ) reserved_sm->insert( new_segment_entt, pos_cmp );

    if constexpr ( std::is_same_v<MULTIBLOCK, Cmp::Crypt::BuildingMultiBlock> )
    {
      if ( calculated_grid_index == door_grid_index ) { reg.emplace_or_replace<Cmp::Crypt::Entrance>( new_segment_entt ); }
    }
    else if constexpr ( std::is_same_v<MULTIBLOCK, Cmp::HealingSpringBuildingMultiBlock> )
    {
      if ( calculated_grid_index == door_grid_index ) { reg.emplace_or_replace<Cmp::HealingSpringEntrance>( new_segment_entt ); }
    }
    else if constexpr ( std::is_same_v<MULTIBLOCK, Cmp::Ruin::BuildingMultiBlock> )
    {
      if ( calculated_grid_index == door_grid_index ) { reg.emplace_or_replace<Cmp::Ruin::Entrance>( new_segment_entt ); }
    }
    else if constexpr ( std::is_same_v<MULTIBLOCK, Cmp::Grave::ExitMultiBlock> )
    {
      if ( calculated_grid_index == door_grid_index ) { reg.emplace_or_replace<Cmp::Exit>( new_segment_entt ); }
    }

    created_entts.push_back( new_segment_entt );
    segment_map.insert( new_segment_entt, pos_cmp );
  }

  update_segments<MULTIBLOCK, MBSEGMENT>( reg, ss, multiblock_entity, new_multiblock_bounds );

  return created_entts;
}

} // namespace detail

//! @brief Create a multiblock structure (e.g. an altar, crypt building, staircase) along with all of its segments.
//! @tparam MULTIBLOCK Multiblock bounds component type.
//! @tparam MBSEGMENT Multiblock segment component type.
//! @param reg
//! @param position World position of the multiblock's origin.
//! @param ss
//! @param ss_index Sprite frame index within `ss`.
//! @param zorder Explicit z-order; if 0, falls back to the sprite sheet's json z-order, then to the y-position.
//! @return The multiblock root entity and its created segment entities.
template <typename MULTIBLOCK, typename MBSEGMENT>
  requires IsMB<MULTIBLOCK> && IsMBSegment<MBSEGMENT>
std::pair<entt::entity, std::vector<entt::entity>> add_multiblock_with_segments( entt::registry &reg, sf::Vector2f position,
                                                                                 const Sprites::SpriteSheet &ss, size_t ss_index, float zorder,
                                                                                 PathFinding::SpatialHashGrid *reserved_sm )
{
  auto mb_entt = reg.create();
  Cmp::Position new_pos_cmp( position, ss.get_sprite_size() );
  auto uuid = Cmp::UUID::generate();

  reg.emplace_or_replace<Cmp::Position>( mb_entt, new_pos_cmp.position, ss.get_sprite_size() );

  Multiblock::detail::create_multiblock<MULTIBLOCK>( reg, mb_entt, uuid, new_pos_cmp, ss, ss_index );
  auto segment_entt_list = Multiblock::detail::create_multiblock_segments<MULTIBLOCK, MBSEGMENT>( reg, mb_entt, uuid, new_pos_cmp, ss, reserved_sm );

  for ( auto [mb_entt, mb_cmp, mb_zorder_cmp] : reg.view<MULTIBLOCK, Cmp::ZOrderValue>().each() )
  {
    if ( zorder != 0 )
    {
      // Use the function param if explicitly set
      mb_zorder_cmp.setZOrder( zorder );
    }
    else if ( ss.get_zorder( ss_index ) != 0 )
    {
      // Use the y-axis position plus the json zorder value
      mb_zorder_cmp.setZOrder( static_cast<sf::FloatRect>( mb_cmp ).position.y + ss.get_zorder( ss_index ) );
    }
    else
    {
      // fallback to the y-axis position only
      mb_zorder_cmp.setZOrder( static_cast<sf::FloatRect>( mb_cmp ).position.y );
    }
  }
  return { mb_entt, segment_entt_list };
}

} // namespace Game::Factory::Multiblock

#endif // SRC_FACTORY_MULTIBLOCKFACTORYIMPL_HPP__
