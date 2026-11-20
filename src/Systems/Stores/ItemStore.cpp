#include <Components/Stats/ActionReward.hpp>
#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ForageAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Stats/SideEffects.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/Stores/ActionJson.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Utils/JsonDeserializer.hpp>

#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <typeindex>

namespace
{

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

std::vector<ItemStore::key_type> ItemStore::get_all( ItemFilter filter ) const
{
  auto types = StoreSingleton::get_all();
  if ( filter == ItemFilter::SHOP_ONLY ) { std::erase_if( types, [this]( const key_type &type ) { return not get( type ).is_shop_item(); } ); }
  return types;
}

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
    const auto &sprite_json = item_value.at( "sprite" );
    Sys::SpriteKey sprite_mtype( Utils::JsonDeserializer::get_string( sprite_json, "name" ) );
    const int sprite_index = sprite_json.value( "index", 0 );
    const bool sprite_random = sprite_json.value( "random", false );
    if ( sprite_json.contains( "index" ) and sprite_json.contains( "random" ) )
    {
      throw std::runtime_error( "Item '" + item_key + "' sprite.index and sprite.random are mutually exclusive in " + m_json_file_path.string() );
    }
    // index 0 is always valid, so only look the sheet up for a non-zero index (the ERROR_SPRITE item has no sheet)
    if ( sprite_index < 0 or
         ( sprite_index > 0 and static_cast<std::size_t>( sprite_index ) >= Sys::SpriteStore::instance().get( sprite_mtype ).sheet_size() ) )
    {
      throw std::runtime_error( "Item '" + item_key + "' sprite.index " + std::to_string( sprite_index ) + " is out of range for '" +
                                sprite_mtype.str() + "' in " + m_json_file_path.string() );
    }
    const auto damage_to_weapon = Detail::make_weapon_damage( item_value.at( "damage_to_weapon" ) );
    auto consumable = make_consumable_property( item_value.at( "consumable" ).get<std::string>() );
    Cmp::WorldItem worlditem( key_type( item_key ), sprite_mtype, static_cast<std::size_t>( sprite_index ),
                              sprite_random ? Sys::SpriteStore::instance().get( sprite_mtype ).sheet_size() : 0, damage_to_weapon.pickaxe,
                              damage_to_weapon.axe, damage_to_weapon.shovel, damage_to_weapon.bow, consumable );
    if ( item_value.contains( "shop" ) )
    {
      const auto &shop_json = item_value.at( "shop" );
      worlditem.set_shop_item( shop_json.value( "enabled", false ) );
      worlditem.set_shop_base_price( shop_json.value( "base_price", 0 ) );
    }
    for ( const auto &action_entry : item_value.at( "actions" ) )
    {
      for ( const auto &[action_key, action_value] : action_entry.items() )
      {
        try
        {
          if ( action_key == Cmp::BurnAction::kKey ) { worlditem.emplace( Detail::make_action<Cmp::BurnAction>( action_value ) ); }
          else if ( action_key == Cmp::CarryAction::kKey ) { worlditem.emplace( Detail::make_action<Cmp::CarryAction>( action_value ) ); }
          else if ( action_key == Cmp::ConsumeAction::kKey ) { worlditem.emplace( Detail::make_action<Cmp::ConsumeAction>( action_value ) ); }
          else if ( action_key == Cmp::DestroyAction::kKey ) { worlditem.emplace( Detail::make_action<Cmp::DestroyAction>( action_value ) ); }
          else if ( action_key == Cmp::ForageAction::kKey ) { worlditem.emplace( Detail::make_action<Cmp::ForageAction>( action_value ) ); }
          else if ( action_key == Cmp::SpawnAction::kKey ) { worlditem.emplace( Detail::make_action<Cmp::SpawnAction>( action_value ) ); }
          else if ( action_key == Cmp::ProximityAction::kKey ) { worlditem.emplace( Detail::make_action<Cmp::ProximityAction>( action_value ) ); }
          else if ( action_key == Cmp::OfferingAction::kKey ) { worlditem.emplace( Detail::make_action<Cmp::OfferingAction>( action_value ) ); }
          else { SPDLOG_WARN( "Unknown action key: {}", action_key ); }
        } catch ( const std::exception &e )
        {
          rethrow_with_context( item_key, action_key, action_value, e );
        }
        // wear is only read from spawn_action (world), carry_action (inventory) and proximity_action
        if ( action_value.contains( "wear" ) and action_key != Cmp::SpawnAction::kKey and action_key != Cmp::CarryAction::kKey and
             action_key != Cmp::ProximityAction::kKey )
        {
          throw std::runtime_error( "Item '" + item_key + "' " + std::string( action_key ) +
                                    ".wear is only allowed on spawn_action, carry_action or proximity_action in " + m_json_file_path.string() );
        }
      }
    }
    // plants have health, so every plant says how much each weapon hurts it; nothing else may
    const bool is_plant = item_key.starts_with( "item.plant." );
    if ( is_plant != item_value.contains( "damage_from_weapon" ) )
    {
      throw std::runtime_error( "Item '" + item_key + "' damage_from_weapon is required on plant items and only allowed on them in " +
                                m_json_file_path.string() );
    }
    if ( is_plant ) { worlditem.set_damage_from_weapon( Detail::make_weapon_damage( item_value.at( "damage_from_weapon" ) ) ); }
    // an offering reward is exactly one of item, wealth or altar, and each altar location has at most one reward
    const auto offering_rewards = worlditem.get_rewards<Cmp::OfferingAction>();
    for ( auto it = offering_rewards.begin(); it != offering_rewards.end(); ++it )
    {
      if ( it->count() != 1 )
      {
        throw std::runtime_error( "Item '" + item_key + "' offering_action.reward must set exactly one of item, wealth or altar in " +
                                  m_json_file_path.string() );
      }
      for ( auto other = std::next( it ); other != offering_rewards.end(); ++other )
      {
        if ( it->location and other->location and it->location != other->location ) continue;
        throw std::runtime_error( "Item '" + item_key + "' offering_action.reward has more than one reward for the same location in " +
                                  m_json_file_path.string() );
      }
    }
    // only an offering reward can be altar specific
    for ( const auto &[action_type, action_pair] : worlditem.actions() )
    {
      if ( action_type == std::type_index( typeid( Cmp::OfferingAction ) ) ) continue;
      const auto &rewards = action_pair.action.rewards();
      if ( rewards.size() <= 1 and std::ranges::none_of( rewards, []( const Cmp::ActionReward &r ) { return r.location.has_value(); } ) ) continue;
      throw std::runtime_error( "Item '" + item_key + "' only offering_action.reward may be an array or set a location in " +
                                m_json_file_path.string() );
    }
    for ( const auto &[property, reward] : { std::pair{ "forage_action.reward", worlditem.get_reward<Cmp::ForageAction>() },
                                             std::pair{ "destroy_action.reward", worlditem.get_reward<Cmp::DestroyAction>() } } )
    {
      if ( not reward or ( reward->wealth == 0 and reward->altar == Cmp::ActionReward::AltarEffect::NONE and reward->loot.empty() ) ) continue;
      throw std::runtime_error( "Item '" + item_key + "' " + property + " must only set item in " + m_json_file_path.string() );
    }
    // an item array (random pick) is only allowed on a destroy_action reward
    for ( const auto &[action_type, action_pair] : worlditem.actions() )
    {
      if ( action_type == std::type_index( typeid( Cmp::DestroyAction ) ) ) continue;
      if ( std::ranges::none_of( action_pair.action.rewards(), []( const Cmp::ActionReward &r ) { return not r.item_pool.empty(); } ) ) continue;
      throw std::runtime_error( "Item '" + item_key + "' only destroy_action.reward.item may be an array in " + m_json_file_path.string() );
    }
    // a proximity reward is dropped beside the item: exactly one of item or loot
    if ( const auto reward = worlditem.get_reward<Cmp::ProximityAction>();
         reward and ( reward->count() != 1 or ( reward->item.empty() and reward->loot.empty() ) ) )
    {
      throw std::runtime_error( "Item '" + item_key + "' proximity_action.reward must set exactly one of item or loot in " + m_json_file_path.string() );
    }
    m_store.emplace( key_type( item_key ), std::move( worlditem ) );
    SPDLOG_DEBUG( "Loaded item: {} ({}[{}])", item_key, sprite_mtype, sprite_random ? "random" : std::to_string( sprite_index ) );
  }

  // output properties must reference items that exist, otherwise the forage/chop/offering silently spawns nothing
  for ( const auto &[item_key, item] : m_store )
  {
    for ( const auto &reward : item.get_rewards<Cmp::OfferingAction>() )
    {
      if ( reward.item.empty() or not get_item_matches( reward.item ).empty() ) continue;
      throw std::runtime_error( "Item '" + item_key.str() + "' has unknown offering_action.reward.item '" + reward.item.str() + "' in " +
                                m_json_file_path.string() );
    }
    for ( const auto &[property, output] :
          { std::pair{ "forage_action.reward.item", item.get_reward<Cmp::ForageAction>().value_or( Cmp::ActionReward{} ).item },
            std::pair{ "destroy_action.reward.item", item.get_reward<Cmp::DestroyAction>().value_or( Cmp::ActionReward{} ).item },
            std::pair{ "proximity_action.reward.item", item.get_reward<Cmp::ProximityAction>().value_or( Cmp::ActionReward{} ).item } } )
    {
      if ( output.empty() or not get_item_matches( output ).empty() ) continue;
      throw std::runtime_error( "Item '" + item_key.str() + "' has unknown " + property + " '" + output.str() + "' in " + m_json_file_path.string() );
    }
    for ( const auto &pool_item : item.get_reward<Cmp::DestroyAction>().value_or( Cmp::ActionReward{} ).item_pool )
    {
      if ( not get_item_matches( pool_item ).empty() ) continue;
      throw std::runtime_error( "Item '" + item_key.str() + "' has unknown destroy_action.reward.item '" + pool_item.str() + "' in " +
                                m_json_file_path.string() );
    }
    const auto proximity_loot = item.get_reward<Cmp::ProximityAction>().value_or( Cmp::ActionReward{} ).loot;
    if ( proximity_loot.empty() ) continue;
    if ( const auto sprite_types = Sys::SpriteStore::instance().get_all(); std::ranges::find( sprite_types, proximity_loot ) == sprite_types.end() )
    {
      throw std::runtime_error( "Item '" + item_key.str() + "' has unknown proximity_action.reward.loot '" + proximity_loot.str() + "' in " +
                                m_json_file_path.string() );
    }
  }
  SPDLOG_DEBUG( "Item store loaded with {} items", m_store.size() );
}

std::vector<ItemStore::key_type> ItemStore::get_item_matches( const key_type &key ) const
{
  if ( not key.str().ends_with( '*' ) ) return m_store.contains( key ) ? std::vector<key_type>{ key } : std::vector<key_type>{};
  const auto prefix = key.str().substr( 0, key.str().size() - 1 );
  std::vector<key_type> matches;
  for ( const auto &[type, _] : m_store )
  {
    if ( type.str().starts_with( prefix ) ) matches.push_back( type );
  }
  return matches;
}

ItemStore::key_type ItemStore::pick_item( const key_type &key ) const
{
  if ( not key.str().ends_with( '*' ) ) return key;
  return get_random( get_item_matches( key ) ).item_type;
}

ItemStore::key_type ItemStore::pick_reward_item( const Cmp::ActionReward &reward ) const
{
  if ( reward.item_pool.empty() ) return pick_item( reward.item );
  const auto pick = static_cast<std::size_t>( Cmp::RandomInt( 0, static_cast<int>( reward.item_pool.size() ) - 1 ).gen() );
  return pick_item( reward.item_pool[pick] );
}

std::vector<ItemStore::key_type> ItemStore::pick_distinct_items( const key_type &key, std::size_t n ) const
{
  auto matches = get_item_matches( key );
  if ( matches.size() < n )
  {
    throw std::runtime_error( "Cannot pick " + std::to_string( n ) + " distinct items for '" + key.str() + "': only " +
                              std::to_string( matches.size() ) + " match in " + m_json_file_path.string() );
  }
  // partial Fisher-Yates: the first n entries end up as a uniform random selection
  for ( std::size_t i = 0; i < n; ++i )
  {
    const auto pick = static_cast<std::size_t>( Cmp::RandomInt( static_cast<int>( i ), static_cast<int>( matches.size() ) - 1 ).gen() );
    std::swap( matches[i], matches[pick] );
  }
  matches.resize( n );
  return matches;
}

} // namespace Game::Sys