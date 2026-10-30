#include <Components/Random.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <utility>

namespace Game::Cmp
{

namespace
{
//! @brief Chance [0, 1] of an effect firing: luck raises the odds of good effects and lowers the odds of bad ones.
float luck_chance( Probability probability, int player_luck, bool good_for_player )
{
  if ( probability == Probability::ALWAYS ) return 1.f;
  const float luck = static_cast<float>( player_luck ) / 100.f;
  return good_for_player ? luck : 1.f - luck;
}
} // namespace

float SpawnNpcEffect::chance( int player_luck ) const { return luck_chance( probability, player_luck, /*good_for_player=*/false ); }
float LightningEffect::chance( int player_luck ) const { return luck_chance( probability, player_luck, /*good_for_player=*/false ); }
// npc-only effects (slow/drunk/repel) help the player; anything touching the player's own movement hinders them
float MovementEffect::chance( int player_luck ) const { return luck_chance( probability, player_luck, /*good_for_player=*/not player ); }

bool SpawnNpcEffect::roll( int player_luck ) const { return Cmp::RandomFloat( 0.f, 1.f ).gen() < chance( player_luck ); }
bool LightningEffect::roll( int player_luck ) const { return Cmp::RandomFloat( 0.f, 1.f ).gen() < chance( player_luck ); }
bool MovementEffect::roll( int player_luck ) const { return Cmp::RandomFloat( 0.f, 1.f ).gen() < chance( player_luck ); }

BaseAction::BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick )
    : BaseAction( health, fear, despair, infamy, luck, tick, Cmp::Toxicity::Toxidrome{} )
{
}

BaseAction::BaseAction( Stats::Health health, Stats::Fear fear, Stats::Despair despair, Stats::Infamy infamy, Stats::Luck luck, Stats::Tick tick,
                        Cmp::Toxicity::Toxidrome toxidrome, SideEffects side_effects, ActionRewards rewards )
    : m_health( health.value ),
      m_fear( fear.value ),
      m_despair( despair.value ),
      m_infamy( infamy.value ),
      m_luck( luck.value ),
      m_toxidrome( std::make_unique<Cmp::Toxicity::Toxidrome>( std::move( toxidrome ) ) ),
      m_side_effects( std::move( side_effects ) ),
      m_rewards( std::move( rewards ) ),
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
      m_rewards( other.m_rewards ),
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
  m_rewards = other.m_rewards;
  m_tick = other.m_tick;
  return *this;
}

BaseAction::BaseAction( BaseAction &&other ) noexcept = default;
BaseAction &BaseAction::operator=( BaseAction &&other ) noexcept = default;

const Cmp::Toxicity::Toxidrome &BaseAction::toxidrome() const { return *m_toxidrome; }

std::optional<ActionReward> BaseAction::reward() const
{
  if ( m_rewards.empty() ) return std::nullopt;
  return m_rewards.front();
}

std::optional<ActionReward> BaseAction::reward( Altar::Kind location ) const
{
  std::optional<ActionReward> any_location;
  for ( const auto &reward : m_rewards )
  {
    if ( reward.location == location ) return reward;
    if ( not reward.location ) any_location = reward;
  }
  return any_location;
}

bool BaseAction::is_zero() const
{
  return m_health == 0 and m_fear == 0 and m_despair == 0 and m_infamy == 0 and m_luck == 0 and m_toxidrome->begin() == m_toxidrome->end() and
         m_side_effects.empty() and m_rewards.empty();
}

} // namespace Game::Cmp
