#ifndef SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__
#define SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__

#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/TachyCadia.hpp>
#include <Components/Toxicity/TraitsBase.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <algorithm>
#include <cassert>
#include <entt/core/type_info.hpp>
#include <entt/fwd.hpp>
#include <optional>
#include <unordered_map>

namespace Game::Cmp::Toxicity
{

//! @brief Holds the set of toxidrome effects currently active on an entity, each with
//! the toxicity value it contributes while active.

class Toxidrome
{
public:
  template <typename T>
  [[nodiscard]] bool has() const
  {
    return m_active.contains( entt::type_hash<T>::value() );
  }

  //! @brief Adds T with the given toxicity contribution. Asserts (aborts in debug
  //! builds) if T is excluded by a toxidrome already active, since that can
  //! only be a caller bug: unlike a fixed, compile-time-known sequence (see
  //! Factory::Toxicity::ToxidromeBuilder), an add() reached from gameplay
  //! code depends on runtime state the compiler can't see, so it can't be
  //! checked at compile time. When asserts are compiled out (NDEBUG), the
  //! add is still refused so release builds never end up with two mutually
  //! exclusive toxidromes active.
  //! @tparam T
  //! @param toxicity_delta
  //! @return true
  //! @return false
  template <typename T>
  bool add( int toxicity_delta = 0 )
  {
    return add( entt::type_hash<T>::value(), toxicity_delta );
  }

  //! @brief Runtime-id counterpart of add<T>(), for merging in a set of toxidromes
  //! whose concrete types aren't known until iteration (e.g. copying another
  //! Toxidrome's active set element by element). Same exclusion semantics.
  //! Re-adding a type already active accumulates onto its existing toxicity
  //! contribution (e.g. repeated venom exposure keeps raising the venom
  //! meter), clamped to [0, 100] to match every other player stat.
  //! @param id
  //! @param toxicity_delta
  //! @return true
  //! @return false
  bool add( entt::id_type id, int toxicity_delta = 0 )
  {
    const bool excluded = is_excluded( id );
    assert( !excluded && "Toxidrome::add: type excluded by an already-active toxidrome" );
    if ( excluded ) return false;
    auto it = m_active.find( id );
    if ( it == m_active.end() )
      m_active.emplace( id, std::clamp( toxicity_delta, 0, 100 ) );
    else
      it->second = std::clamp( it->second + toxicity_delta, 0, 100 );
    return true;
  }

  //! @brief Remove the toxidrome effect from the list
  //! @tparam T
  template <typename T>
  void remove()
  {
    m_active.erase( entt::type_hash<T>::value() );
  }

  //! @brief Reduces every active toxidrome's own toxicity contribution by up to `amount`
  //!        (e.g. from healing over time), removing any that reach zero. Returns the total
  //!        actually removed, summed across all active toxidromes.
  //! @param amount
  //! @return int
  int decay( int amount )
  {
    int total_removed = 0;
    for ( auto it = m_active.begin(); it != m_active.end(); )
    {
      const int removed = std::min( amount, it->second );
      it->second -= removed;
      total_removed += removed;
      if ( it->second <= 0 )
        it = m_active.erase( it );
      else
        ++it;
    }
    return total_removed;
  }

  [[nodiscard]] auto begin() const { return m_active.begin(); }
  [[nodiscard]] auto end() const { return m_active.end(); }

  //! @brief Look up the toxicity level for the toxidrome T
  //! @tparam T The toxidrome type to look up
  //! @return std::optional<int>
  template <typename T>
  [[nodiscard]] std::optional<int> at() const
  {
    const auto it = m_active.find( entt::type_hash<T>::value() );
    if ( it == m_active.end() ) return std::nullopt;
    return it->second;
  }

private:
  //! @brief True if any currently active toxidrome excludes Ex.
  //! @tparam Ex
  template <typename... Ex>
  [[nodiscard]] bool excludes_active( entt::exclude_t<Ex...> /*unused*/ ) const
  {
    return ( m_active.contains( entt::type_hash<Ex>::value() ) || ... );
  }

  //! @brief Checks id against every known toxidrome tag type Ts, given explicitly by
  //! is_excluded() below, and tests the matching type's own excludes against m_active.
  //! @tparam Ts
  //! @param id
  //! @return true
  //! @return false
  template <typename... Ts>
  [[nodiscard]] bool is_excluded_by( entt::id_type id ) const
  {
    return ( ( entt::type_hash<Ts>::value() == id && excludes_active( excluded_by<Ts> ) ) || ... );
  }

  //! @brief Checks if the given toxidrome type id is excluded by an already-active toxidrome.
  //! @param id The hashed type id to check
  //! @return true
  //! @return false
  [[nodiscard]] bool is_excluded( entt::id_type id ) const
  {
    return is_excluded_by<Bradycardia, Tachycardia, Hypoxia, Hallucinogen, Phototoxia, Venom>( id );
  }

  // id -> the toxicity value it contributes while active.
  std::unordered_map<entt::id_type, int> m_active;
};

} // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__
