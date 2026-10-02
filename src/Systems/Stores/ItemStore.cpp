#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
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

//! @brief Build a stat-modifier action of the given kind from a JSON action entry.
template <typename Action>
Action make_action( const nlohmann::json &j )
{
  using Game::Utils::JsonDeserializer;
  // toxidrome fields may be nested under "toxidrome" or sit directly on the action entry
  const auto &tox = j.contains( "toxidrome" ) ? j.at( "toxidrome" ) : j;
  return Action(
      { JsonDeserializer::get_int( j, "health" ) }, { JsonDeserializer::get_int( j, "fear" ) }, { JsonDeserializer::get_int( j, "despair" ) },
      { JsonDeserializer::get_int( j, "infamy" ) }, { JsonDeserializer::get_int( j, "luck" ) }, { JsonDeserializer::get_float( j, "tick" ) },
      Game::Factory::Toxicity::create_toxidrome( JsonDeserializer::get_string( tox, "type" ), JsonDeserializer::get_int( tox, "toxicity" ) ) );
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
    auto expiry = item_value.at( "expiry" ).get<float>();
    Cmp::WorldItem worlditem( key_type( item_key ), sprite_mtype, sf::seconds( expiry ) );
    for ( const auto &action_entry : item_value.at( "actions" ) )
    {
      for ( const auto &[action_key, action_value] : action_entry.items() )
      {
        if ( action_key == "burn_action" ) { worlditem.emplace( make_action<Cmp::BurnAction>( action_value ) ); }
        else if ( action_key == "carry_action" ) { worlditem.emplace( make_action<Cmp::CarryAction>( action_value ) ); }
        else if ( action_key == "consume_action" ) { worlditem.emplace( make_action<Cmp::ConsumeAction>( action_value ) ); }
        else if ( action_key == "destroy_action" ) { worlditem.emplace( make_action<Cmp::DestroyAction>( action_value ) ); }
        else if ( action_key == "spawn_action" ) { worlditem.emplace( make_action<Cmp::SpawnAction>( action_value ) ); }
        else if ( action_key == "proximity_action" ) { worlditem.emplace( make_action<Cmp::ProximityAction>( action_value ) ); }
        else if ( action_key == "sacrifice_action" ) { worlditem.emplace( make_action<Cmp::SacrificeAction>( action_value ) ); }
        else { SPDLOG_WARN( "Unknown action key: {}", action_key ); }
      }
    }
    m_store.emplace( key_type( item_key ), std::move( worlditem ) );
    SPDLOG_DEBUG( "Loaded item: {} ({})", item_key, sprite_mtype );
  }
  SPDLOG_DEBUG( "Item store loaded with {} items", m_store.size() );
}

} // namespace Game::Sys