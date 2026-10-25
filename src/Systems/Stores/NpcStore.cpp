
#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/CollisionAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ProjectileAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SacrificeAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Factory/ToxicityFactory.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Utils/JsonDeserializer.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace Game::Sys
{

NpcStore::NpcStore( std::filesystem::path json_file_path )
    : StoreSingleton<NpcStore, Cmp::Npc::NPC>( std::move( json_file_path ) )
{
  init_store();
  SPDLOG_DEBUG( "NpcStore initialized" );
}

void NpcStore::init_store()
{
  nlohmann::json json = Utils::JsonDeserializer::load_json_file( m_json_file_path );
  for ( const auto &[item_key, item_value] : json.items() )
  {
    std::vector<Sys::SpriteKey> mtype_list;
    for ( const auto &sprite_json : item_value.at( "sprites" ) )
    {
      mtype_list.emplace_back( sprite_json.get<std::string>() );
    }

    auto lerp_speed = item_value.at( "lerpspeed" );

    Cmp::Npc::NPC npc( mtype_list, lerp_speed );
    for ( const auto &action_entry : item_value.at( "actions" ) )
    {
      for ( const auto &[action_key, action_value] : action_entry.items() )
      {
        try
        {
          if ( action_key == Cmp::BurnAction::kKey ) { npc.emplace( make_action<Cmp::BurnAction>( action_value ) ); }
          else if ( action_key == Cmp::CarryAction::kKey ) { npc.emplace( make_action<Cmp::CarryAction>( action_value ) ); }
          else if ( action_key == Cmp::ConsumeAction::kKey ) { npc.emplace( make_action<Cmp::ConsumeAction>( action_value ) ); }
          else if ( action_key == Cmp::DestroyAction::kKey ) { npc.emplace( make_action<Cmp::DestroyAction>( action_value ) ); }
          else if ( action_key == Cmp::SpawnAction::kKey ) { npc.emplace( make_action<Cmp::SpawnAction>( action_value ) ); }
          else if ( action_key == Cmp::ProximityAction::kKey ) { npc.emplace( make_action<Cmp::ProximityAction>( action_value ) ); }
          else if ( action_key == Cmp::SacrificeAction::kKey ) { npc.emplace( make_action<Cmp::SacrificeAction>( action_value ) ); }
          else if ( action_key == Cmp::ProjectileAction::kKey ) { npc.emplace( make_action<Cmp::ProjectileAction>( action_value ) ); }
          else if ( action_key == Cmp::CollisionAction::kKey ) { npc.emplace( make_action<Cmp::CollisionAction>( action_value ) ); }
          else { SPDLOG_WARN( "Unknown action key: {}", action_key ); }
        } catch ( const std::exception &e )
        {
          rethrow_with_context( item_key, action_key, action_value, e );
        }
      }
    }
    m_store.emplace( key_type( item_key ), std::move( npc ) );
    SPDLOG_DEBUG( "Loaded item: {}", item_key );
  }
  SPDLOG_DEBUG( "NPC store loaded with {} items", m_store.size() );
}

} // namespace Game::Sys