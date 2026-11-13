#include <Systems/Stores/StoreSingleton.hpp>

#include <Components/Random.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>

#include <nlohmann/json.hpp>
#include <regex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <utility>

namespace Game::Sys
{

namespace Detail
{

std::function<bool( const std::string & )> make_pattern_matcher( const std::string &pattern )
{
  try
  {
    // Try to use as regex first
    return [pattern_regex = std::regex( pattern )]( const std::string &s ) { return std::regex_search( s, pattern_regex ); };
  } catch ( const std::regex_error &e )
  {
    // If regex fails, fallback to substring matching
    SPDLOG_DEBUG( "Pattern '{}' is not valid regex, using substring matching", pattern );
    return [pattern]( const std::string &s ) { return s.find( pattern ) != std::string::npos; };
  }
}

void rethrow_with_context( const std::filesystem::path &json_file_path, const std::string &owner_key, const std::string &action_key,
                           const nlohmann::json &j, const std::exception &e )
{
  throw std::runtime_error( json_file_path.string() + ": '" + owner_key + "' " + action_key + " " + j.dump() + ": " + e.what() );
}

} // namespace Detail

namespace
{

//! @brief Throw if a get_random() candidate list is empty.
void require_non_empty( std::size_t size, const std::source_location &loc )
{
  if ( size != 0 ) return;
  throw std::runtime_error( std::string( "Cannot pick a random type from an empty list. Called from " ) + loc.file_name() + ":" +
                            std::to_string( loc.line() ) + " in '" + loc.function_name() + "'" );
}

} // namespace

template <typename Derived, typename StoreValue>
Derived *StoreSingleton<Derived, StoreValue>::s_instance = nullptr;

template <typename Derived, typename StoreValue>
StoreSingleton<Derived, StoreValue>::StoreSingleton( std::filesystem::path json_file_path )
    : m_json_file_path( std::move( json_file_path ) )
{
  if ( s_instance ) throw std::runtime_error( "Store already initialized" );
  s_instance = static_cast<Derived *>( this );
}

template <typename Derived, typename StoreValue>
StoreSingleton<Derived, StoreValue>::~StoreSingleton()
{
  if ( s_instance == static_cast<Derived *>( this ) ) s_instance = nullptr;
}

template <typename Derived, typename StoreValue>
const StoreValue &StoreSingleton<Derived, StoreValue>::get( const key_type &type, std::source_location loc ) const
{
  auto it = m_store.find( type );
  if ( it != m_store.end() ) return it->second;

  throw std::runtime_error( "Failed to locate '" + type.str() + "' in store. Called from " + loc.file_name() + ":" + std::to_string( loc.line() ) +
                            " in '" + loc.function_name() + "'" );
}

template <typename Derived, typename StoreValue>
std::vector<typename StoreSingleton<Derived, StoreValue>::key_type> StoreSingleton<Derived, StoreValue>::get_all() const
{
  std::vector<key_type> types;
  types.reserve( m_store.size() );
  for ( const auto &[type, _] : m_store )
  {
    if ( type == "ERROR_SPRITE" ) continue;
    types.push_back( type );
  }
  return types;
}

template <typename Derived, typename StoreValue>
std::vector<typename StoreSingleton<Derived, StoreValue>::key_type> StoreSingleton<Derived, StoreValue>::get_pattern( const std::string &pattern ) const
{
  const auto matches = Detail::make_pattern_matcher( pattern );
  std::vector<key_type> types;
  for ( const auto &[type, _] : m_store )
  {
    if ( matches( type.str() ) ) types.push_back( type );
  }
  return types;
}

template <typename Derived, typename StoreValue>
const StoreValue &StoreSingleton<Derived, StoreValue>::get_random( const std::vector<key_type> &list, std::source_location loc ) const
{
  require_non_empty( list.size(), loc );
  const int pick = Cmp::RandomInt( 0, static_cast<int>( list.size() ) - 1 ).gen();
  return get( list.at( static_cast<std::size_t>( pick ) ), loc );
}

template <typename Derived, typename StoreValue>
const StoreValue &StoreSingleton<Derived, StoreValue>::get_random( int player_luck_stat, const std::vector<key_type> &list,
                                                                   std::source_location loc ) const
{
  require_non_empty( list.size(), loc );

  // mirrors GraveSystem::trigger_grave_consequence. The list is split into a "bad" front half and a
  // "good" back half, and luck weights which half gets picked (uniformly within the half). At luck 0
  // only the bad half is reachable, at luck 100 only the good half, and at luck 50 it reduces to a
  // plain uniform pick across the whole list.
  const int good_weight = player_luck_stat;
  const int bad_weight = 100 - good_weight;

  const std::size_t bad_count = list.size() / 2;
  const std::size_t good_count = list.size() - bad_count;

  const int half_roll = Cmp::RandomInt( 0, 99 ).gen();
  const bool pick_bad_half = bad_count > 0 && half_roll < bad_weight;

  const std::size_t start = pick_bad_half ? 0 : bad_count;
  const std::size_t count = pick_bad_half ? bad_count : good_count;

  const int index_roll = Cmp::RandomInt( 0, static_cast<int>( count ) - 1 ).gen();
  return get( list.at( start + static_cast<std::size_t>( index_roll ) ), loc );
}

template <typename Derived, typename StoreValue>
void StoreSingleton<Derived, StoreValue>::rethrow_with_context( const std::string &owner_key, const std::string &action_key, const nlohmann::json &j,
                                                                const std::exception &e ) const
{
  Detail::rethrow_with_context( m_json_file_path, owner_key, action_key, j, e );
}

// The closed set of stores. Each has a matching `extern template` after its class in its own header.
template class StoreSingleton<SpriteStore, Sprites::SpriteSheet>;
template class StoreSingleton<ItemStore, Cmp::WorldItem>;
template class StoreSingleton<NpcStore, Cmp::Npc::NPC>;

} // namespace Game::Sys
