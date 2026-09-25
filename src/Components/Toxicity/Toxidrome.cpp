#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Toxidrome.hpp>
#include <Components/Toxicity/Venom.hpp>

namespace Game::Cmp::Toxicity
{

bool Toxidrome::add( entt::id_type id, int toxicity_delta )
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

int Toxidrome::decay( int amount )
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

template <typename... Ex>
[[nodiscard]] bool Toxidrome::excludes_active( entt::exclude_t<Ex...> /*unused*/ ) const
{
  return ( m_active.contains( entt::type_hash<Ex>::value() ) || ... );
}

template <typename... Ts>
[[nodiscard]] bool Toxidrome::is_excluded_by( entt::id_type id ) const
{
  return ( ( entt::type_hash<Ts>::value() == id && excludes_active( excluded_by<Ts> ) ) || ... );
}

[[nodiscard]] bool Toxidrome::is_excluded( entt::id_type id ) const
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
int Toxidrome::fight_one( int delta )
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
int Toxidrome::fight_active( entt::exclude_t<Ex...> /*unused*/, int delta )
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
int Toxidrome::fight_exclusion_by( entt::id_type id, int delta )
{
  ( ( entt::type_hash<Ts>::value() == id ? delta = fight_active( excluded_by<Ts>, delta ) : 0 ), ... );
  return delta;
}

int Toxidrome::fight_exclusion( entt::id_type id, int delta )
{
  return fight_exclusion_by<Bradycardia, Tachycardia, Hypoxia, Hallucinogen, Phototoxia, Venom>( id, delta );
}

} // namespace Game::Cmp::Toxicity