#include <Systems/Crypt/Variants/CryptVariantArbiter.hpp>

#include <Components/Random.hpp>
#include <Systems/Crypt/Variants/ClassicCryptVariant.hpp>
#include <Systems/Crypt/Variants/OpenCryptVariant.hpp>

#include <spdlog/spdlog.h>

#include <stdexcept>

namespace Game::Sys::Crypt
{

CryptVariantArbiter CryptVariantArbiter::with_default_variants()
{
  CryptVariantArbiter arbiter;
  arbiter.register_variant<ClassicCryptVariant>( "Classic", 3 ); // index 0
  arbiter.register_variant<OpenCryptVariant>( "Open", 1 );       // index 1
  return arbiter;
}

std::unique_ptr<CryptVariant> CryptVariantArbiter::pick( CryptSystem &crypt, PassageSystem &passages, int override_idx ) const
{
  if ( m_entries.empty() ) throw std::runtime_error( "CryptVariantArbiter::pick - no crypt variants registered" );

  const Entry *chosen = nullptr;
  if ( override_idx >= 0 and static_cast<std::size_t>( override_idx ) < m_entries.size() )
  {
    chosen = &m_entries[static_cast<std::size_t>( override_idx )];
    SPDLOG_INFO( "Crypt variant forced by override: {} ({})", chosen->name, override_idx );
  }
  else
  {
    if ( override_idx >= 0 ) SPDLOG_WARN( "Crypt variant override {} out of range (0-{}), picking at random", override_idx, m_entries.size() - 1 );

    unsigned total_weight = 0;
    for ( const auto &entry : m_entries )
      total_weight += entry.weight;

    if ( total_weight == 0 ) { chosen = &m_entries.front(); }
    else
    {
      auto roll = static_cast<unsigned>( Cmp::RandomInt( 0, static_cast<int>( total_weight ) - 1 ).gen() );
      for ( const auto &entry : m_entries )
      {
        if ( roll < entry.weight )
        {
          chosen = &entry;
          break;
        }
        roll -= entry.weight;
      }
    }
    SPDLOG_INFO( "Crypt variant picked at random: {}", chosen->name );
  }

  return chosen->make( crypt, passages );
}

} // namespace Game::Sys::Crypt
