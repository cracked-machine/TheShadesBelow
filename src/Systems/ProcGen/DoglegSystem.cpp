#include <Components/Altar/MultiBlock.hpp>
#include <Components/Crypt/Entrance.hpp>
#include <Components/Exit.hpp>
#include <Components/Inventory/DowsingTarget.hpp>
#include <Components/Player/Doglegs.hpp>
#include <Components/Position.hpp>
#include <Systems/ProcGen/DoglegSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>

#include <algorithm>
#include <array>
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
constexpr float kViewMargin = 8.f; // px past the view edge, so markers slide off screen instead of popping

} // namespace

void DoglegSystem::update( sf::Time dt )
{
  // always clear first, so nothing stale is left to render once the rod leaves the inventory
  clear_doglegs();

  auto [inventory_entt, _, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_entt == entt::null ) return;
  auto *dowsing_cmp = reg().try_get<Cmp::Inventory::DowsingTarget>( inventory_entt );
  if ( not dowsing_cmp ) return;

  dowsing_cmp->m_dowsing_pulse_time += dt;
  update_target( *dowsing_cmp );
  update_markers( *dowsing_cmp );
}

void DoglegSystem::add_dogleg( sf::Vector2f source_pos, sf::Vector2f target_pos, sf::Color color )
{
  auto &dl_cmp = Utils::Player::get_doglegs( reg() );
  sf::Vector2f corner{};
  if ( target_pos.y - source_pos.y < target_pos.x - source_pos.x ) { corner = sf::Vector2f{ source_pos.x, target_pos.y }; }
  else { corner = sf::Vector2f{ target_pos.x, source_pos.y }; }

  dl_cmp.m_doglegs.push_back( { source_pos, corner, target_pos, color } );
}

void DoglegSystem::update_target( const Cmp::Inventory::DowsingTarget &dowsing_cmp )
{

  using Target = Cmp::Inventory::DowsingTarget::Target;

  auto pos_cmp = Utils::Player::get_position( reg() );
  switch ( dowsing_cmp.target )
  {
    case Target::YELLOW: {
      auto altar_view = reg().view<Cmp::Altar::MultiBlock>();
      for ( auto [altar_entt, altar_cmp] : altar_view.each() )
      {
        // yellow for altar paths
        add_dogleg( pos_cmp.getCenter(), altar_cmp.getCenter(), kAltarColour );
      }
      break;
    }
    case Target::RED: {
      auto crypt_view = reg().view<Cmp::Crypt::Entrance, Cmp::Position>();
      for ( auto [crypt_entt, crypt_cmp, crypt_pos_cmp] : crypt_view.each() )
      {
        // red for crypt paths
        add_dogleg( pos_cmp.getCenter(), crypt_pos_cmp.getCenter(), kCryptColour );
      }
      break;
    }
    case Target::GREEN: {
      auto exit_view = reg().view<Cmp::Exit, Cmp::Position>();
      for ( auto [exit_entt, exit_cmp, exit_pos_cmp] : exit_view.each() )
      {
        add_dogleg( pos_cmp.getCenter(), exit_pos_cmp.getCenter(), kExitColour );
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
    const float first_leg_length = ( dogleg.corner - dogleg.source ).length();
    const float end_length = end_lengths[dogleg_idx];

    for ( int i = 0; i < kMarkerCount; ++i )
    {
      // distance of this marker along the path; markers emerge from the player and vanish at the target or view edge
      const float dist = head - ( static_cast<float>( i ) * kMarkerSpacing );
      if ( dist < 0.f or dist > end_length ) continue;

      const bool on_first_leg = dist < first_leg_length;
      const sf::Vector2f leg_start = on_first_leg ? dogleg.source : dogleg.corner;
      const sf::Vector2f leg_end = on_first_leg ? dogleg.corner : dogleg.target;
      const auto dir = Utils::Maths::normalized( leg_end - leg_start );
      if ( not dir ) continue;

      const sf::Vector2f marker_pos = leg_start + ( *dir * ( on_first_leg ? dist : dist - first_leg_length ) );
      const sf::Vector2f side = dir->perpendicular();

      // fade towards the tail of the train
      sf::Color color = dogleg.color;
      color.a = static_cast<std::uint8_t>( color.a * ( kMarkerCount - i ) / kMarkerCount );

      for ( const auto &v : kDowsingMarkerShape )
        dogleg_cmp.m_markers.append( sf::Vertex{ .position = marker_pos + ( *dir * v.x ) + ( side * v.y ), .color = color } );
    }
  }
}

void DoglegSystem::clear_doglegs()
{
  auto &dl_cmp = Utils::Player::get_doglegs( reg() );
  dl_cmp.m_doglegs.clear();
  dl_cmp.m_markers.clear();
}

float DoglegSystem::visible_length( const Cmp::Player::Doglegs::Dogleg &dogleg, const sf::FloatRect &view_bounds )
{
  float travelled = 0.f;
  const std::array<sf::Vector2f, 3> points{ dogleg.source, dogleg.corner, dogleg.target };
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