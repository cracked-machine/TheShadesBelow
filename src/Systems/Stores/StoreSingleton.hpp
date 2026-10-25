#ifndef SRC_SYSTEMS_STORES_STORESINGLETON_HPP__
#define SRC_SYSTEMS_STORES_STORESINGLETON_HPP__

#include <Components/Random.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Factory/ToxicityFactory.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <Utils/JsonDeserializer.hpp>
#include <filesystem>
#include <functional>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Game::Sys
{

namespace Detail
{
//! @brief Build a predicate that tests a string against a pattern.
//! @param pattern Used as a regex if valid, otherwise as a plain substring.
//! @return The predicate.
std::function<bool( const std::string & )> make_pattern_matcher( const std::string &pattern );
} // namespace Detail

//! @brief CRTP base for singleton, string-keyed data stores. Gives each Derived store class its own static
//! instance and a map of StoreKey<Derived> to StoreValue, plus lookup helpers. Stores are plain data holders
//! (not systems): they are owned by Sys::Store, loaded once from a JSON file, and accessed via instance().
//! @tparam Derived
//! @tparam StoreValue
template <typename Derived, typename StoreValue>
class StoreSingleton
{
public:
  //! @brief Clears the singleton instance pointer.
  ~StoreSingleton()
  {
    if ( s_instance == static_cast<Derived *>( this ) ) s_instance = nullptr;
  }

  //! @brief Stores are non-copyable and non-movable: s_instance points at the single live object.
  StoreSingleton( const StoreSingleton & ) = delete;
  StoreSingleton( StoreSingleton && ) = delete;
  StoreSingleton &operator=( const StoreSingleton & ) = delete;
  StoreSingleton &operator=( StoreSingleton && ) = delete;

  //! @brief Strongly-typed key used to look up entries in the Derived store.
  using key_type = StoreKey<Derived>;

  //! @brief Map type used to hold this store's loaded StoreValue entries.
  using store_map = std::unordered_map<key_type, StoreValue>;

  //! @brief Get the singleton instance of the Derived store.
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
  [[nodiscard]] const StoreValue &get( const key_type &type, std::source_location loc = std::source_location::current() ) const
  {
    auto it = m_store.find( type );
    if ( it != m_store.end() ) return it->second;

    throw std::runtime_error( "Failed to locate '" + type.str() + "' in store. Called from " + loc.file_name() + ":" + std::to_string( loc.line() ) +
                              " in '" + loc.function_name() + "'" );
  }

  //! @brief Get all type keys in the store, excluding the "ERROR_SPRITE" sentinel key.
  //! @return The list of keys.
  [[nodiscard]] std::vector<key_type> get_all() const
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

  //! @brief Get all type keys in the store that match a pattern.
  //! Supports regex, falling back to plain substring matching if the pattern is not valid regex.
  //! @param pattern partial string pattern to match type keys
  //! @return The list of matching keys.
  [[nodiscard]] std::vector<key_type> get_pattern( const std::string &pattern ) const
  {
    const auto matches = Detail::make_pattern_matcher( pattern );
    std::vector<key_type> types;
    for ( const auto &[type, _] : m_store )
    {
      if ( matches( type.str() ) ) types.push_back( type );
    }
    return types;
  }

  //! @brief Look up a stored value by a type key picked, with equal probability, from a caller-supplied list.
  //! @param list The type keys to choose from; every candidate must exist in the store.
  //! @param loc Call site captured for the error message if the list is empty or the chosen type is not found
  //! @return Reference to the stored value of the chosen type.
  //! @throws std::runtime_error if list is empty, or if the chosen type is not present in the store.
  [[nodiscard]] const StoreValue &get_random( const std::vector<key_type> &list, std::source_location loc = std::source_location::current() ) const
  {
    if ( list.empty() )
    {
      throw std::runtime_error( std::string( "Cannot pick a random type from an empty list. Called from " ) + loc.file_name() + ":" +
                                std::to_string( loc.line() ) + " in '" + loc.function_name() + "'" );
    }

    const int pick = Cmp::RandomInt( 0, static_cast<int>( list.size() ) - 1 ).gen();
    return get( list.at( static_cast<std::size_t>( pick ) ), loc );
  }

  //! @brief Look up a stored value by a type key picked from a caller-supplied list, biased by player luck.
  //! @param player_luck_stat Player's luck stat, range [0, 100].
  //! @param list Keys ordered from worst to best outcome; higher luck shifts odds towards the back.
  //! @param loc Call site captured for the error message if the list is empty or the chosen type is not found
  //! @return Reference to the stored value of the chosen type.
  //! @throws std::runtime_error if list is empty, or if the chosen type is not present in the store.
  [[nodiscard]] const StoreValue &get_random( int player_luck_stat, const std::vector<key_type> &list,
                                              std::source_location loc = std::source_location::current() ) const
  {
    if ( list.empty() )
    {
      throw std::runtime_error( std::string( "Cannot pick a random type from an empty list. Called from " ) + loc.file_name() + ":" +
                                std::to_string( loc.line() ) + " in '" + loc.function_name() + "'" );
    }

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

protected:
  //! @brief Construct the store and register it as the singleton instance of Derived.
  //! @param json_file_path Path of the JSON file the derived store loads its data from.
  //! @throws std::runtime_error if an instance of Derived already exists.
  explicit StoreSingleton( std::filesystem::path json_file_path )
      : m_json_file_path( std::move( json_file_path ) )
  {
    if ( s_instance ) throw std::runtime_error( "Store already initialized" );
    s_instance = static_cast<Derived *>( this );
  }

  //! @brief Build a stat-modifier action of the given kind from a JSON action entry.
  template <typename Action>
  Action make_action( const nlohmann::json &j )
  {
    using Game::Utils::JsonDeserializer;
    // toxidrome fields may be nested under "toxidrome" or sit directly on the action entry
    const auto &tox = j.contains( "toxidrome" ) ? j.at( "toxidrome" ) : j;
    return Action( { JsonDeserializer::get_int( j, "health" ) }, { JsonDeserializer::get_int( j, "fear" ) },
                   { JsonDeserializer::get_int( j, "despair" ) }, { JsonDeserializer::get_int( j, "infamy" ) },
                   { JsonDeserializer::get_int( j, "luck" ) }, { JsonDeserializer::get_float( j, "tick" ) },
                   Game::Factory::Toxicity::create_toxidrome( JsonDeserializer::get_string_or_list( tox, "type" ),
                                                              JsonDeserializer::get_int( tox, "toxicity" ) ),
                   make_side_effects( j ), make_reward( j ) );
  }

  //! @brief Convert the "reward.special" string of a JSON action entry.
  //! @throws std::runtime_error on an unknown value, so an invalid config can't silently disable the altar effect.
  static Cmp::ActionReward::Special make_reward_special( const std::string &special )
  {
    using Special = Cmp::ActionReward::Special;
    if ( special == "none" ) return Special::NONE;
    if ( special == "altar" ) return Special::ALTAR;
    if ( special == "banish" ) return Special::BANISH;
    if ( special == "crypt" ) return Special::CRYPT;
    throw std::runtime_error( "Unknown reward special: '" + special + "'" );
  }

  //! @brief Build the optional reward of a JSON action entry from its optional "reward" object.
  //! @throws std::runtime_error if the "reward" object sets none of item, wealth or special.
  static std::optional<Cmp::ActionReward> make_reward( const nlohmann::json &j )
  {
    using Game::Utils::JsonDeserializer;
    if ( not j.contains( "reward" ) ) return std::nullopt;
    const auto &rw = j.at( "reward" );

    Cmp::ActionReward reward;
    if ( rw.contains( "item" ) ) { reward.item = Sys::ItemKey( JsonDeserializer::get_string( rw, "item" ) ); }
    if ( rw.contains( "wealth" ) ) { reward.wealth = JsonDeserializer::get_int( rw, "wealth" ); }
    if ( rw.contains( "special" ) ) { reward.special = make_reward_special( JsonDeserializer::get_string( rw, "special" ) ); }
    if ( reward.count() == 0 ) throw std::runtime_error( "reward must set at least one of item, wealth or special" );
    return reward;
  }

  //! @brief Build the optional side effects of a JSON action entry from its optional "side_effects" object.
  //! @throws std::runtime_error if an effect's probability is outside [0, 1].
  static Cmp::SideEffects make_side_effects( const nlohmann::json &j )
  {
    using Game::Utils::JsonDeserializer;
    Cmp::SideEffects effects;
    if ( not j.contains( "side_effects" ) ) return effects;
    const auto &fx = j.at( "side_effects" );

    const auto get_probability = []( const nlohmann::json &effect )
    {
      const float probability = JsonDeserializer::get_float( effect, "probability" );
      if ( probability < 0.f or probability > 1.f ) throw std::runtime_error( "effect probability must be in range [0, 1]" );
      return probability;
    };
    if ( fx.contains( "spawn_npc" ) )
    {
      const auto &spawn_npc = fx.at( "spawn_npc" );
      effects.spawn_npc = Cmp::SpawnNpcEffect{ .npc_type = JsonDeserializer::get_string( spawn_npc, "type" ),
                                               .probability = get_probability( spawn_npc ),
                                               .unique = spawn_npc.contains( "unique" ) and JsonDeserializer::get_bool( spawn_npc, "unique" ) };
    }
    if ( fx.contains( "lightning" ) ) { effects.lightning = Cmp::LightningEffect{ .probability = get_probability( fx.at( "lightning" ) ) }; }
    if ( fx.contains( "movement" ) )
    {
      const auto &movement = fx.at( "movement" );
      effects.movement = Cmp::MovementEffect{ .type = JsonDeserializer::get_string( movement, "type" ), .probability = get_probability( movement ) };
    }
    return effects;
  }

  //! @brief Rethrow an error raised while loading an action, prefixed with the JSON file, owning entry, action key and
  //!        the offending action object, so the bad config can be located.
  //! @throws std::runtime_error always.
  [[noreturn]] void rethrow_with_context( const std::string &owner_key, const std::string &action_key, const nlohmann::json &j,
                                          const std::exception &e ) const
  {
    throw std::runtime_error( m_json_file_path.string() + ": '" + owner_key + "' " + action_key + " " + j.dump() + ": " + e.what() );
  }

  //! @brief Filesystem path of the JSON file this store loads its data from.
  std::filesystem::path m_json_file_path;

  //! @brief Map of all loaded StoreValue entries, keyed by type.
  store_map m_store;

  //! @brief Pointer to the single Derived instance, set by the StoreSingleton constructor.
  static Derived *s_instance;
};

template <typename Derived, typename StoreValue>
Derived *StoreSingleton<Derived, StoreValue>::s_instance = nullptr;

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_STORESINGLETON_HPP__
