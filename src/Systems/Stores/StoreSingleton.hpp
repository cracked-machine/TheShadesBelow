#ifndef SRC_SYSTEMS_STORES_STORESINGLETON_HPP__
#define SRC_SYSTEMS_STORES_STORESINGLETON_HPP__

#include <Components/Random.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Game::Sys
{

//! @brief CRTP base for singleton, string-keyed data stores. Gives each Derived store class its own static
//! instance and a shared map of string id to StoreValue, plus lookup helpers. Stores are plain data holders
//! (not systems): they are owned by Sys::Store, loaded once from a JSON file, and accessed via instance().
//! @tparam Derived
//! @tparam StoreValue
template <typename Derived, typename StoreValue>
class StoreSingleton
{
public:
  //! @brief Clears the singleton instance pointer.
  ~StoreSingleton() { s_instance = nullptr; }

  //! @brief Stores are non-copyable and non-movable: s_instance points at the single live object.
  StoreSingleton( const StoreSingleton & ) = delete;
  StoreSingleton( StoreSingleton && ) = delete;
  StoreSingleton &operator=( const StoreSingleton & ) = delete;
  StoreSingleton &operator=( StoreSingleton && ) = delete;

  //! @brief String-keyed map type used to hold this store's loaded StoreValue entries.
  using store_map = std::unordered_map<std::string, StoreValue>;

  //! @brief Get the singleton instance of the Derived store.
  //! @return Reference to the Derived store instance.
  //! @throws std::runtime_error if the store has not yet been initialized.
  static Derived &instance()
  {
    if ( not s_instance ) throw std::runtime_error( "Store not yet initialized" );
    return *s_instance;
  }

  //! @brief Look up a stored value by its string key.
  //! @param key
  //! @return A copy of the stored value.
  //! @throws std::runtime_error if the key is not present in the store.
  [[nodiscard]] StoreValue get_item( const std::string &key ) const
  {
    auto it = m_store.find( key );
    if ( it == m_store.end() ) throw std::runtime_error( "Unknown key: " + key );
    return it->second;
  }

  //! @brief Get all keys in the store, excluding the "ERROR_SPRITE" sentinel key.
  //! @return The list of keys.
  [[nodiscard]] std::vector<std::string> get_all_item_keys() const
  {
    std::vector<std::string> keys;
    keys.reserve( m_store.size() );
    for ( const auto &[k, _] : m_store )
    {
      if ( k == "ERROR_SPRITE" ) continue;
      keys.push_back( k );
    }
    return keys;
  }

  //! @brief Pick a random element from a caller-supplied list of keys, biased by player luck.
  //! @param player_luck_stat Player's luck stat, range [0, 100].
  //! @param list Keys ordered from worst to best outcome; higher luck shifts odds towards the back.
  //! @return The chosen key.
  //! @throws std::runtime_error if list is empty.
  [[nodiscard]] std::string get_random_item_from_list( int player_luck_stat, std::vector<std::string> list ) const
  {
    if ( list.empty() ) throw std::runtime_error( "provided list is empty" );

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
    return list.at( start + static_cast<std::size_t>( index_roll ) );
  }

protected:
  //! @brief Construct the store and register it as the singleton instance of Derived.
  //! @param json_file_path Path of the JSON file the derived store loads its data from.
  explicit StoreSingleton( std::filesystem::path json_file_path )
      : m_json_file_path( std::move( json_file_path ) )
  {
    s_instance = static_cast<Derived *>( this );
  }

  //! @brief Filesystem path of the JSON file this store loads its data from.
  std::filesystem::path m_json_file_path;

  //! @brief String-keyed map of all loaded StoreValue entries.
  store_map m_store;

  //! @brief Pointer to the single Derived instance, set by the StoreSingleton constructor.
  static Derived *s_instance;
};

template <typename Derived, typename StoreValue>
Derived *StoreSingleton<Derived, StoreValue>::s_instance = nullptr;

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_STORESINGLETON_HPP__
