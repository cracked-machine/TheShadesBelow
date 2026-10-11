#include <Components/Altar/MultiBlock.hpp>
#include <Components/Crypt/Chest.hpp>
#include <Components/Crypt/Entrance.hpp>
#include <Components/Crypt/Lever.hpp>
#include <Components/Exit.hpp>
#include <Components/Inventory/DowsingTarget.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Player/Doglegs.hpp>
#include <Components/Position.hpp>
#include <PathFinding/BreadthFirst.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/ProcGen/DoglegSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <array>
#include <optional>
#include <vector>

namespace Game::Sys::ProcGen
{

namespace
{

//! @brief Triangle list for a chevron in marker-local space: +x is the direction of travel, y is sideways, units
//! are pixels. The two arms are separate parallelograms sharing the centre edge so that a semi-transparent color
//! is not blended twice at the tip.
//! @param width Sideways extent between the two trailing ends
//! @param height Extent along the direction of travel, from trailing ends to tip
//! @param thickness Stroke thickness of each arm, measured along the direction of travel
constexpr std::array<sf::Vector2f, 12> make_chevron_shape( float width, float height, float thickness )
{
  const float front = ( height + thickness ) * 0.5f;
  const float back = front - height;
  const float half_width = width * 0.5f;

  const sf::Vector2f tip{ front, 0.f };
  const sf::Vector2f notch{ front - thickness, 0.f };
  const sf::Vector2f left{ back, -half_width };
  const sf::Vector2f left_inner{ back - thickness, -half_width };
  const sf::Vector2f right{ back, half_width };
  const sf::Vector2f right_inner{ back - thickness, half_width };

  return { tip, left, left_inner, tip, left_inner, notch, tip, right, right_inner, tip, right_inner, notch };
}

//! @brief Shape of each marker in the dowsing rod pulse train, as a triangle list in marker-local space (+x is the
//! direction of travel, origin is the marker centre, units are pixels). Replace this to change the marker shape;
//! any number of triangles is fine.
constexpr auto kDowsingMarkerShape = make_chevron_shape( 10.f, 5.f, 3.f );

constexpr sf::Color kAltarColour{ 255, 255, 0, 255 };
constexpr sf::Color kCryptColour{ 255, 0, 0, 255 };
constexpr sf::Color kExitColour{ 0, 255, 0, 255 };
constexpr int kMarkerCount = 10;      // markers in the pulse train
constexpr float kMarkerSpacing = 8.f; // px between consecutive markers
constexpr float kPulseSpeed = 50.f;   // px per second
constexpr float kTrainLength = kMarkerCount * kMarkerSpacing;
constexpr float kViewMargin = 8.f;                   // px past the view edge, so markers slide off screen instead of popping
const sf::Time kFieldRefresh = sf::seconds( 0.25f ); // how stale the crypt routes may get while the player stands still

//! @brief Where a marker sits on a path, and which way it faces
struct MarkerPlacement
{
  sf::Vector2f position;
  sf::Vector2f direction;
};

//! @brief Find the point `dist` px along a path
//! @return std::nullopt if `dist` is past the end of the path
std::optional<MarkerPlacement> place_marker( const Cmp::Player::Doglegs::Dogleg &dogleg, float dist )
{
  for ( std::size_t i = 0; i + 1 < dogleg.points.size(); ++i )
  {
    const sf::Vector2f delta = dogleg.points[i + 1] - dogleg.points[i];
    const float length = delta.length();
    if ( length <= 0.f ) continue;
    if ( dist <= length ) return MarkerPlacement{ .position = dogleg.points[i] + ( delta * ( dist / length ) ), .direction = delta / length };
    dist -= length;
  }
  return std::nullopt;
}

} // namespace

void DoglegSystem::init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh )
{
  m_npc_navmesh = npc_navmesh;
  // a field belongs to the navmesh it was made from
  m_field.clear();
}

void DoglegSystem::update( sf::Time dt )
{
  // always clear first, so nothing stale is left to render once the rod leaves the inventory
  clear();

  m_location = Utils::scene_setting<Cmp::SceneSettings::CurrentScene>( reg() ).id;

  auto [inventory_entt, _, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_entt == entt::null ) return;
  auto *dowsing_cmp = reg().try_get<Cmp::Inventory::DowsingTarget>( inventory_entt );
  if ( not dowsing_cmp ) return;

  if ( Utils::Player::get_inventory_wear_level( reg() ) <= 0 ) return;

  dowsing_cmp->m_dowsing_pulse_time += dt;
  if ( m_location == Cmp::SceneSettings::SceneId::CRYPT ) update_field( dt );
  update_target( *dowsing_cmp );
  update_markers( *dowsing_cmp );
}

void DoglegSystem::add_dogleg( sf::Vector2f source_pos, sf::Vector2f target_pos, sf::Color color )
{
  auto &dl_cmp = Utils::Player::get_doglegs( reg() );
  sf::Vector2f corner{};
  if ( target_pos.y - source_pos.y < target_pos.x - source_pos.x ) { corner = sf::Vector2f{ source_pos.x, target_pos.y }; }
  else { corner = sf::Vector2f{ target_pos.x, source_pos.y }; }

  dl_cmp.m_doglegs.push_back( { { source_pos, corner, target_pos }, color } );
}

void DoglegSystem::add_route( sf::Vector2f source_pos, sf::Vector2f target_pos, sf::Color color )
{
  const std::vector<sf::Vector2f> route = PathFinding::route_to( m_field, target_pos );
  if ( route.empty() ) return; // no way there on foot

  // The route runs between cell centres, but the player moves sub-grid: leave from the player and step sideways
  // onto the line of the first leg, rather than cutting a diagonal that could clip a passage corner.
  std::vector<sf::Vector2f> points{ source_pos };
  if ( route.size() == 1 ) { points.push_back( target_pos ); }
  else
  {
    const bool first_leg_horizontal = route[0].y == route[1].y;
    const sf::Vector2f join = first_leg_horizontal ? sf::Vector2f{ source_pos.x, route[0].y } : sf::Vector2f{ route[0].x, source_pos.y };
    if ( join != source_pos ) points.push_back( join );
    points.insert( points.end(), route.begin() + 1, route.end() );
  }

  Utils::Player::get_doglegs( reg() ).m_doglegs.push_back( { std::move( points ), color } );
}

void DoglegSystem::update_field( sf::Time dt )
{
  auto navmesh = m_npc_navmesh.lock();
  if ( not navmesh )
  {
    m_field.clear();
    return;
  }

  // Searching the whole crypt every frame is wasteful: redo it when the player changes cell, and every so often
  // besides, since rooms and passages open and close around a player who is standing still.
  const sf::Vector2f player_centre = Utils::Player::get_position( reg() ).getCenter();
  const sf::Vector2f player_cell = Utils::snap_to_grid( player_centre, Utils::Rounding::TOWARDS_ZERO );
  m_field_age += dt;
  if ( not m_field.empty() and player_cell == m_field_cell and m_field_age < kFieldRefresh ) return;

  m_field = PathFinding::breadth_first_field( reg(), *navmesh, player_centre );
  m_field_cell = player_cell;
  m_field_age = sf::Time::Zero;
}

void DoglegSystem::update_target( const Cmp::Inventory::DowsingTarget &dowsing_cmp )
{

  using Target = Cmp::Inventory::DowsingTarget::Target;

  auto pos_cmp = Utils::Player::get_position( reg() );
  switch ( dowsing_cmp.target )
  {
    case Target::YELLOW: {
      switch ( m_location )
      {
        case Cmp::SceneSettings::SceneId::GRAVEYARD: {
          auto altar_view = reg().view<Cmp::Altar::MultiBlock>();
          for ( auto [altar_entt, altar_cmp] : altar_view.each() )
          {
            add_dogleg( pos_cmp.getCenter(), altar_cmp.getCenter(), kAltarColour );
          }
          break;
        }
        case Cmp::SceneSettings::SceneId::CRYPT: {
          auto crypt_chest_view = reg().view<Cmp::Crypt::Chest, Cmp::Position>();
          for ( auto [chest_entt, chest_cmp, chest_pos] : crypt_chest_view.each() )
          {
            if ( chest_cmp.open ) continue;
            add_route( pos_cmp.getCenter(), chest_pos.getCenter(), kAltarColour );
          }
          break;
        }
        default:
          break;
      }
      break;
    }
    case Target::RED: {
      switch ( m_location )
      {
        case Cmp::SceneSettings::SceneId::GRAVEYARD: {
          auto crypt_entrance_view = reg().view<Cmp::Crypt::Entrance, Cmp::Position>();
          for ( auto [crypt_entt, crypt_cmp, crypt_pos_cmp] : crypt_entrance_view.each() )
          {
            add_dogleg( pos_cmp.getCenter(), crypt_pos_cmp.getCenter(), kCryptColour );
          }
          break;
        }
        case Cmp::SceneSettings::SceneId::CRYPT: {
          auto crypt_lever_view = reg().view<Cmp::Crypt::Lever, Cmp::Position>();
          for ( auto [lever_entt, lever_cmp, lever_pos_cmp] : crypt_lever_view.each() )
          {
            if ( lever_cmp.isEnabled() ) continue;
            add_route( pos_cmp.getCenter(), lever_pos_cmp.getCenter(), kCryptColour );
          }
          break;
        }
        default:
          break;
      }
      break;
    }
    case Target::GREEN: {
      switch ( m_location )
      {
        case Cmp::SceneSettings::SceneId::GRAVEYARD: {
          auto graveyard_exit_view = reg().view<Cmp::Exit, Cmp::Position>();
          for ( auto [exit_entt, exit_cmp, exit_pos_cmp] : graveyard_exit_view.each() )
          {
            add_dogleg( pos_cmp.getCenter(), exit_pos_cmp.getCenter(), kExitColour );
          }
          break;
        }
        case Cmp::SceneSettings::SceneId::CRYPT: {
          auto crypt_exit_view = reg().view<Cmp::Exit, Cmp::Position>();
          for ( auto [exit_entt, exit_cmp, exit_pos_cmp] : crypt_exit_view.each() )
          {
            add_route( pos_cmp.getCenter(), exit_pos_cmp.getCenter(), kExitColour );
          }
          break;
        }
        default:
          break;
      }
      break;
    }
    case Target::NONE: {
      break;
    }
  }
}

void DoglegSystem::update_markers( Cmp::Inventory::DowsingTarget &dowsing_cmp )
{

  auto &dogleg_cmp = Utils::Player::get_doglegs( reg() );
  if ( dogleg_cmp.m_doglegs.empty() ) return;

  // the pulse only needs to travel the part of each path that is on screen
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );
  std::vector<float> end_lengths;
  end_lengths.reserve( dogleg_cmp.m_doglegs.size() );
  for ( const auto &dogleg : dogleg_cmp.m_doglegs )
    end_lengths.push_back( visible_length( dogleg, view_bounds ) );

  // all paths share one pulse, so they launch together; restart once the tail has cleared the longest visible path
  const float longest = *std::ranges::max_element( end_lengths );

  float head = dowsing_cmp.m_dowsing_pulse_time.asSeconds() * kPulseSpeed;
  if ( head > longest + kTrainLength )
  {
    dowsing_cmp.m_dowsing_pulse_time = sf::Time::Zero;
    head = 0.f;
  }

  for ( std::size_t dogleg_idx = 0; dogleg_idx < dogleg_cmp.m_doglegs.size(); ++dogleg_idx )
  {
    const auto &dogleg = dogleg_cmp.m_doglegs[dogleg_idx];
    const float end_length = end_lengths[dogleg_idx];

    for ( int i = 0; i < kMarkerCount; ++i )
    {
      // distance of this marker along the path; markers emerge from the player and vanish at the target or view edge
      const float dist = head - ( static_cast<float>( i ) * kMarkerSpacing );
      if ( dist < 0.f or dist > end_length ) continue;

      const auto marker = place_marker( dogleg, dist );
      if ( not marker ) continue;
      const sf::Vector2f side = marker->direction.perpendicular();

      // fade towards the tail of the train
      sf::Color color = dogleg.color;
      color.a = static_cast<std::uint8_t>( color.a * ( kMarkerCount - i ) / kMarkerCount );

      for ( const auto &v : kDowsingMarkerShape )
        dogleg_cmp.m_markers.append( sf::Vertex{ .position = marker->position + ( marker->direction * v.x ) + ( side * v.y ), .color = color } );
    }
  }
}

void DoglegSystem::clear()
{
  auto &dl_cmp = Utils::Player::get_doglegs( reg() );
  dl_cmp.m_doglegs.clear();
  dl_cmp.m_markers.clear();
}

float DoglegSystem::visible_length( const Cmp::Player::Doglegs::Dogleg &dogleg, const sf::FloatRect &view_bounds )
{
  float travelled = 0.f;
  const auto &points = dogleg.points;
  for ( std::size_t i = 0; i + 1 < points.size(); ++i )
  {
    const sf::Vector2f start = points[i];
    const sf::Vector2f delta = points[i + 1] - start;
    if ( not view_bounds.contains( start ) ) return travelled;
    if ( view_bounds.contains( points[i + 1] ) )
    {
      travelled += delta.length();
      continue;
    }

    // leg leaves the view: find the fraction of the leg at which it crosses the view edge
    float exit_fraction = 1.f;
    if ( delta.x > 0.f ) exit_fraction = std::min( exit_fraction, ( view_bounds.position.x + view_bounds.size.x - start.x ) / delta.x );
    if ( delta.x < 0.f ) exit_fraction = std::min( exit_fraction, ( view_bounds.position.x - start.x ) / delta.x );
    if ( delta.y > 0.f ) exit_fraction = std::min( exit_fraction, ( view_bounds.position.y + view_bounds.size.y - start.y ) / delta.y );
    if ( delta.y < 0.f ) exit_fraction = std::min( exit_fraction, ( view_bounds.position.y - start.y ) / delta.y );
    return std::min( travelled + ( delta.length() * exit_fraction ) + kViewMargin, dogleg.length() );
  }
  return travelled;
}

} // namespace Game::Sys::ProcGen