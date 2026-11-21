#ifndef SRC_SYSTEMS_STORES_ACTIONJSON_HPP__
#define SRC_SYSTEMS_STORES_ACTIONJSON_HPP__

//! @file
//! @brief Parsing of the "*_action" objects in res/json/items.json and res/json/npc.json into Cmp::BaseAction
//! subclasses. Only included by the stores that load actions (ItemStore.cpp, NpcStore.cpp), so StoreSingleton.hpp
//! and everything that includes a store doesn't depend on BaseAction.hpp.

#include <Components/Altar/Kind.hpp>
#include <Components/Stats/ActionReward.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/SideEffects.hpp>
#include <Components/WeaponDamage.hpp>
#include <Factory/ToxicityFactory.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/JsonDeserializer.hpp>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace Game::Sys::Detail
{

//! @brief Convert the "reward.altar" string of a JSON action entry.
//! @throws std::runtime_error on an unknown value, so an invalid config can't silently disable the altar effect.
inline Cmp::ActionReward::AltarEffect make_reward_altar( const std::string &altar )
{
  using AltarEffect = Cmp::ActionReward::AltarEffect;
  if ( altar == "None" ) return AltarEffect::NONE;
  if ( altar == "Banish" ) return AltarEffect::BANISH;
  if ( altar == "ExtraLife" ) return AltarEffect::EXTRA_LIFE;
  if ( altar == "CryptKey" ) return AltarEffect::CRYPT_KEY;
  if ( altar == "ToxicImmunity" ) return AltarEffect::TOXIC_IMMUNITY;
  throw std::runtime_error( "Unknown reward altar: '" + altar + "'" );
}

//! @brief Convert the "reward.target" string of a JSON action entry.
//! @throws std::runtime_error on an unknown value.
inline Cmp::ActionReward::Target make_reward_target( const std::string &target )
{
  using Target = Cmp::ActionReward::Target;
  if ( target == "inventory" ) return Target::INVENTORY;
  if ( target == "nearby" ) return Target::NEARBY;
  throw std::runtime_error( "Unknown reward target: '" + target + "'" );
}

//! @brief Convert the "reward.location" string of a JSON action entry.
//! @throws std::runtime_error on an unknown value.
inline Cmp::Altar::Kind make_reward_location( const std::string &location )
{
  using Kind = Cmp::Altar::Kind;
  if ( location == "Graveyard" ) return Kind::GRAVEYARD;
  if ( location == "Crypt" ) return Kind::CRYPT;
  if ( location == "Shop" ) return Kind::SHOP;
  throw std::runtime_error( "Unknown reward location: '" + location + "'" );
}

//! @brief Convert the "side_effects.movement.player.type" string of a JSON action entry.
//! @throws std::runtime_error on an unknown value.
inline Cmp::MovementEffect::Player::Type make_player_movement_type( const std::string &type )
{
  using Type = Cmp::MovementEffect::Player::Type;
  if ( type == "slow" ) return Type::SLOW;
  if ( type == "drunk" ) return Type::DRUNK;
  throw std::runtime_error( "Unknown player movement type: '" + type + "'" );
}

//! @brief Convert the "side_effects.*.probability" string of a JSON action entry.
//! @throws std::runtime_error on an unknown value.
inline Cmp::Probability make_probability( const std::string &probability )
{
  if ( probability == "always" ) return Cmp::Probability::ALWAYS;
  if ( probability == "player_luck" ) return Cmp::Probability::PLAYER_LUCK;
  throw std::runtime_error( "Unknown side effect probability: '" + probability + "'" );
}

//! @brief Convert the "side_effects.spawn_npc.location" string of a JSON action entry.
//! @throws std::runtime_error on an unknown value.
inline Cmp::SpawnNpcEffect::Location make_spawn_npc_location( const std::string &location )
{
  using Location = Cmp::SpawnNpcEffect::Location;
  if ( location == "nearby" ) return Location::NEARBY;
  if ( location == "center" ) return Location::CENTER;
  throw std::runtime_error( "Unknown spawn_npc location: '" + location + "'" );
}

//! @brief Build the "side_effects.spawn_npc.if" condition of a JSON action entry.
//! @throws std::runtime_error on an unknown stat.
inline Cmp::SpawnNpcEffect::Condition make_spawn_npc_condition( const nlohmann::json &cond )
{
  using Game::Utils::JsonDeserializer;
  using Stat = Cmp::SpawnNpcEffect::Condition::Stat;
  Cmp::SpawnNpcEffect::Condition condition{ .stat_name = JsonDeserializer::get_string( cond, "stat" ),
                                            .value = JsonDeserializer::get_int( cond, "value" ) };
  const auto &stat = condition.stat_name;
  if ( stat == "health" )
    condition.stat = Stat::HEALTH;
  else if ( stat == "fear" )
    condition.stat = Stat::FEAR;
  else if ( stat == "despair" )
    condition.stat = Stat::DESPAIR;
  else if ( stat == "infamy" )
    condition.stat = Stat::INFAMY;
  else if ( stat == "luck" )
    condition.stat = Stat::LUCK;
  else if ( stat == "all" )
    condition.stat = Stat::ALL_TOXIDROMES;
  else if ( const auto id = Game::Factory::Toxicity::toxidrome_id( stat ) )
  {
    condition.stat = Stat::TOXIDROME;
    condition.toxidrome_id = *id;
  }
  else { throw std::runtime_error( "Unknown spawn_npc if.stat: '" + stat + "'" ); }
  return condition;
}

//! @brief Convert the "side_effects.movement.npc.type" string of a JSON action entry.
//! @throws std::runtime_error on an unknown value.
inline Cmp::MovementEffect::Npc::Type make_npc_movement_type( const std::string &type )
{
  using Type = Cmp::MovementEffect::Npc::Type;
  if ( type == "slow" ) return Type::SLOW;
  if ( type == "drunk" ) return Type::DRUNK;
  if ( type == "repel" ) return Type::REPEL;
  throw std::runtime_error( "Unknown npc movement type: '" + type + "'" );
}

//! @brief Build the rewards of a JSON action entry from its optional "reward" object, or array of objects.
//! @throws std::runtime_error if a "reward" object sets none of item, wealth, altar or loot.
inline Cmp::ActionRewards make_reward( const nlohmann::json &j )
{
  using Game::Utils::JsonDeserializer;
  if ( not j.contains( "reward" ) ) return {};

  const auto make_one = []( const nlohmann::json &rw )
  {
    Cmp::ActionReward reward;
    if ( rw.contains( "item" ) and rw.at( "item" ).is_array() )
    {
      for ( const auto &pool_item : rw.at( "item" ) )
      {
        if ( not pool_item.is_string() ) throw std::runtime_error( "reward item array must only contain item key strings" );
        reward.item_pool.emplace_back( pool_item.get<std::string>() );
      }
      if ( reward.item_pool.empty() ) throw std::runtime_error( "reward item array must not be empty" );
    }
    else if ( rw.contains( "item" ) ) { reward.item = Sys::ItemKey( JsonDeserializer::get_string( rw, "item" ) ); }
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
    if ( ( rw.contains( "target" ) or rw.contains( "count" ) ) and not reward.has_item() )
    {
      throw std::runtime_error( "reward target and count require an item" );
    }
    if ( not reward.item_pool.empty() and ( rw.contains( "target" ) or rw.contains( "count" ) ) )
    {
      throw std::runtime_error( "reward target and count are not allowed with an item array" );
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
inline Cmp::SideEffects make_side_effects( const nlohmann::json &j )
{
  using Game::Utils::JsonDeserializer;
  Cmp::SideEffects effects;
  if ( not j.contains( "side_effects" ) ) return effects;
  const auto &fx = j.at( "side_effects" );

  const auto get_probability = []( const nlohmann::json &effect )
  { return make_probability( JsonDeserializer::get_string( effect, "probability" ) ); };
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
    if ( spawn_npc.contains( "if" ) ) { effects.spawn_npc->condition = make_spawn_npc_condition( spawn_npc.at( "if" ) ); }
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
  if ( fx.contains( "toxic_immunity" ) )
  {
    const auto &toxic_immunity = fx.at( "toxic_immunity" );
    effects.toxic_immunity = Cmp::ToxicImmunityEffect{ .tick = JsonDeserializer::get_int( toxic_immunity, "tick" ),
                                                       .probability = get_probability( toxic_immunity ) };
  }
  if ( fx.contains( "kill_npc" ) )
  {
    const auto &kill_npc = fx.at( "kill_npc" );
    Cmp::KillNpcEffect kill_npc_effect{ .trigger_range = kill_npc.contains( "trigger_range" ) ? JsonDeserializer::get_int( kill_npc, "trigger_range" )
                                                                                              : 1,
                                        .probability = get_probability( kill_npc ) };
    if ( kill_npc.contains( "npc_types" ) )
    {
      for ( const auto &npc_type : JsonDeserializer::get_string_or_list( kill_npc, "npc_types" ) )
      {
        kill_npc_effect.npc_types.emplace_back( npc_type );
      }
    }
    if ( kill_npc.contains( "wear_per_kill" ) ) { kill_npc_effect.wear_per_kill = JsonDeserializer::get_float( kill_npc, "wear_per_kill" ); }
    if ( kill_npc.contains( "death_anim" ) )
    {
      kill_npc_effect.death_anim = Sys::SpriteKey( JsonDeserializer::get_string( kill_npc, "death_anim" ) );
    }
    effects.kill_npc = kill_npc_effect;
  }
  return effects;
}

//! @brief Build a stat-modifier action of the given kind from a JSON action entry.
template <typename Action>
[[nodiscard]] Action make_action( const nlohmann::json &j )
{
  using Game::Utils::JsonDeserializer;
  // toxidrome fields may be nested under "toxidrome" or sit directly on the action entry
  const auto &tox = j.contains( "toxidrome" ) ? j.at( "toxidrome" ) : j;
  Action action(
      { JsonDeserializer::get_int( j, "health" ) }, { JsonDeserializer::get_int( j, "fear" ) }, { JsonDeserializer::get_int( j, "despair" ) },
      { JsonDeserializer::get_int( j, "infamy" ) }, { JsonDeserializer::get_int( j, "luck" ) },
      { JsonDeserializer::get_float( j, "tick" ) },
      Game::Factory::Toxicity::create_toxidrome( JsonDeserializer::get_string_or_list( tox, "type" ), JsonDeserializer::get_int( tox, "toxicity" ) ),
      make_side_effects( j ), make_reward( j ) );
  if ( j.contains( "wear" ) )
  {
    const auto &wear = j.at( "wear" );
    action.set_wear( Cmp::ActionWear{ .expiry_seconds = JsonDeserializer::get_float( wear, "expiryseconds" ),
                                      .active_only = wear.at( "active_only" ).get<bool>() } );
  }
  // sfx is optional, but when present "final" is required ("loop" is optional)
  if ( j.contains( "sfx" ) )
  {
    const auto &sfx = j.at( "sfx" );
    action.set_sfx( Cmp::ActionSfx{ sfx.value( "loop", std::string{} ), sfx.at( "final" ).get<std::string>() } );
  }
  return action;
}

//! @brief Convert a "damage_to_weapon" or "damage_from_weapon" object: { "pickaxe", "axe", "shovel", "bow" }, all required.
inline Cmp::WeaponDamage make_weapon_damage( const nlohmann::json &j )
{
  return Cmp::WeaponDamage{ .pickaxe = j.at( "pickaxe" ).get<int>(),
                            .axe = j.at( "axe" ).get<int>(),
                            .shovel = j.at( "shovel" ).get<int>(),
                            .bow = j.at( "bow" ).get<int>() };
}

} // namespace Game::Sys::Detail

#endif // SRC_SYSTEMS_STORES_ACTIONJSON_HPP__
