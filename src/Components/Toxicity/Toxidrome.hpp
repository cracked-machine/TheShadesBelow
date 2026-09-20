#ifndef SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__
#define SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__

#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/TraitsBase.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <algorithm>
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

  //! @brief Adds T with the given toxicity contribution.
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
  //! Toxidrome's active set element by element). Re-adding a type already active
  //! accumulates onto its existing toxicity contribution (e.g. repeated venom
  //! exposure keeps raising the venom meter), clamped to [0, 100] to match every
  //! other player stat.
  //!
  //! If T is excluded by an already-active toxidrome, toxicity_delta is instead
  //! spent fighting the excluding toxidrome(s) down towards zero (e.g. repeated
  //! tachycardia exposure wears down an active bradycardia). Once they're driven
  //! to zero and removed, the clash resolves and T takes over completely with
  //! whatever toxicity_delta remains; while any excluding toxidrome is still
  //! active, T stays out and this returns false.
  //! @param id
  //! @param toxicity_delta
  //! @return true
  //! @return false
  bool add( entt::id_type id, int toxicity_delta = 0 )
  {
    if ( is_excluded( id ) )
    {
      if ( toxicity_delta <= 0 ) return false;
      toxicity_delta = fight_exclusion( id, toxicity_delta );
      if ( is_excluded( id ) ) return false;
    }
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

  //! @brief Spends up to `delta` reducing T's own active toxicity value towards zero,
  //! removing T once it reaches zero. Returns whatever of `delta` wasn't needed for that
  //! (0 if T is still active afterwards, or wasn't active at all).
  //! @tparam T
  //! @param delta
  //! @return int
  template <typename T>
  int fight_one( int delta )
  {
    const auto it = m_active.find( entt::type_hash<T>::value() );
    if ( it == m_active.end() ) return 0;
    const int damage = std::min( delta, it->second );
    it->second -= damage;
    if ( it->second <= 0 ) m_active.erase( it );
    return delta - damage;
  }

  //! @brief Spends `delta` fighting down every active toxidrome in Ex, in order, carrying
  //! any leftover from one on to the next.
  //! @tparam Ex
  //! @param delta
  //! @return int
  template <typename... Ex>
  int fight_active( entt::exclude_t<Ex...> /*unused*/, int delta )
  {
    ( ( delta = fight_one<Ex>( delta ) ), ... );
    return delta;
  }

  //! @brief Finds the known toxidrome type Ts matching id and fights down whichever active
  //! toxidromes it excludes, given explicitly by fight_exclusion() below.
  //! @tparam Ts
  //! @param id
  //! @param delta
  //! @return int
  template <typename... Ts>
  int fight_exclusion_by( entt::id_type id, int delta )
  {
    ( ( entt::type_hash<Ts>::value() == id ? delta = fight_active( excluded_by<Ts>, delta ) : 0 ), ... );
    return delta;
  }

  //! @brief Spends `delta` fighting down whichever already-active toxidromes exclude id,
  //! removing any that are driven to zero. Returns whatever of `delta` is left over once
  //! they're all gone (0 if any excluding toxidrome is still active).
  //! @param id
  //! @param delta
  //! @return int
  int fight_exclusion( entt::id_type id, int delta )
  {
    return fight_exclusion_by<Bradycardia, Tachycardia, Hypoxia, Hallucinogen, Phototoxia, Venom>( id, delta );
  }

  // id -> the toxicity value it contributes while active.
  std::unordered_map<entt::id_type, int> m_active;
};

} // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__
