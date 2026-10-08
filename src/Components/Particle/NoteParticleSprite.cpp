#include <Components/Particle/NoteParticleSprite.hpp>
#include <Components/Random.hpp>

#include <SFML/System/Vector2.hpp>
#include <numbers>
#include <random>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
void NoteParticle::emit()
{
  static std::random_device rd;
  static std::mt19937 rng( rd() );

  m_wave_time = 0.f;

  m_phase = m_phase_range( rng );
  m_frequency = m_freq_range( rng );

  m_velocity = sf::Vector2f( 0.f, -m_speed_range( rng ) );

  // floored at 80 so notes stay vivid rather than landing on a muddy/near-black color
  constexpr int kColorFloor = 80;
  m_base_color = sf::Color( static_cast<std::uint8_t>( Cmp::RandomInt( kColorFloor, 255 ).gen() ),
                            static_cast<std::uint8_t>( Cmp::RandomInt( kColorFloor, 255 ).gen() ),
                            static_cast<std::uint8_t>( Cmp::RandomInt( kColorFloor, 255 ).gen() ) );
};
} // namespace detail

NoteParticleSprite::NoteParticleSprite( size_t count )
    : SpriteBase( count ) {};

void NoteParticleSprite::simulate( sf::Time dt )
{
  constexpr float amplitude = 5.f;

  for ( auto &p : m_particles_list )
  {
    p.m_lifetime -= dt;
    p.m_wave_time += dt.asSeconds();
    const float ratio = p.m_lifetime.asSeconds() / m_max_lifetime.asSeconds();
    if ( p.m_lifetime <= sf::Time::Zero and not try_emit( p ) ) continue;

    const float wave_x = amplitude * std::sin( ( 2.f * std::numbers::pi_v<float> * p.m_frequency * p.m_wave_time ) + p.m_phase );

    p.m_vertex.position += sf::Vector2f{ wave_x, p.m_velocity.y } * dt.asSeconds();

    p.m_vertex.color = p.m_base_color;
    p.m_vertex.color.a = static_cast<std::uint8_t>( ratio * 255 );
  }
}

namespace
{

// notehead: filled rounded polygon via triangle-fan wedges (same technique as SpriteBase::draw())
constexpr int kHeadSides = 10;
constexpr float kHeadRadius = 1.5f;
// stem: thin quad rising from each head's right edge
constexpr float kStemWidth = 0.6f;
constexpr float kStemHeight = 4.f;
// distance between the centres of adjacent noteheads
constexpr float kHeadSpacing = 4.f;
// beam: horizontal quad joining the tops of the stems
constexpr float kBeamThickness = 1.f;
// black border grown outward around every piece, drawn first so it peeks out from behind the fill
constexpr float kOutlineThickness = 4.f;

//! @brief A row of head/stem pairs centred on a point, joined by a beam when there is more than one.
struct NoteGlyph
{
  int m_heads;

  //! @brief Append the glyph's triangles, with every edge grown outward by `inflate` screen px.
  void push( std::vector<sf::Vertex> &verts, sf::Vector2f pos, float scale, float inflate, sf::Color col ) const
  {
    auto push_quad = [&]( float left, float top, float right, float bottom )
    {
      verts.push_back( { { left, bottom }, col } );
      verts.push_back( { { right, bottom }, col } );
      verts.push_back( { { right, top }, col } );
      verts.push_back( { { left, bottom }, col } );
      verts.push_back( { { right, top }, col } );
      verts.push_back( { { left, top }, col } );
    };

    constexpr float kAngleStep = 2.f * std::numbers::pi_v<float> / static_cast<float>( kHeadSides );
    const float head_radius = ( kHeadRadius * scale ) + inflate;
    const float half_width = ( kStemWidth * scale * 0.5f ) + inflate;
    const float stem_top_y = pos.y - ( kStemHeight * scale );
    const float first_stem_x = pos.x - ( kHeadSpacing * scale * 0.5f * static_cast<float>( m_heads - 1 ) ) + ( kHeadRadius * scale * 0.85f );
    const float last_stem_x = first_stem_x + ( kHeadSpacing * scale * static_cast<float>( m_heads - 1 ) );

    for ( int h = 0; h < m_heads; ++h )
    {
      const float stem_x = first_stem_x + ( kHeadSpacing * scale * static_cast<float>( h ) );
      const sf::Vector2f head( stem_x - ( kHeadRadius * scale * 0.85f ), pos.y );
      for ( int i = 0; i < kHeadSides; ++i )
      {
        verts.push_back( { head, col } );
        verts.push_back( { head + sf::Vector2f( head_radius, sf::radians( kAngleStep * static_cast<float>( i ) ) ), col } );
        verts.push_back( { head + sf::Vector2f( head_radius, sf::radians( kAngleStep * static_cast<float>( i + 1 ) ) ), col } );
      }
      push_quad( stem_x - half_width, stem_top_y - inflate, stem_x + half_width, pos.y + inflate );
    }

    // beam: spans the outer edges of the outermost stems, its top flush with the stem tops
    if ( m_heads > 1 )
    {
      push_quad( first_stem_x - half_width, stem_top_y - inflate, last_stem_x + half_width, stem_top_y + ( kBeamThickness * scale ) + inflate );
    }
  }
};

constexpr NoteGlyph kEighthNotes{ 2 };
constexpr NoteGlyph kQuarterNote{ 1 };

} // namespace

void NoteParticleSprite::draw( sf::RenderTarget &target, sf::RenderStates states ) const
{
  states.texture = nullptr;
  states.blendMode = sf::BlendAlpha;

  std::vector<sf::Vertex> verts;
  verts.reserve( m_particles_list.size() * 114 );
  SPDLOG_DEBUG( "Drawing {} particles", m_particles_list.size() );

  for ( size_t i = 0; i < m_particles_list.size(); ++i )
  {
    const auto &p = m_particles_list[i];
    // alternates per particle and again on every respawn, so even a single particle shows both glyphs
    const auto &glyph = ( ( i + p.m_generation ) % 2 == 0 ) ? kEighthNotes : kQuarterNote;
    const auto pos = m_world_to_screen( p.m_vertex.position );
    const auto colour = p.m_vertex.color;

    glyph.push( verts, pos, m_world_to_screen_scale, kOutlineThickness, sf::Color( 0, 0, 0, colour.a ) );
    glyph.push( verts, pos, m_world_to_screen_scale, 0.f, colour );
  }

  target.draw( verts.data(), verts.size(), sf::PrimitiveType::Triangles, states );
}

} // namespace Game::Cmp::Particle
