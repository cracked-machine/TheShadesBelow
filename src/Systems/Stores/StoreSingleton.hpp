#ifndef SRC_SYSTEMS_STORES_STORESINGLETON_HPP__
#define SRC_SYSTEMS_STORES_STORESINGLETON_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <filesystem>
#include <functional>
#include <nlohmann/json_fwd.hpp>
#include <source_location>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace Game::Sys
{

namespace Detail
{
//! @brief Build a predicate that tests a string against a pattern.
//! @param pattern Used as a regex if valid, otherwise as a plain substring.
//! @return The predicate.
std::function<bool( const std::string & )> make_pattern_matcher( const std::string &pattern );

//! @brief See StoreSingleton::rethrow_with_context. Defined in StoreSingleton.cpp so this header only needs
//! nlohmann/json_fwd.hpp (j.dump() needs the complete json type).
[[noreturn]] void rethrow_with_context( const std::filesystem::path &json_file_path, const std::string &owner_key, const std::string &action_key,
                                        const nlohmann::json &j, const std::exception &e );
} // namespace Detail

//! @brief CRTP base for singleton, string-keyed data stores. Gives each Derived store class its own static
//! instance and a map of StoreKey<Derived> to StoreValue, plus lookup helpers. Stores are plain data holders
//! (not systems): they are owned by Sys::Store, loaded once from a JSON file, and accessed via instance().
//! @details The member definitions live in StoreSingleton.cpp and are explicitly instantiated there, once per store,
//! so editing them rebuilds one file instead of every store user. Adding a new store therefore needs:
//!   - a `template class StoreSingleton<NewStore, Value>;` line at the end of StoreSingleton.cpp, and
//!   - an `extern template class StoreSingleton<NewStore, Value>;` line after the NewStore class in its header.
//! Forgetting the first fails to link (undefined StoreSingleton members) rather than misbehaving.
//! @tparam Derived The store class deriving from this one.
//! @tparam StoreValue The type of the stored entries.
template <typename Derived, typename StoreValue>
class StoreSingleton
{
public:
  //! @brief Clears the singleton instance pointer.
  ~StoreSingleton();

  //! @brief Stores are non-copyable and non-movable: s_instance points at the single live object.
  StoreSingleton( const StoreSingleton & ) = delete;
  StoreSingleton( StoreSingleton && ) = delete;
  StoreSingleton &operator=( const StoreSingleton & ) = delete;
  StoreSingleton &operator=( StoreSingleton && ) = delete;

  //! @brief Strongly-typed key used to look up entries in the Derived store.
  using key_type = StoreKey<Derived>;

  //! @brief Map type used to hold this store's loaded StoreValue entries.
  using store_map = std::unordered_map<key_type, StoreValue>;

  //! @brief Get the singleton instance of the Derived store. Kept inline: it is trivial and called constantly.
  //! @return Reference to the Derived store instance.
  //! @throws std::runtime_error if the store has not yet been initialized.
  static Derived &instance()
  {
    if ( not s_instance ) throw std::runtime_error( "Store not yet initialized" );
    return *s_instance;
  }

  //! @brief Look up a stored value by its type key.
  //! @param type The key to search for
  //! @param loc Call site captured for the error message if the type is not found
  //! @return Reference to the stored value.
  //! @throws std::runtime_error if the type is not present in the store.
  [[nodiscard]] const StoreValue &get( const key_type &type, std::source_location loc = std::source_location::current() ) const;

  //! @brief Get all type keys in the store, excluding the "ERROR_SPRITE" sentinel key.
  //! @return The list of keys.
  [[nodiscard]] std::vector<key_type> get_all() const;

  //! @brief Get all type keys in the store that match a pattern.
  //! Supports regex, falling back to plain substring matching if the pattern is not valid regex.
  //! @param pattern partial string pattern to match type keys
  //! @return The list of matching keys.
  [[nodiscard]] std::vector<key_type> get_pattern( const std::string &pattern ) const;

  //! @brief Look up a stored value by a type key picked, with equal probability, from a caller-supplied list.
  //! @param list The type keys to choose from; every candidate must exist in the store.
  //! @param loc Call site captured for the error message if the list is empty or the chosen type is not found
  //! @return Reference to the stored value of the chosen type.
  //! @throws std::runtime_error if list is empty, or if the chosen type is not present in the store.
  [[nodiscard]] const StoreValue &get_random( const std::vector<key_type> &list, std::source_location loc = std::source_location::current() ) const;

  //! @brief Look up a stored value by a type key picked from a caller-supplied list, biased by player luck.
  //! @param player_luck_stat Player's luck stat, range [0, 100].
  //! @param list Keys ordered from worst to best outcome; higher luck shifts odds towards the back.
  //! @param loc Call site captured for the error message if the list is empty or the chosen type is not found
  //! @return Reference to the stored value of the chosen type.
  //! @throws std::runtime_error if list is empty, or if the chosen type is not present in the store.
  [[nodiscard]] const StoreValue &get_random( int player_luck_stat, const std::vector<key_type> &list,
                                              std::source_location loc = std::source_location::current() ) const;

protected:
  //! @brief Construct the store and register it as the singleton instance of Derived.
  //! @param json_file_path Path of the JSON file the derived store loads its data from.
  //! @throws std::runtime_error if an instance of Derived already exists.
  explicit StoreSingleton( std::filesystem::path json_file_path );

  //! @brief Rethrow an error raised while loading an action, prefixed with the JSON file, owning entry, action key and
  //!        the offending action object, so the bad config can be located.
  //! @throws std::runtime_error always.
  [[noreturn]] void rethrow_with_context( const std::string &owner_key, const std::string &action_key, const nlohmann::json &j,
                                          const std::exception &e ) const;

  //! @brief Filesystem path of the JSON file this store loads its data from.
  std::filesystem::path m_json_file_path;

  //! @brief Map of all loaded StoreValue entries, keyed by type.
  store_map m_store;

  //! @brief Pointer to the single Derived instance, set by the StoreSingleton constructor.
  static Derived *s_instance;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_STORESINGLETON_HPP__
