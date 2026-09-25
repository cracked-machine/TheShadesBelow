#include <Components/Stats/PlayerStats.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <spdlog/spdlog.h>
#include <utility>

namespace Game::Cmp
{

PlayerStats::PlayerStats( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck )
    : PlayerStats( health, fear, despair, infamy, luck, Cmp::Toxicity::Toxidrome{} )
{
}

PlayerStats::PlayerStats( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck,
                          Cmp::Toxicity::Toxidrome toxidrome )
    : m_health( std::clamp( health.value, 0, 100 ) ),
      m_fear( std::clamp( fear.value, 0, 100 ) ),
      m_despair( std::clamp( despair.value, 0, 100 ) ),
      m_infamy( std::clamp( infamy.value, 0, 100 ) ),
      m_luck( std::clamp( luck.value, 0, 100 ) ),
      m_toxidrome( std::make_unique<Cmp::Toxicity::Toxidrome>( std::move( toxidrome ) ) )
{
}

PlayerStats::~PlayerStats() = default;

PlayerStats::PlayerStats( const PlayerStats &other )
    : m_health( other.m_health ),
      m_fear( other.m_fear ),
      m_despair( other.m_despair ),
      m_infamy( other.m_infamy ),
      m_luck( other.m_luck ),
      m_toxidrome( std::make_unique<Cmp::Toxicity::Toxidrome>( *other.m_toxidrome ) )
{
}

PlayerStats &PlayerStats::operator=( const PlayerStats &other )
{
  if ( this == &other ) return *this;
  m_health = other.m_health;
  m_fear = other.m_fear;
  m_despair = other.m_despair;
  m_infamy = other.m_infamy;
  m_luck = other.m_luck;
  m_toxidrome = std::make_unique<Cmp::Toxicity::Toxidrome>( *other.m_toxidrome );
  return *this;
}

PlayerStats::PlayerStats( PlayerStats &&other ) noexcept = default;
PlayerStats &PlayerStats::operator=( PlayerStats &&other ) noexcept = default;

const Cmp::Toxicity::Toxidrome &PlayerStats::toxidrome() const { return *m_toxidrome; }

void PlayerStats::decay_all_toxidrome( int amount ) { m_toxidrome->decay_all( amount ); }

void PlayerStats::apply( const BaseAction &action )
{
  m_health = std::clamp( m_health + action.health(), 0, 100 );
  m_fear = std::clamp( m_fear + action.fear(), 0, 100 );
  m_despair = std::clamp( m_despair + action.despair(), 0, 100 );
  m_infamy = std::clamp( m_infamy + action.infamy(), 0, 100 );
  m_luck = std::clamp( m_luck + action.luck(), 0, 100 );
  for ( const auto &[id, toxicity_delta] : action.toxidrome() )
  {
    if ( m_toxidrome->add( id, toxicity_delta ) ) { SPDLOG_INFO( "Added {} toxicity", toxicity_delta ); }
  }
}

} // namespace Game::Cmp
