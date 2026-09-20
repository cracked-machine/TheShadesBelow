#ifndef SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__
#define SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__

#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/TachyCadia.hpp>
#include <Components/Toxicity/TraitsBase.hpp>
#include <cassert>
#include <entt/entt.hpp>
#include <unordered_map>
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
    return add( entt::type_hash<T>::value() );
  }

  // Runtime-id counterpart of add<T>(), for merging in a set of toxidromes
  // whose concrete types aren't known until iteration (e.g. copying another
  // Toxidrome's active set element by element). Same exclusion semantics.
  bool add( entt::id_type id )
  {
    const bool excluded = is_excluded( id );
    assert( !excluded && "Toxidrome::add: type excluded by an already-active toxidrome" );
    if ( excluded ) return false;
    m_active.insert( id );
    return true;
  }

  template <typename T>
  void remove()
  {
    m_active.erase( entt::type_hash<T>::value() );
  }

  [[nodiscard]] auto begin() const { return m_active.begin(); }
  [[nodiscard]] auto end() const { return m_active.end(); }

private:
  // Every known toxidrome tag type, used only to build exclusion_table() below.
  template <typename... Ts>
  struct type_list
  {
  };
  using AllToxidromes = type_list<Bradycardia, Tachycardia, Hypoxia, Hallucinogen, Phototoxia>;

  template <typename T, typename... Ex>
  static void register_excludes( std::unordered_map<entt::id_type, std::unordered_set<entt::id_type>> &table, entt::exclude_t<Ex...> /*unused*/ )
  {
    table[entt::type_hash<T>::value()] = { entt::type_hash<Ex>::value()... };
  }

  template <typename... Ts>
  static std::unordered_map<entt::id_type, std::unordered_set<entt::id_type>> build_exclusion_table( type_list<Ts...> /*unused*/ )
  {
    std::unordered_map<entt::id_type, std::unordered_set<entt::id_type>> table;
    ( register_excludes<Ts>( table, excluded_by<Ts> ), ... );
    return table;
  }

  // id -> the set of ids it excludes, derived once from every type's toxidrome_traits.
  static const std::unordered_map<entt::id_type, std::unordered_set<entt::id_type>> &exclusion_table()
  {
    static const auto table = build_exclusion_table( AllToxidromes{} );
    return table;
  }

  [[nodiscard]] bool is_excluded( entt::id_type id ) const
  {
    const auto &table = exclusion_table();
    const auto it = table.find( id );
    if ( it == table.end() ) return false;
    for ( entt::id_type excluded_id : it->second )
    {
      if ( m_active.contains( excluded_id ) ) return true;
    }
    return false;
  }

  std::unordered_set<entt::id_type> m_active;
};

} // namespace Game::Cmp::Toxicity

#endif // SRC_CMPS_TOXIDROME_TOXIDROMES_HPP__
