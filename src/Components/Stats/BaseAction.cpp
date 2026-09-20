#include <Components/Stats/BaseAction.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <utility>

namespace Game::Cmp
{

BaseAction::BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick )
    : BaseAction( health, fear, despair, infamy, luck, tick, Cmp::Toxicity::Toxidrome{} )
{
}

BaseAction::BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick,
                         Cmp::Toxicity::Toxidrome toxidrome )
    : m_health( health.value ),
      m_fear( fear.value ),
      m_despair( despair.value ),
      m_infamy( infamy.value ),
      m_luck( luck.value ),
      m_toxidrome( std::make_unique<Cmp::Toxicity::Toxidrome>( std::move( toxidrome ) ) ),
      m_tick( tick.value )
{
}

BaseAction::~BaseAction() = default;

BaseAction::BaseAction( const BaseAction &other )
    : m_health( other.m_health ),
      m_fear( other.m_fear ),
      m_despair( other.m_despair ),
      m_infamy( other.m_infamy ),
      m_luck( other.m_luck ),
      m_toxidrome( std::make_unique<Cmp::Toxicity::Toxidrome>( *other.m_toxidrome ) ),
      m_tick( other.m_tick )
{
}

BaseAction &BaseAction::operator=( const BaseAction &other )
{
  if ( this == &other ) return *this;
  m_health = other.m_health;
  m_fear = other.m_fear;
  m_despair = other.m_despair;
  m_infamy = other.m_infamy;
  m_luck = other.m_luck;
  m_toxidrome = std::make_unique<Cmp::Toxicity::Toxidrome>( *other.m_toxidrome );
  m_tick = other.m_tick;
  return *this;
}

BaseAction::BaseAction( BaseAction &&other ) noexcept = default;
BaseAction &BaseAction::operator=( BaseAction &&other ) noexcept = default;

const Cmp::Toxicity::Toxidrome &BaseAction::toxidrome() const { return *m_toxidrome; }

} // namespace Game::Cmp
