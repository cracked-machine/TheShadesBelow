#ifndef SRC_FACTORY_TOXICITYFACTORY_HPP__
#define SRC_FACTORY_TOXICITYFACTORY_HPP__

#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/Toxicity/TraitsBase.hpp>
#include <cstddef>
#include <string>
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
// runtime-conditional code, must go through Cmp::Toxicity::Toxidrome::apply
// instead, which can only check exclusions at runtime.
template <typename... Added>
class ToxidromeBuilder
{
public:
  ToxidromeBuilder() = default;
  explicit ToxidromeBuilder( std::vector<int> deltas )
      : m_deltas( std::move( deltas ) )
  {
  }

  template <typename T>
  [[nodiscard]] auto add( int toxicity_delta = 0 ) const
  {
    static_assert( Cmp::Toxicity::no_conflicting_toxidromes_v<Added..., T>, "ToxidromeBuilder: two of the given toxidromes are mutually exclusive" );
    std::vector<int> next = m_deltas;
    next.push_back( toxicity_delta );
    return ToxidromeBuilder<Added..., T>( std::move( next ) );
  }

  [[nodiscard]] Cmp::Toxicity::Toxidrome build() const
  {
    Cmp::Toxicity::Toxidrome t;
    std::size_t i = 0;
    // store the signed deltas verbatim; exclusions were already checked by add<T>()
    ( t.template set<Added>( m_deltas[i++] ), ... );
    return t;
  }

private:
  std::vector<int> m_deltas;
};

//! @brief Build a single-affliction Toxidrome from a data-driven type name (e.g. loaded from JSON).
//! @param type Toxidrome type name; "none" yields an empty Toxidrome.
//!             "all" with a negative toxicity reduces every active toxidrome (a cure).
//! @param toxicity The toxidrome's toxicity contribution while active.
//! @return The constructed Cmp::Toxicity::Toxidrome.
//! @throws std::runtime_error on an invalid or no-op configuration: an unknown type, "none" with non-zero toxicity,
//!         a named type with zero toxicity, or "all" with non-negative toxicity.
Cmp::Toxicity::Toxidrome create_toxidrome( const std::string &type, int toxicity );

//! @brief Build a Toxidrome from a list of data-driven type names (e.g. a JSON "type" array).
//! @param types Toxidrome type names; a single name behaves exactly like create_toxidrome( const std::string &, int ).
//!              Several names only support a negative toxicity (a cure applied to each listed toxidrome), since the
//!              list may contain mutually exclusive toxidromes.
//! @param toxicity The toxicity delta applied to each listed toxidrome.
//! @return The constructed Cmp::Toxicity::Toxidrome.
//! @throws std::runtime_error on an invalid or no-op configuration: an empty list, a multi-type list with non-negative
//!         toxicity, or one containing "none", "all", an unknown or a duplicated type.
Cmp::Toxicity::Toxidrome create_toxidrome( const std::vector<std::string> &types, int toxicity );

} // namespace Game::Factory::Toxicity

#endif // SRC_FACTORY_TOXICITYFACTORY_HPP__
