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

  //! @brief Convert the "reward.altar" string of a JSON action entry.
  //! @throws std::runtime_error on an unknown value, so an invalid config can't silently disable the altar effect.
  static Cmp::ActionReward::AltarEffect make_reward_altar( const std::string &altar )
  {
    using AltarEffect = Cmp::ActionReward::AltarEffect;
    if ( altar == "None" ) return AltarEffect::NONE;
    if ( altar == "Candle" ) return AltarEffect::CANDLE;
    if ( altar == "Banish" ) return AltarEffect::BANISH;
    if ( altar == "ExtraLife" ) return AltarEffect::EXTRA_LIFE;
    if ( altar == "CryptKey" ) return AltarEffect::CRYPT_KEY;
    if ( altar == "ToxicImmunity" ) return AltarEffect::TOXIC_IMMUNITY;
    throw std::runtime_error( "Unknown reward altar: '" + altar + "'" );
  }

  //! @brief Convert the "reward.target" string of a JSON action entry.
  //! @throws std::runtime_error on an unknown value.
  static Cmp::ActionReward::Target make_reward_target( const std::string &target )
  {
    using Target = Cmp::ActionReward::Target;
    if ( target == "inventory" ) return Target::INVENTORY;
    if ( target == "nearby" ) return Target::NEARBY;
    throw std::runtime_error( "Unknown reward target: '" + target + "'" );
  }

  //! @brief Convert the "reward.location" string of a JSON action entry.
  //! @throws std::runtime_error on an unknown value.
  static Cmp::Altar::Kind make_reward_location( const std::string &location )
  {
    using Kind = Cmp::Altar::Kind;
    if ( location == "Graveyard" ) return Kind::GRAVEYARD;
    if ( location == "Crypt" ) return Kind::CRYPT;
    if ( location == "Shop" ) return Kind::SHOP;
    throw std::runtime_error( "Unknown reward location: '" + location + "'" );
  }

  //! @brief Convert the "side_effects.movement.player.type" string of a JSON action entry.
  //! @throws std::runtime_error on an unknown value.
  static Cmp::MovementEffect::Player::Type make_player_movement_type( const std::string &type )
  {
    using Type = Cmp::MovementEffect::Player::Type;
    if ( type == "slow" ) return Type::SLOW;
    if ( type == "drunk" ) return Type::DRUNK;
    throw std::runtime_error( "Unknown player movement type: '" + type + "'" );
  }

  //! @brief Convert the "side_effects.*.probability" string of a JSON action entry.
  //! @throws std::runtime_error on an unknown value.
  static Cmp::Probability make_probability( const std::string &probability )
  {
    if ( probability == "always" ) return Cmp::Probability::ALWAYS;
    if ( probability == "player_luck" ) return Cmp::Probability::PLAYER_LUCK;
    throw std::runtime_error( "Unknown side effect probability: '" + probability + "'" );
  }

  //! @brief Convert the "side_effects.spawn_npc.location" string of a JSON action entry.
  //! @throws std::runtime_error on an unknown value.
  static Cmp::SpawnNpcEffect::Location make_spawn_npc_location( const std::string &location )
  {
    using Location = Cmp::SpawnNpcEffect::Location;
    if ( location == "nearby" ) return Location::NEARBY;
    if ( location == "center" ) return Location::CENTER;
    throw std::runtime_error( "Unknown spawn_npc location: '" + location + "'" );
  }

  //! @brief Convert the "side_effects.movement.npc.type" string of a JSON action entry.
  //! @throws std::runtime_error on an unknown value.
  static Cmp::MovementEffect::Npc::Type make_npc_movement_type( const std::string &type )
  {
    using Type = Cmp::MovementEffect::Npc::Type;
    if ( type == "slow" ) return Type::SLOW;
    if ( type == "drunk" ) return Type::DRUNK;
    if ( type == "repel" ) return Type::REPEL;
    throw std::runtime_error( "Unknown npc movement type: '" + type + "'" );
  }

  //! @brief Build the rewards of a JSON action entry from its optional "reward" object, or array of objects.
  //! @throws std::runtime_error if a "reward" object sets none of item, wealth, altar or loot.
  static Cmp::ActionRewards make_reward( const nlohmann::json &j )
  {
    using Game::Utils::JsonDeserializer;
    if ( not j.contains( "reward" ) ) return {};

    const auto make_one = []( const nlohmann::json &rw )
    {
      Cmp::ActionReward reward;
      if ( rw.contains( "item" ) ) { reward.item = Sys::ItemKey( JsonDeserializer::get_string( rw, "item" ) ); }
      if ( rw.contains( "wealth" ) ) { reward.wealth = JsonDeserializer::get_int( rw, "wealth" ); }
      if ( rw.contains( "altar" ) ) { reward.altar = make_reward_altar( JsonDeserializer::get_string( rw, "altar" ) ); }
      if ( rw.contains( "location" ) ) { reward.location = make_reward_location( JsonDeserializer::get_string( rw, "location" ) ); }
      if ( rw.contains( "loot" ) ) { reward.loot = Sys::SpriteKey( JsonDeserializer::get_string( rw, "loot" ) ); }
      if ( rw.contains( "trigger_range" ) ) { reward.trigger_range = JsonDeserializer::get_int( rw, "trigger_range" ); }
      if ( rw.contains( "max_instances" ) ) { reward.max_instances = JsonDeserializer::get_int( rw, "max_instances" ); }
      if ( rw.contains( "tick" ) ) { reward.tick = JsonDeserializer::get_int( rw, "tick" ); }
      if ( rw.contains( "target" ) ) { reward.target = make_reward_target( JsonDeserializer::get_string( rw, "target" ) ); }
      if ( rw.contains( "count" ) ) { reward.quantity = JsonDeserializer::get_int( rw, "count" ); }
      if ( reward.count() == 0 ) throw std::runtime_error( "reward must set at least one of item, wealth, altar or loot" );
      if ( ( rw.contains( "target" ) or rw.contains( "count" ) ) and reward.item.empty() )
      {
        throw std::runtime_error( "reward target and count require an item" );
      }
      if ( rw.contains( "count" ) and ( reward.target != Cmp::ActionReward::Target::NEARBY or reward.quantity < 1 ) )
      {
        throw std::runtime_error( "reward count must be at least 1 and requires target 'nearby'" );
      }
      return reward;
    };

    const auto &rw = j.at( "reward" );
    if ( not rw.is_array() ) return { make_one( rw ) };
    Cmp::ActionRewards rewards;
    for ( const auto &entry : rw )
    {
      rewards.push_back( make_one( entry ) );
    }
    return rewards;
  }

  //! @brief Build the optional side effects of a JSON action entry from its optional "side_effects" object.
  //! @throws std::runtime_error if an effect's probability is not a known value.
  static Cmp::SideEffects make_side_effects( const nlohmann::json &j )
  {
    using Game::Utils::JsonDeserializer;
    Cmp::SideEffects effects;
    if ( not j.contains( "side_effects" ) ) return effects;
    const auto &fx = j.at( "side_effects" );

    const auto get_probability = []( const nlohmann::json &effect ) { return make_probability( JsonDeserializer::get_string( effect, "probability" ) ); };
    if ( fx.contains( "spawn_npc" ) )
    {
      const auto &spawn_npc = fx.at( "spawn_npc" );
      effects.spawn_npc = Cmp::SpawnNpcEffect{
          .npc_type = JsonDeserializer::get_string( spawn_npc, "type" ),
          .location = spawn_npc.contains( "location" ) ? make_spawn_npc_location( JsonDeserializer::get_string( spawn_npc, "location" ) )
                                                       : Cmp::SpawnNpcEffect::Location::NEARBY,
          .probability = get_probability( spawn_npc ),
          .max_instances = spawn_npc.contains( "max_instances" ) ? JsonDeserializer::get_int( spawn_npc, "max_instances" ) : 0,
          .trigger_range = spawn_npc.contains( "trigger_range" ) ? JsonDeserializer::get_int( spawn_npc, "trigger_range" ) : 1 };
    }
    if ( fx.contains( "lightning" ) ) { effects.lightning = Cmp::LightningEffect{ .probability = get_probability( fx.at( "lightning" ) ) }; }
    if ( fx.contains( "movement" ) )
    {
      const auto &movement = fx.at( "movement" );
      Cmp::MovementEffect movement_effect{ .probability = get_probability( movement ) };
      if ( movement.contains( "player" ) )
      {
        const auto &player = movement.at( "player" );
        movement_effect.player = Cmp::MovementEffect::Player{ .type = make_player_movement_type( JsonDeserializer::get_string( player, "type" ) ),
                                                              .tick = JsonDeserializer::get_int( player, "tick" ) };
      }
      if ( movement.contains( "npc" ) )
      {
        const auto &npc = movement.at( "npc" );
        movement_effect.npc = Cmp::MovementEffect::Npc{ .type = make_npc_movement_type( JsonDeserializer::get_string( npc, "type" ) ),
                                                        .tick = JsonDeserializer::get_int( npc, "tick" ) };
      }
      if ( not movement_effect.player and not movement_effect.npc ) throw std::runtime_error( "movement must set at least one of player or npc" );
      effects.movement = movement_effect;
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
