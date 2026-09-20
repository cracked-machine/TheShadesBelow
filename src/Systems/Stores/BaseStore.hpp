#ifndef SRC_SYSTEMS_STORES_BASESTORE_HPP__
#define SRC_SYSTEMS_STORES_BASESTORE_HPP__

#include <Components/Inventory/WorldItem.hpp>
#include <Components/Random.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Sprites/SpriteMetaType.hpp>
#include <Systems/BaseSystem.hpp>

#include <Utils/Player.hpp>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <stdexcept>

namespace Game::Sys
{

//! @brief Base class for JSON-backed data stores. Loads JSON files from disk and provides shared helpers for
//! extracting Cmp::Stats::BaseAction fields (health, fear, despair, infamy, toxicity, tick, disease) from JSON entries.
class BaseStore : public BaseSystem
{
public:
  //! @brief Construct a new Base Store object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  BaseStore( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief Destroy the Base Store object
  ~BaseStore() {}

  //! @brief Load and parse a JSON file from disk.
  //! @param json_file
  //! @return The parsed JSON document.
  //! @throws std::runtime_error if the file does not exist or cannot be opened.
  nlohmann::json load_json_file( const std::filesystem::path &json_file );

  //! @brief Declared but not implemented; item store initialization is instead handled by ItemStore::init_store().
  void init_item_store();

  //! @brief Extract the "health" field from a JSON action entry.
  //! @param item
  //! @return The health value.
  int health( const nlohmann::json &item );

  //! @brief Extract the "fear" field from a JSON action entry.
  //! @param item
  //! @return The fear value.
  int fear( const nlohmann::json &item );

  //! @brief Extract the "despair" field from a JSON action entry.
  //! @param item
  //! @return The despair value.
  int despair( const nlohmann::json &item );

  //! @brief Extract the "infamy" field from a JSON action entry.
  //! @param item
  //! @return The infamy value.
  int infamy( const nlohmann::json &item );

  //! @brief Extract the "toxicity" field from a JSON action entry.
  //! @param item
  //! @return The toxicity value.
  int toxicity( const nlohmann::json &item );

  //! @brief Extract the "luck" field from a JSON action entry.
  //! @param item
  //! @return The toxicity value.
  int luck( const nlohmann::json &item );

  //! @brief Extract the "tick" field from a JSON action entry.
  //! @param item
  //! @return The tick value.
  float tick( const nlohmann::json &item );

  //! @brief Extract the "toxidrome" field from a JSON action entry.
  //! @param item
  //! @return The extracted Cmp::Stats::Disease value.
  Cmp::Toxicity::Toxidrome toxidrome( const nlohmann::json &item );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

protected:
  //! @brief Filesystem path of the JSON file this store loads its data from.
  std::filesystem::path m_json_file_path;
};

//////////////////////////////////////////////////////////////////////////
// CRTP intermediate — gives each derived class its own static instance //
//////////////////////////////////////////////////////////////////////////

//! @brief CRTP base for singleton, string-keyed data stores. Gives each Derived store class its own static
//! instance and a shared map of string id to StoreValue, plus lookup helpers.
//! @tparam Derived
//! @tparam StoreValue
template <typename Derived, typename StoreValue>
class StoreSingleton : public BaseStore
{
public:
  using BaseStore::BaseStore;

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
  //! @brief String-keyed map of all loaded StoreValue entries.
  store_map m_store;

  //! @brief Pointer to the single Derived instance, set by the derived class's constructor.
  static Derived *s_instance;
};

template <typename Derived, typename StoreValue>
Derived *StoreSingleton<Derived, StoreValue>::s_instance = nullptr;

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_BASESTORE_HPP__
