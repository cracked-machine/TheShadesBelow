#ifndef SRC_FACTORY_TOXICITYFACTORY_HPP__
#define SRC_FACTORY_TOXICITYFACTORY_HPP__

#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/Toxicity/TraitsBase.hpp>
#include <cstddef>
#include <utility>
#include <vector>

namespace Game::Factory::Toxicity
{

// Accumulates a fixed, compile-time-known set of toxidromes (each with a
// runtime toxicity value) via chained add<T>(delta) calls, then builds a
// Cmp::Toxicity::Toxidrome from them. Each add<T>() fails to compile if T is
// mutually exclusive with any toxidrome already added earlier in the chain
// - this only works because the whole chain is one fixed sequence the
// compiler can see end-to-end (e.g. initial entity construction, or a JSON
// entry naming a single type). Toxidromes added later, from
// runtime-conditional code, must go through Cmp::Toxicity::Toxidrome::add
// instead, which can only check exclusions at runtime.
template <typename... Added>
class ToxidromeBuilder
{
public:
  ToxidromeBuilder() = default;
  explicit ToxidromeBuilder( std::vector<int> deltas ) : m_deltas( std::move( deltas ) ) {}

  template <typename T>
  [[nodiscard]] auto add( int toxicity_delta = 0 ) const
  {
    static_assert( Cmp::Toxicity::no_conflicting_toxidromes_v<Added..., T>,
                   "ToxidromeBuilder: two of the given toxidromes are mutually exclusive" );
    std::vector<int> next = m_deltas;
    next.push_back( toxicity_delta );
    return ToxidromeBuilder<Added..., T>( std::move( next ) );
  }

  [[nodiscard]] Cmp::Toxicity::Toxidrome build() const
  {
    Cmp::Toxicity::Toxidrome t;
    std::size_t i = 0;
    ( t.template add<Added>( m_deltas[i++] ), ... );
    return t;
  }

private:
  std::vector<int> m_deltas;
};

} // namespace Game::Factory::Toxicity

#endif // SRC_FACTORY_TOXICITYFACTORY_HPP__
