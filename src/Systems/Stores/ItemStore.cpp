#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ForageAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SacrificeAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Factory/ToxicityFactory.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Utils/JsonDeserializer.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace
{

//! @brief Convert the "wear.location" string of a JSON item entry.
Game::Cmp::WorldItem::Location make_wear_location( const std::string &wear_location )
{
  using Location = Game::Cmp::WorldItem::Location;
  if ( wear_location == "none" ) return Location::NONE;
  if ( wear_location == "world" ) return Location::WORLD;
  if ( wear_location == "inventory" ) return Location::INVENTORY;
  if ( wear_location == "both" ) return Location::BOTH;
  SPDLOG_WARN( "Unknown wear location: {}", wear_location );
  return Location::NONE;
}

Game::Cmp::WorldItem::Consumable make_consumable_property( const std::string &consumable )
{
  using Consumable = Game::Cmp::WorldItem::Consumable;
  if ( consumable == "none" ) return Consumable::NONE;
  if ( consumable == "drinkable" ) return Consumable::DRINKABLE;
  if ( consumable == "edible" ) return Consumable::EDIBLE;
  SPDLOG_WARN( "Unknown consumable type: {}", consumable );
  return Consumable::NONE;
}

} // namespace

namespace Game::Sys
{

ItemStore::ItemStore( std::filesystem::path json_file_path )
    : StoreSingleton<ItemStore, Cmp::WorldItem>( std::move( json_file_path ) )
{
  init_store();
  SPDLOG_DEBUG( "ItemStore initialized" );
}

void ItemStore::init_store()
{
  nlohmann::json json = Utils::JsonDeserializer::load_json_file( m_json_file_path );
  for ( const auto &[item_key, item_value] : json.items() )
  {
    Sys::SpriteKey sprite_mtype( item_value.at( "sprite" ).get<std::string>() );
    const auto &wear = item_value.at( "wear" );
    auto expiry = wear.at( "expiryseconds" ).get<float>();
    auto wear_location = make_wear_location( wear.at( "location" ).get<std::string>() );
    auto wear_active_only = wear.at( "active_only" ).get<bool>();
    auto damage_to_weapon = item_value.at( "damage_to_weapon" );
    auto pickaxe_damage = damage_to_weapon.at( "pickaxe" ).get<int>();
    auto axe_damage = damage_to_weapon.at( "axe" ).get<int>();
    auto shovel_damage = damage_to_weapon.at( "shovel" ).get<int>();
    auto bow_damage = damage_to_weapon.at( "bow" ).get<int>();
    auto consumable = make_consumable_property( item_value.at( "consumable" ).get<std::string>() );
    Cmp::WorldItem worlditem( key_type( item_key ), sprite_mtype, pickaxe_damage, axe_damage, shovel_damage, bow_damage, sf::seconds( expiry ),
                              wear_location, wear_active_only, consumable );
    for ( const auto &action_entry : item_value.at( "actions" ) )
    {
      for ( const auto &[action_key, action_value] : action_entry.items() )
      {
        try
        {
          if ( action_key == Cmp::BurnAction::kKey ) { worlditem.emplace( make_action<Cmp::BurnAction>( action_value ) ); }
          else if ( action_key == Cmp::CarryAction::kKey ) { worlditem.emplace( make_action<Cmp::CarryAction>( action_value ) ); }
          else if ( action_key == Cmp::ConsumeAction::kKey ) { worlditem.emplace( make_action<Cmp::ConsumeAction>( action_value ) ); }
          else if ( action_key == Cmp::DestroyAction::kKey ) { worlditem.emplace( make_action<Cmp::DestroyAction>( action_value ) ); }
          else if ( action_key == Cmp::ForageAction::kKey ) { worlditem.emplace( make_action<Cmp::ForageAction>( action_value ) ); }
          else if ( action_key == Cmp::SpawnAction::kKey ) { worlditem.emplace( make_action<Cmp::SpawnAction>( action_value ) ); }
          else if ( action_key == Cmp::ProximityAction::kKey ) { worlditem.emplace( make_action<Cmp::ProximityAction>( action_value ) ); }
          else if ( action_key == Cmp::SacrificeAction::kKey ) { worlditem.emplace( make_action<Cmp::SacrificeAction>( action_value ) ); }
          else { SPDLOG_WARN( "Unknown action key: {}", action_key ); }
        } catch ( const std::exception &e )
        {
          rethrow_with_context( item_key, action_key, action_value, e );
        }
      }
    }
    // a sacrifice reward is exactly one of item, wealth or special; forage/chop rewards are an item only
    if ( const auto reward = worlditem.get_sacrifice_reward(); reward and reward->count() != 1 )
    {
      throw std::runtime_error( "Item '" + item_key + "' sacrifice_action.reward must set exactly one of item, wealth or special in " +
                                m_json_file_path.string() );
    }
    for ( const auto &[property, reward] : { std::pair{ "forage_action.reward", worlditem.get_reward<Cmp::ForageAction>() },
                                             std::pair{ "destroy_action.reward", worlditem.get_reward<Cmp::DestroyAction>() } } )
    {
      if ( not reward or ( reward->wealth == 0 and reward->special == Cmp::ActionReward::Special::NONE ) ) continue;
      throw std::runtime_error( "Item '" + item_key + "' " + property + " must only set item in " + m_json_file_path.string() );
    }
    m_store.emplace( key_type( item_key ), std::move( worlditem ) );
    SPDLOG_DEBUG( "Loaded item: {} ({})", item_key, sprite_mtype );
  }

  // output properties must reference items that exist, otherwise the forage/chop/sacrifice silently spawns nothing
  for ( const auto &[item_key, item] : m_store )
  {
    for ( const auto &[property, output] :
          { std::pair{ "sacrifice_action.reward.item", item.get_sacrifice_reward().value_or( Cmp::ActionReward{} ).item },
            std::pair{ "forage_action.reward.item", item.get_forage_output() }, std::pair{ "destroy_action.reward.item", item.get_axe_output() } } )
    {
      if ( output.empty() or m_store.contains( output ) ) continue;
      throw std::runtime_error( "Item '" + item_key.str() + "' has unknown " + property + " '" + output.str() + "' in " + m_json_file_path.string() );
    }
  }
  SPDLOG_DEBUG( "Item store loaded with {} items", m_store.size() );
}

} // namespace Game::Sys