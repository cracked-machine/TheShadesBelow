#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ForageAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <SFML/System/Time.hpp>
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
    auto damage_to_weapon = item_value.at( "damage_to_weapon" );
    auto pickaxe_damage = damage_to_weapon.at( "pickaxe" ).get<int>();
    auto axe_damage = damage_to_weapon.at( "axe" ).get<int>();
    auto shovel_damage = damage_to_weapon.at( "shovel" ).get<int>();
    auto bow_damage = damage_to_weapon.at( "bow" ).get<int>();
    auto consumable = make_consumable_property( item_value.at( "consumable" ).get<std::string>() );
    Cmp::WorldItem worlditem( key_type( item_key ), sprite_mtype, pickaxe_damage, axe_damage, shovel_damage, bow_damage, consumable );
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
          else if ( action_key == Cmp::OfferingAction::kKey ) { worlditem.emplace( make_action<Cmp::OfferingAction>( action_value ) ); }
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
    if ( item_value.contains( "axe_hits" ) )
    {
      const auto axe_hits = Utils::JsonDeserializer::get_int( item_value, "axe_hits" );
      if ( not item_key.starts_with( "item.plant." ) )
      {
        throw std::runtime_error( "Item '" + item_key + "' axe_hits is only allowed on plant items in " + m_json_file_path.string() );
      }
      // each hit wears the plant down, so it must have a world wear level to be chopped down over several hits
      if ( axe_hits < 1 or ( axe_hits > 1 and not worlditem.wears_in_world() ) )
      {
        throw std::runtime_error( "Item '" + item_key + "' axe_hits must be 1, or above 1 with a spawn_action wear, in " +
                                  m_json_file_path.string() );
      }
      worlditem.set_axe_hits( axe_hits );
    }
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
    for ( const auto &[action_type, action_pair] : worlditem )
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
    // a proximity reward is dropped beside the item: exactly one of item or loot
    if ( const auto reward = worlditem.get_reward<Cmp::ProximityAction>();
         reward and ( reward->count() != 1 or ( reward->item.empty() and reward->loot.empty() ) ) )
    {
      throw std::runtime_error( "Item '" + item_key + "' proximity_action.reward must set exactly one of item or loot in " + m_json_file_path.string() );
    }
    m_store.emplace( key_type( item_key ), std::move( worlditem ) );
    SPDLOG_DEBUG( "Loaded item: {} ({})", item_key, sprite_mtype );
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