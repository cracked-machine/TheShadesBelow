#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SacrificeAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Systems/Stores/BaseStore.hpp>
#include <Systems/Stores/ItemStore.hpp>

#include <nlohmann/json.hpp>

namespace Game::Sys
{

ItemStore::ItemStore( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : StoreSingleton<ItemStore, Cmp::WorldItem>( reg, window, sprite_factory, sound_bank )
{
  s_instance = this;
  m_json_file_path = "res/json/items.json";
  init_store();
  SPDLOG_DEBUG( "ItemStore initialized" );
}

void ItemStore::init_store()
{
  nlohmann::json json = load_json_file( m_json_file_path );
  for ( const auto &[item_key, item_value] : json.items() )
  {
    Sprites::SpriteMetaType sprite_mtype = item_value.at( "sprite" ).get<std::string>();
    Cmp::WorldItem worlditem( item_key, sprite_mtype );
    for ( const auto &action_entry : item_value.at( "actions" ) )
    {
      for ( const auto &[action_key, action_value] : action_entry.items() )
      {
        if ( action_key == "burn_action" )
        {
          worlditem.actions.emplace( typeid( Cmp::BurnAction ),
                                     Cmp::BurnAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                      { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                      { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "carry_action" )
        {
          worlditem.actions.emplace( typeid( Cmp::CarryAction ),
                                     Cmp::CarryAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                       { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                       { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "consume_action" )
        {
          worlditem.actions.emplace( typeid( Cmp::ConsumeAction ),
                                     Cmp::ConsumeAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                         { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                         { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "destroy_action" )
        {
          worlditem.actions.emplace( typeid( Cmp::DestroyAction ),
                                     Cmp::DestroyAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                         { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                         { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "spawn_action" )
        {
          worlditem.actions.emplace( typeid( Cmp::SpawnAction ),
                                     Cmp::SpawnAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                       { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                       { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "proximity_action" )
        {
          worlditem.actions.emplace( typeid( Cmp::ProximityAction ),
                                     Cmp::ProximityAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                           { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                           { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "sacrifice_action" )
        {
          worlditem.actions.emplace( typeid( Cmp::SacrificeAction ),
                                     Cmp::SacrificeAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                           { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                           { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else { SPDLOG_WARN( "Unknown action key: {}", action_key ); }
      }
    }
    m_store.emplace( item_key, std::move( worlditem ) );
    SPDLOG_DEBUG( "Loaded item: {} ({})", item_key, sprite_mtype );
  }
  SPDLOG_DEBUG( "Item store loaded with {} items", m_store.size() );
}

} // namespace Game::Sys