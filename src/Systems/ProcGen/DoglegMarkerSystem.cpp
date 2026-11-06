#include <Systems/ProcGen/DoglegMarkerSystem.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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

//! @brief Shape of each marker in the pulse train, as a triangle list in marker-local space (+x is the direction of
//! travel, origin is the marker centre, units are pixels). Replace this to change the marker shape; any number of
//! triangles is fine.
constexpr auto kMarkerShape = make_chevron_shape( 10.f, 5.f, 3.f );

constexpr int kMarkerCount = 10;      // markers in the pulse train
constexpr float kMarkerSpacing = 8.f; // px between consecutive markers
constexpr float kPulseSpeed = 50.f;   // px per second
constexpr float kTrainLength = kMarkerCount * kMarkerSpacing;
constexpr float kViewMargin = 8.f; // px past the view edge, so markers slide off screen instead of popping

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

//! @brief Append one train of markers, its head `head` px along the path, stopping at `end_length`
void append_train( Cmp::Player::Doglegs &doglegs, const Cmp::Player::Doglegs::Dogleg &dogleg, float head, float end_length )
{
  for ( int i = 0; i < kMarkerCount; ++i )
  {
    // distance of this marker along the path; markers emerge from the path start and vanish at the end
    const float dist = head - ( static_cast<float>( i ) * kMarkerSpacing );
    if ( dist < 0.f or dist > end_length ) continue;

    const auto marker = place_marker( dogleg, dist );
    if ( not marker ) continue;
    const sf::Vector2f side = marker->direction.perpendicular();

    // fade towards the tail of the train
    sf::Color color = dogleg.color;
    color.a = static_cast<std::uint8_t>( color.a * ( kMarkerCount - i ) / kMarkerCount );

    for ( const auto &v : kMarkerShape )
      doglegs.m_markers.append( sf::Vertex{ .position = marker->position + ( marker->direction * v.x ) + ( side * v.y ), .color = color } );
  }
}

} // namespace

void DoglegMarkerSystem::add_dogleg( Cmp::Player::Doglegs &doglegs, sf::Vector2f source_pos, sf::Vector2f target_pos, sf::Color color )
{
  sf::Vector2f corner{};
  if ( target_pos.y - source_pos.y < target_pos.x - source_pos.x ) { corner = sf::Vector2f{ source_pos.x, target_pos.y }; }
  else { corner = sf::Vector2f{ target_pos.x, source_pos.y }; }

  doglegs.m_doglegs.push_back( { { source_pos, corner, target_pos }, color } );
}

void DoglegMarkerSystem::clear( Cmp::Player::Doglegs &doglegs )
{
  doglegs.m_doglegs.clear();
  doglegs.m_markers.clear();
}

void DoglegMarkerSystem::update_markers( Cmp::Player::Doglegs &doglegs, sf::Time &pulse_time, std::optional<float> repeat_every )
{
  if ( doglegs.m_doglegs.empty() ) return;

  if ( repeat_every and *repeat_every > 0.f )
  {
    // a train leaves every repeat_every px, so the pulse only needs to cycle over that distance
    float lead = pulse_time.asSeconds() * kPulseSpeed;
    if ( lead >= *repeat_every )
    {
      lead = std::fmod( lead, *repeat_every );
      pulse_time = sf::seconds( lead / kPulseSpeed );
    }

    for ( const auto &dogleg : doglegs.m_doglegs )
    {
      const float end_length = dogleg.length();
      for ( float head = lead; head - kTrainLength <= end_length; head += *repeat_every )
        append_train( doglegs, dogleg, head, end_length );
    }
    return;
  }

  // the pulse only needs to travel the part of each path that is on screen
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );
  std::vector<float> end_lengths;
  end_lengths.reserve( doglegs.m_doglegs.size() );
  for ( const auto &dogleg : doglegs.m_doglegs )
    end_lengths.push_back( visible_length( dogleg, view_bounds ) );

  // all paths share one pulse, so they launch together; restart once the tail has cleared the longest visible path
  const float longest = *std::ranges::max_element( end_lengths );

  float head = pulse_time.asSeconds() * kPulseSpeed;
  if ( head > longest + kTrainLength )
  {
    pulse_time = sf::Time::Zero;
    head = 0.f;
  }

  for ( std::size_t dogleg_idx = 0; dogleg_idx < doglegs.m_doglegs.size(); ++dogleg_idx )
    append_train( doglegs, doglegs.m_doglegs[dogleg_idx], head, end_lengths[dogleg_idx] );
}

float DoglegMarkerSystem::visible_length( const Cmp::Player::Doglegs::Dogleg &dogleg, const sf::FloatRect &view_bounds )
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
