#include <Components/Random.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <utility>

namespace Game::Cmp
{

bool SpawnNpcEffect::roll() const { return Cmp::RandomFloat( 0.f, 1.f ).gen() < probability; }
bool LightningEffect::roll() const { return Cmp::RandomFloat( 0.f, 1.f ).gen() < probability; }
bool MovementEffect::roll() const { return Cmp::RandomFloat( 0.f, 1.f ).gen() < probability; }

BaseAction::BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick )
    : BaseAction( health, fear, despair, infamy, luck, tick, Cmp::Toxicity::Toxidrome{} )
{
}

BaseAction::BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick,
                         Cmp::Toxicity::Toxidrome toxidrome, SideEffects side_effects )
    : m_health( health.value ),
      m_fear( fear.value ),
      m_despair( despair.value ),
      m_infamy( infamy.value ),
      m_luck( luck.value ),
      m_toxidrome( std::make_unique<Cmp::Toxicity::Toxidrome>( std::move( toxidrome ) ) ),
      m_side_effects( std::move( side_effects ) ),
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
      m_side_effects( other.m_side_effects ),
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
  m_side_effects = other.m_side_effects;
  m_tick = other.m_tick;
  return *this;
}

BaseAction::BaseAction( BaseAction &&other ) noexcept = default;
BaseAction &BaseAction::operator=( BaseAction &&other ) noexcept = default;

const Cmp::Toxicity::Toxidrome &BaseAction::toxidrome() const { return *m_toxidrome; }

bool BaseAction::is_zero() const
{
  return m_health == 0 and m_fear == 0 and m_despair == 0 and m_infamy == 0 and m_luck == 0 and m_toxidrome->begin() == m_toxidrome->end() and
         m_side_effects.empty();
}

} // namespace Game::Cmp
