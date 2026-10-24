#include <Components/Particle/AshPileParticleSprite.hpp>

#include <Components/Random.hpp>
#include <SFML/System/Time.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace Game::Cmp::Particle
{

namespace
{
//! @brief Base greyscale channel value ash particles are drawn with, before per-particle variation.
constexpr int kAshColorBase = 20;
//! @brief Exclusive upper bound of the random colour variation added on top of kAshColorBase.
constexpr int kAshColorVariationMax = 20;
} // namespace

//! @brief Implementation detail — do not use externally
namespace detail
{
void AshPileParticle::emit()
{
  m_color_variation = Cmp::RandomInt( 0, kAshColorVariationMax ).gen();

  m_vertex.position = m_emitter_position;
  m_emitted_particle = true;
}
} // namespace detail

AshPileParticleSprite::AshPileParticleSprite()
    : SpriteBase( kMaxPopulation )
{
  if ( kBaseWidth % 2 != 0 ) throw std::logic_error( "kPyramidWidth must be even: simulate() fills outward from a centre pair of columns" );

  for ( int c = 0; c < kApexHeight; ++c )
  {
    m_columns[c] = std::vector<bool>( c + 1 );
  }
  for ( int c = 0; c < kApexHeight; ++c )
  {
    m_columns[kApexHeight + c] = std::vector<bool>( kApexHeight - c );
  }
}

size_t AshPileParticleSprite::filled_count( const std::vector<bool> &column )
{
  return static_cast<size_t>( std::count( column.begin(), column.end(), true ) );
}

void AshPileParticleSprite::place_particle( detail::AshPileParticle &particle )
{
  size_t col_index = Cmp::RandomInt( kBaseHalfWidth - 1, kBaseHalfWidth ).gen();
  bool going_left = col_index < static_cast<size_t>( kBaseHalfWidth );

  while ( true )
  {
    auto &column = m_columns.at( col_index );
    const size_t filled = filled_count( column );
    const bool column_full = filled == column.size();

    // Prefer a less-filled neighbour over stacking further onto this column, so the pile fills
    // outward in even rings instead of spiking the current column to full height first.
    const bool has_neighbour = going_left ? col_index > 0 : col_index + 1 < static_cast<size_t>( kBaseWidth );
    if ( has_neighbour )
    {
      const size_t neighbour_index = going_left ? col_index - 1 : col_index + 1;
      const auto &neighbour = m_columns.at( neighbour_index );
      const size_t neighbour_filled = filled_count( neighbour );

      if ( neighbour_filled < neighbour.size() and ( column_full or neighbour_filled < filled ) )
      {
        col_index = neighbour_index;
        continue;
      }
    }

    if ( not column_full )
    {
      // Fill from the base upward: the base row (column.size() - 1) is the same ground line for
      // every column, so the first particle in a column always lands there, and later ones climb
      // toward the apex as `filled` grows.
      const size_t row_index = column.size() - 1 - filled;
      column.at( row_index ) = true;
      // OpenGL/SFML draws downwards so we need to flip the shape by inverting the positions using the pyramid height
      const int row_offset = kApexHeight - static_cast<int>( column.size() );
      particle.m_vertex.position = particle.m_emitter_position +
                                   sf::Vector2f( static_cast<float>( col_index ), static_cast<float>( row_offset + static_cast<int>( row_index ) ) );
      break;
    }

    // This column is full and its only outward neighbour has no room either: the random
    // left/right split was uneven for this batch of particles — flip to the other half, which
    // is guaranteed to still have room since total slots == total particles.
    if ( going_left )
    {
      going_left = false;
      col_index = static_cast<size_t>( kBaseHalfWidth );
    }
    else
    {
      going_left = true;
      col_index = static_cast<size_t>( kBaseHalfWidth ) - 1;
    }
  }

  particle.m_vertex.color.a = 255;
  particle.m_vertex.color.r = kAshColorBase + particle.m_color_variation;
  particle.m_vertex.color.g = kAshColorBase + particle.m_color_variation;
  particle.m_vertex.color.b = kAshColorBase + particle.m_color_variation;
}

void AshPileParticleSprite::simulate( sf::Time dt )
{
  m_emit_delay_accumulator += dt;

  // Catch up rather than clamping to one placement per call: if the accumulator has built up more
  // than one timeout's worth (e.g. kEmitDelayTimeout set below a frame's dt), place that many
  // particles this call instead of silently discarding the surplus and capping the rate at 1/frame.
  while ( m_emit_delay_accumulator >= kEmitDelayTimeout )
  {
    m_emit_delay_accumulator -= kEmitDelayTimeout;

    bool placed_one = false;
    for ( auto &p : m_particles_list )
    {
      // keep looking until we find an empty pyramid index
      if ( not p.m_emitted_particle )
      {
        p.do_emit();
        // do_emit() no-ops instead of emitting once the sprite has been stop()'d, so m_emitted_particle
        // never gets set: without this check we'd re-find and re-place this same particle forever.
        if ( not p.m_particle_active ) break;
        placed_one = true;
        place_particle( p );
        break;
      }
    }

    if ( not placed_one ) break; // every particle already emitted, or the sprite has been stopped
  }
}

} // namespace Game::Cmp::Particle
