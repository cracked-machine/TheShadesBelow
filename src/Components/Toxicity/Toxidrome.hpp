#ifndef SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__
#define SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__

#include <Components/Toxicity/TraitsBase.hpp>
#include <cassert>
#include <entt/entt.hpp>
#include <unordered_set>

namespace Game::Cmp::Toxicity
{

// Holds the set of toxidrome effects currently active on an entity.
class Toxidrome
{
public:
  template <typename T>
  [[nodiscard]] bool has() const
  {
    return m_active.contains( entt::type_hash<T>::value() );
  }

  // Adds T. Asserts (aborts in debug builds) if T is excluded by a toxidrome
  // already active, since that can only be a caller bug: unlike a fixed,
  // compile-time-known sequence (see Factory::Toxicity::ToxidromeBuilder),
  // an add() reached from gameplay code depends on runtime state the
  // compiler can't see, so it can't be checked at compile time. When
  // asserts are compiled out (NDEBUG), the add is still refused so release
  // builds never end up with two mutually exclusive toxidromes active.
  template <typename T>
  bool add()
  {
    const bool excluded = is_excluded<T>();
    assert( !excluded && "Toxidromes::add: type excluded by an already-active toxidrome" );
    if ( excluded ) return false;
    m_active.insert( entt::type_hash<T>::value() );
    return true;
  }

  template <typename T>
  void remove()
  {
    m_active.erase( entt::type_hash<T>::value() );
  }

private:
  template <typename... Ex>
  bool any_active( entt::exclude_t<Ex...> /*unused*/ ) const
  {
    return ( has<Ex>() || ... );
  }

  template <typename T>
  bool is_excluded() const
  {
    return any_active( excluded_by<T> );
  }

  std::unordered_set<entt::id_type> m_active;
};

} // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__
