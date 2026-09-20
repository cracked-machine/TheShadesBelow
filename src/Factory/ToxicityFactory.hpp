#ifndef SRC_FACTORY_TOXICITYFACTORY_HPP__
#define SRC_FACTORY_TOXICITYFACTORY_HPP__

#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/Toxicity/TraitsBase.hpp>

namespace Game::Factory::Toxicity
{

// Accumulates a fixed, compile-time-known set of toxidromes via chained
// add<T>() calls, then builds a Cmp::Toxidrome::Toxidromes from them.
// Each add<T>() fails to compile if T is mutually exclusive with any
// toxidrome already added earlier in the chain — this only works because
// the whole chain is one fixed sequence the compiler can see end-to-end
// (e.g. initial entity construction). Toxidromes added later, from
// runtime-conditional code, must go through Cmp::Toxidrome::Toxidromes::add
// instead, which can only check exclusions at runtime.
template <typename... Added>
class ToxidromeBuilder
{
public:
  template <typename T>
  [[nodiscard]] constexpr auto add() const
  {
    static_assert( Cmp::Toxicity::no_conflicting_toxidromes_v<Added..., T>, "ToxidromeBuilder: two of the given toxidromes are mutually exclusive" );
    return ToxidromeBuilder<Added..., T>{};
  }

  [[nodiscard]] Cmp::Toxicity::Toxidrome build() const
  {
    Cmp::Toxicity::Toxidrome t;
    ( t.template add<Added>(), ... );
    return t;
  }
};

} // namespace Game::Factory::Toxicity

#endif // SRC_FACTORY_TOXICITYFACTORY_HPP__
