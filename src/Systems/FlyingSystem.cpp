#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteRenderOffset.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/Crypt/BuildingMultiBlock.hpp>
#include <Components/Crypt/ObjectiveMultiBlock.hpp>
#include <Components/Grave/ExitMultiBlock.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/PlantMultiBlock.hpp>
#include <Components/Hazard/CollisionResist.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Obstacle.hpp>
#include <Components/ObstacleCap.hpp>
#include <Components/Persistent/FlyingLandingOverlap.hpp>
#include <Components/Persistent/FlyingLiftOffset.hpp>
#include <Components/Persistent/FlyingOscillationAmplitude.hpp>
#include <Components/Persistent/FlyingTransitionTime.hpp>
#include <Components/Persistent/PlayerRenderOffset.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/CrashLanding.hpp>
#include <Components/Player/Flying.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Player/PendingNoPath.hpp>
#include <Components/Player/SolidBoundary.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/Ruin/BuildingMultiBlock.hpp>
#include <Components/Ruin/HexagramMultiBlock.hpp>
#include <Components/Ruin/StairsBalustradeMultiBlock.hpp>
#include <Components/Ruin/StairsGateMultiBlock.hpp>
#include <Components/Ruin/StairsLowerMultiBlock.hpp>
#include <Components/Ruin/StairsUpperMultiBlock.hpp>
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Spring/HealingSpringMultiBlock.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/UpdatePlayerPositionEvent.hpp>
#include <SFML/Audio/Sound.hpp>
#include <Systems/FlyingSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Threats/WormholeSystem.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Npc.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace Game::Sys
{

namespace
{

//! @brief How often the oscillation picks a new random displacement target. Short enough to look lively,
//! long enough that easing towards it reads as a wobble rather than a jitter.
constexpr float kOscillationRetargetSeconds = 0.15f;

//! @brief Rate (per second) the displacement eases towards its target.
constexpr float kOscillationEaseRate = 12.f;

//! @brief Max distance, in grid cells, searched for a free landing cell.
constexpr int kLandingSearchRadius = 8;

//! @brief Whether a multiblock is bigger than a single grid cell on either axis.
bool is_large( const sf::FloatRect &mb ) { return mb.size.x > Constants::kGridSizePxF.x or mb.size.y > Constants::kGridSizePxF.y; }

//! @brief A multiblock root found under a cell: its bounds and sprite sheet.
struct MultiblockRoot
{
  sf::FloatRect bounds;
  Sys::SpriteKey sprite_type;
};

//! @brief Set `found` to the first MULTIBLOCK root whose bounds overlap `rect`, unless already found.
template <typename MULTIBLOCK>
void find_root_of( entt::registry &reg, const sf::FloatRect &rect, std::optional<MultiblockRoot> &found )
{
  if ( found ) return;
  for ( auto [mb_entt, mb_cmp, anim_cmp] : reg.view<MULTIBLOCK, Cmp::AnimData>().each() )
  {
    if ( not rect.findIntersection( mb_cmp ) ) continue;
    found = MultiblockRoot{ mb_cmp, anim_cmp.m_sprite_type };
    return;
  }
}

//! @brief The multiblock root, of any of the given types, whose bounds overlap `rect`, if any.
template <typename... MULTIBLOCKS>
std::optional<MultiblockRoot> find_multiblock_root( entt::registry &reg, const sf::FloatRect &rect )
{
  std::optional<MultiblockRoot> found;
  ( find_root_of<MULTIBLOCKS>( reg, rect, found ), ... );
  return found;
}

//! @brief Whether `rect` overlaps the position of any entity owning a `Component`. Unlike
//! Utils::Collision::any_intersects this also works for empty tag components.
template <typename Component>
bool any_overlaps( entt::registry &reg, const Cmp::RectBounds &rect )
{
  auto view = reg.view<Component, Cmp::Position>();
  for ( auto entt : view )
  {
    if ( rect.findIntersection( view.template get<Cmp::Position>( entt ) ) ) return true;
  }
  return false;
}

} // namespace

FlyingSystem::FlyingSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&FlyingSystem::on_player_action>( this );
}

void FlyingSystem::on_player_action( Events::PlayerActionEvent ev )
{
  if ( ev.action == Events::PlayerActionEvent::GameActions::FLY ) { m_fly_requested = true; }
}

void FlyingSystem::update( sf::Time dt )
{
  // read and clear the flag.
  const bool fly_requested = std::exchange( m_fly_requested, false );

  // dying mid-flight doesn't land - the death animation plays at the frozen lift offset until game over
  if ( Utils::Player::get_mortality( reg() ).state != Cmp::Player::Mortality::State::ALIVE ) return;

  // the descent can't be cancelled - the fly key is ignored until touchdown
  if ( Utils::Player::is_landing( reg() ) )
  {
    advance_descent( dt );
    return;
  }

  if ( not Utils::Player::is_flying( reg() ) )
  {
    if ( fly_requested and can_take_off() ) { take_off(); }
    return;
  }

  // the player's exact body, not the nearest grid cell - landing is in place, never a sideways slide off an obstacle edge
  const std::optional<sf::Vector2f> landing_spot = landing_spot_below();
  if ( not broom_usable() )
  {
    // worn out (or gone) over an obstacle: crash land on the nearest free cell
    if ( landing_spot ) { begin_landing( landing_spot ); }
    else
    {
      SPDLOG_DEBUG( "Witchesbroom worn out over an obstacle - crash landing" );
      reg().emplace_or_replace<Cmp::Player::CrashLanding>( Utils::Player::get_entity( reg() ) );
      begin_landing( find_landing_cell() );
    }
    advance_descent( dt );
    return;
  }

  // released over an obstacle: keep flying until the player has fully cleared it, including its edges
  if ( not fly_requested and landing_spot )
  {
    begin_landing( landing_spot );
    advance_descent( dt );
    return;
  }

  // ascend: linear ramp up to the full lift
  m_lift_fraction = std::min( 1.f, m_lift_fraction + ( dt.asSeconds() / transition_seconds() ) );
  update_render_offset( dt );
}

sf::Vector2f FlyingSystem::resting_render_offset() { return { 0.f, Sys::PersistSystem::get<Cmp::Persist::PlayerRenderOffset>( reg() ).get_value() }; }

float FlyingSystem::transition_seconds()
{
  return std::max( 0.01f, Sys::PersistSystem::get<Cmp::Persist::FlyingTransitionTime>( reg() ).get_value() );
}

bool FlyingSystem::broom_usable()
{
  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  return inventory_type == "item.witchesbroom" and Utils::Player::get_inventory_wear_level( reg() ) > 0;
}

bool FlyingSystem::can_take_off()
{
  if ( Utils::Player::get_mortality( reg() ).state != Cmp::Player::Mortality::State::ALIVE ) return false;
  // past the wormhole's point of no return the player is committed to the teleport
  if ( WormholeSystem::is_player_committed_to_jump( reg() ) ) return false;
  return broom_usable();
}

void FlyingSystem::take_off()
{
  auto player_entt = Utils::Player::get_entity( reg() );
  reg().emplace_or_replace<Cmp::Player::Flying>( player_entt );
  m_lift_fraction = 0.f;
  m_displacement = 0.f;
  m_displacement_target = 0.f;
  m_displacement_elapsed = sf::Time::Zero;
  update_render_offset( sf::Time::Zero );
  SPDLOG_DEBUG( "Player took off on the witchesbroom" );
  if ( m_sound_bank.get_effect( "broom_takeoff" ).getStatus() != sf::Sound::Status::Playing ) m_sound_bank.get_effect( "broom_takeoff" ).play();
}

void FlyingSystem::begin_landing( std::optional<sf::Vector2f> target )
{
  auto &flying_cmp = reg().get<Cmp::Player::Flying>( Utils::Player::get_entity( reg() ) );
  flying_cmp.landing = true;

  m_land_start = Utils::Player::get_position( reg() ).position;
  if ( target ) { m_land_target = *target; }
  else
  {
    SPDLOG_WARN( "No free landing cell within {} cells of the player", kLandingSearchRadius );
    m_land_target = m_land_start;
  }
  // releasing mid-ascent descends from the current height
  m_descent_start_fraction = m_lift_fraction;
  m_descent_progress = 0.f;
  SPDLOG_DEBUG( "Player landing at {},{}", m_land_target.x, m_land_target.y );
  if ( m_sound_bank.get_effect( "broom_landing" ).getStatus() != sf::Sound::Status::Playing ) m_sound_bank.get_effect( "broom_landing" ).play();
}

void FlyingSystem::advance_descent( sf::Time dt )
{
  m_descent_progress = std::min( 1.f, m_descent_progress + ( dt.asSeconds() / transition_seconds() ) );
  const float t = m_descent_progress * m_descent_progress * ( 3.f - ( 2.f * m_descent_progress ) ); // smoothstep

  // slide the ground position to the landing cell while the sprite sinks
  const sf::Vector2f ground_pos = m_land_start + ( ( m_land_target - m_land_start ) * t );
  get_systems_event_queue().trigger( Events::UpdatePlayerPositionEvent( ground_pos ) );
  m_lift_fraction = m_descent_start_fraction * ( 1.f - t );

  if ( m_descent_progress < 1.f )
  {
    update_render_offset( dt );
    return;
  }

  // touchdown - normal collision resumes
  auto player_entt = Utils::Player::get_entity( reg() );
  reg().remove<Cmp::Player::Flying, Cmp::Player::CrashLanding>( player_entt );
  reg().emplace_or_replace<Cmp::AbsoluteRenderOffset>( player_entt, resting_render_offset() );
  m_lift_fraction = 0.f;
  SPDLOG_DEBUG( "Player landed at {},{}", m_land_target.x, m_land_target.y );
}

void FlyingSystem::update_render_offset( sf::Time dt )
{
  const float amplitude = Sys::PersistSystem::get<Cmp::Persist::FlyingOscillationAmplitude>( reg() ).get_value();
  const float lift = Sys::PersistSystem::get<Cmp::Persist::FlyingLiftOffset>( reg() ).get_value();

  m_displacement_elapsed += dt;
  if ( m_displacement_elapsed.asSeconds() >= kOscillationRetargetSeconds )
  {
    m_displacement_elapsed = sf::Time::Zero;
    Cmp::RandomFloat rng( -amplitude, amplitude );
    m_displacement_target = rng.gen();
  }
  // frame-rate independent exponential ease towards the target, clamped in case the amplitude was just reduced
  const float ease = 1.f - std::exp( -kOscillationEaseRate * dt.asSeconds() );
  m_displacement += ( m_displacement_target - m_displacement ) * ease;
  m_displacement = std::clamp( m_displacement, -amplitude, amplitude );

  // negative y is "up" - the lift is visual only, Cmp::Position stays on the ground
  auto player_entt = Utils::Player::get_entity( reg() );
  // scaled by the lift fraction so the lift and oscillation ease in/out on take-off and landing,
  // on top of the player's resting render offset
  reg().emplace_or_replace<Cmp::AbsoluteRenderOffset>( player_entt,
                                                       resting_render_offset() + sf::Vector2f{ 0.f, ( -lift + m_displacement ) * m_lift_fraction } );

  // only draw the player in front of obstacles once the sprite has cleared their (1x1) height
  if ( auto *flying_cmp = reg().try_get<Cmp::Player::Flying>( player_entt ) )
  {
    flying_cmp->elevated = ( lift * m_lift_fraction ) >= std::min( lift, Constants::kGridSizePxF.y );
  }
}

bool FlyingSystem::blocks_flight( entt::registry &reg, entt::entity entity )
{
  // invisible level boundary - no sprite to consult
  if ( reg.all_of<Cmp::Player::SolidBoundary>( entity ) ) return true;

  const auto *pos_cmp = reg.try_get<Cmp::Position>( entity );
  if ( not pos_cmp ) return reg.all_of<Cmp::Wall>( entity );

  // multiblock segment: use the root's flying_collision_mask for this cell, else block if larger than 1x1.
  // Inset so a multiblock merely sharing an edge with this cell doesn't count.
  const sf::FloatRect cell = Cmp::RectBounds::scaled( *pos_cmp, 0.9f ).getBounds();
  const auto root = find_multiblock_root<Cmp::Altar::MultiBlock, Cmp::Grave::MultiBlock, Cmp::Grave::ExitMultiBlock, Cmp::PlantMultiBlock,
                                         Cmp::Crypt::BuildingMultiBlock, Cmp::Crypt::ObjectiveMultiBlock, Cmp::Ruin::BuildingMultiBlock,
                                         Cmp::Ruin::HexagramMultiBlock, Cmp::Ruin::StairsLowerMultiBlock, Cmp::Ruin::StairsUpperMultiBlock,
                                         Cmp::Ruin::StairsGateMultiBlock, Cmp::Ruin::StairsBalustradeMultiBlock, Cmp::HealingSpringMultiBlock,
                                         Cmp::HealingSpringBuildingMultiBlock>( reg, cell );
  if ( root )
  {
    const auto &ss = Sys::SpriteStore::instance().get( root->sprite_type );
    const auto &flying_mask = ss.flying_collision_mask();
    if ( flying_mask.empty() ) return is_large( root->bounds );

    const sf::Vector2f offset = ( pos_cmp->position - root->bounds.position ).componentWiseDiv( Constants::kGridSizePxF );
    const int col = static_cast<int>( std::round( offset.x ) );
    const int row = static_cast<int>( std::round( offset.y ) );
    const int index = ( row * ss.get_grid_size().x ) + col;
    if ( index < 0 or static_cast<std::size_t>( index ) >= flying_mask.size() ) return true;
    return flying_mask[static_cast<std::size_t>( index )];
  }

  // single sprite (obstacle, wall, loot...): use its flying_collision_mask if specified
  if ( const auto *anim_cmp = reg.try_get<Cmp::AnimData>( entity ) )
  {
    const auto &flying_mask = Sys::SpriteStore::instance().get( anim_cmp->m_sprite_type ).flying_collision_mask();
    if ( not flying_mask.empty() ) return flying_mask.front();
  }

  // fallback: walls block, other 1x1 sprites can be flown over
  return reg.all_of<Cmp::Wall>( entity );
}

float FlyingSystem::elevated_zorder( entt::registry &reg )
{
  const auto &player_pos = Utils::Player::get_position( reg );
  float zorder = kFlyingZOrder + player_pos.position.y;

  // the sprite as drawn - lifted by the render offset
  sf::FloatRect drawn_bounds = player_pos;
  if ( const auto *offset_cmp = reg.try_get<Cmp::AbsoluteRenderOffset>( Utils::Player::get_entity( reg ) ) )
  {
    drawn_bounds.position += offset_cmp->getOffset();
  }

  for ( auto [entity, anim_cmp, pos_cmp, zorder_cmp] :
        reg.view<Cmp::AnimData, Cmp::Position, Cmp::ZOrderValue>( entt::exclude<Cmp::Player::Character> ).each() )
  {
    if ( not drawn_bounds.findIntersection( pos_cmp ) ) continue;
    if ( not Sys::SpriteStore::instance().get( anim_cmp.m_sprite_type ).has_zorder() and not reg.all_of<Cmp::Wall>( entity ) ) continue;
    // honour the explicit zorder or wall: where the player's ground ordering puts them behind it, the flight boost doesn't
    // lift them in front of it
    if ( player_pos.position.y >= zorder_cmp.get() ) continue;
    zorder = std::min( zorder, zorder_cmp.get() - 0.01f );
  }
  return zorder;
}

sf::FloatRect FlyingSystem::shadow_bounds( entt::registry &reg )
{
  const auto &player_pos = Utils::Player::get_position( reg );
  const float render_offset_y = Sys::PersistSystem::get<Cmp::Persist::PlayerRenderOffset>( reg ).get_value();
  // flattened circle: full radius wide, half radius tall
  const sf::Vector2f size{ kFlightShadowRadiusPx * 2.f, kFlightShadowRadiusPx };
  const sf::Vector2f centre{ player_pos.getCenter().x,
                             player_pos.position.y + player_pos.size.y - ( kFlightShadowRadiusPx * 0.5f ) + render_offset_y };
  return { centre - ( size / 2.f ), size };
}

float FlyingSystem::shadow_zorder( entt::registry &reg )
{
  // just before the player, so it is drawn in front of the same obstacles the player is
  float zorder = Utils::Player::get_zorder( reg ).get() - 0.01f;

  // the cap occupies the cell above its obstacle main. It only hides the shadow on open ground: where another
  // obstacle main shares the cap's cell (e.g. stacked walls) the shadow is cast on top of that obstacle.
  auto obstacle_main_under = [&reg]( const Cmp::Position &cap_pos )
  {
    for ( auto [obstacle_entt, obstacle_cmp, obstacle_pos_cmp] : reg.view<Cmp::Obstacle, Cmp::Position>().each() )
    {
      if ( obstacle_cmp.m_enabled and cap_pos.findIntersection( obstacle_pos_cmp ) ) return true;
    }
    return false;
  };

  const sf::FloatRect bounds = shadow_bounds( reg );
  for ( auto [entity, cap_cmp, pos_cmp, zorder_cmp] : reg.view<Cmp::ObstacleCap, Cmp::Position, Cmp::ZOrderValue>().each() )
  {
    if ( not cap_cmp.m_enabled ) continue;
    if ( not bounds.findIntersection( pos_cmp ) ) continue;
    if ( obstacle_main_under( pos_cmp ) ) continue;
    zorder = std::min( zorder, zorder_cmp.get() - 0.01f );
  }
  return zorder;
}

std::optional<sf::Vector2f> FlyingSystem::find_landing_cell()
{
  const auto &player_pos = Utils::Player::get_position( reg() );
  const sf::Vector2f player_centre = player_pos.getCenter();
  const sf::Vector2f origin = Utils::snap_to_grid( player_pos.position );
  const auto grid = Constants::kGridSizePxF;

  // every cell in the search square, nearest to the player's actual position first
  std::vector<sf::Vector2f> candidates;
  for ( int gy = -kLandingSearchRadius; gy <= kLandingSearchRadius; ++gy )
  {
    for ( int gx = -kLandingSearchRadius; gx <= kLandingSearchRadius; ++gx )
    {
      candidates.emplace_back( origin.x + ( static_cast<float>( gx ) * grid.x ), origin.y + ( static_cast<float>( gy ) * grid.y ) );
    }
  }
  auto distance_sq = [&]( const sf::Vector2f &cell ) { return ( ( cell + grid / 2.f ) - player_centre ).lengthSquared(); };
  std::ranges::stable_sort( candidates, {}, distance_sq );

  auto is_free = [this]( const sf::Vector2f &cell ) { return is_free_landing_cell( cell, /*allow_hazards=*/false ); };
  if ( auto it = std::ranges::find_if( candidates, is_free ); it != candidates.end() ) return *it;
  return std::nullopt;
}

std::optional<sf::Vector2f> FlyingSystem::landing_spot_below()
{
  const auto &player_pos = Utils::Player::get_position( reg() );
  // fully clear - land exactly where the player is
  if ( is_free_landing_area( player_pos, /*allow_hazards=*/true ) ) return player_pos.position;

  // only slightly overlapping an obstacle edge (e.g. hovering over a 16x16 gap) - land on the nearest grid cell
  const float overlap = Sys::PersistSystem::get<Cmp::Persist::FlyingLandingOverlap>( reg() ).get_value();
  const sf::Vector2f inset = player_pos.size * overlap;
  const sf::FloatRect tolerant_area( player_pos.position + inset, player_pos.size - ( inset * 2.f ) );
  if ( not is_free_landing_area( tolerant_area, /*allow_hazards=*/true ) ) return std::nullopt;

  const sf::Vector2f cell = Utils::snap_to_grid( player_pos.position );
  if ( is_free_landing_cell( cell, /*allow_hazards=*/true ) ) return cell;
  return std::nullopt;
}

bool FlyingSystem::is_free_landing_cell( const sf::Vector2f &cell, bool allow_hazards )
{
  // shrink the hitbox slightly so neighbouring cells sharing an edge don't count as overlapping
  return is_free_landing_area( Cmp::RectBounds::scaled( cell, Constants::kGridSizePxF, 0.9f ).getBounds(), allow_hazards );
}

bool FlyingSystem::is_free_landing_area( const sf::FloatRect &area, bool allow_hazards )
{
  const Cmp::RectBounds cell_bounds = Cmp::RectBounds::scaled( area.position, area.size, 1.f );
  // must be a walkable world position, i.e. not void/out of bounds
  if ( not any_overlaps<Cmp::Armable>( reg(), cell_bounds ) ) return false;
  if ( any_overlaps<Cmp::Player::NoPath>( reg(), cell_bounds ) ) return false;
  if ( any_overlaps<Cmp::Player::PendingNoPath>( reg(), cell_bounds ) ) return false;
  // sinkholes: the hazard system makes a landed player fall in
  if ( not allow_hazards and any_overlaps<Cmp::Hazard::CollisionResist>( reg(), cell_bounds ) ) return false;
  for ( auto npc_entt : reg().view<Cmp::Npc::NPC>() )
  {
    const auto npc_bounds = Utils::Npc::collision_bounds( reg(), npc_entt );
    if ( npc_bounds and cell_bounds.findIntersection( *npc_bounds ) ) return false;
  }
  return true;
}

} // namespace Game::Sys
