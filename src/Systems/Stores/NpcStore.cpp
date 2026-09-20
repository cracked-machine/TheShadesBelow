
#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/CollisionAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/ProjectileAction.hpp>
#include <Components/Stats/ProximityAction.hpp>
#include <Components/Stats/SacrificeAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Systems/Stores/NpcStore.hpp>

#include <nlohmann/json.hpp>

namespace Game::Sys
{

NpcStore::NpcStore( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : StoreSingleton<NpcStore, Cmp::Npc::NPC>( reg, window, sprite_factory, sound_bank )
{
  s_instance = this;
  m_json_file_path = "res/json/npc.json";
  init_store();
  SPDLOG_DEBUG( "NpcStore initialized" );
}

void NpcStore::init_store()
{
  nlohmann::json json = load_json_file( m_json_file_path );
  for ( const auto &[item_key, item_value] : json.items() )
  {
    std::vector<Sprites::SpriteMetaType> mtype_list;
    for ( const auto &sprite_json : item_value.at( "sprites" ) )
    {
      mtype_list.push_back( sprite_json.get<std::string>() );
    }

    auto lerp_speed = item_value.at( "lerpspeed" );
    auto frame_rate = item_value.at( "framerate" );

    Cmp::Npc::NPC npc( mtype_list, lerp_speed, frame_rate );
    for ( const auto &action_entry : item_value.at( "actions" ) )
    {
      for ( const auto &[action_key, action_value] : action_entry.items() )
      {
        if ( action_key == "burn_action" )
        {
          npc.actions.emplace( typeid( Cmp::BurnAction ),
                               Cmp::BurnAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "carry_action" )
        {
          npc.actions.emplace( typeid( Cmp::CarryAction ),
                               Cmp::CarryAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                 { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                 { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "consume_action" )
        {
          npc.actions.emplace( typeid( Cmp::ConsumeAction ),
                               Cmp::ConsumeAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                   { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                   { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "destroy_action" )
        {
          npc.actions.emplace( typeid( Cmp::DestroyAction ),
                               Cmp::DestroyAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                   { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                   { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "spawn_action" )
        {
          npc.actions.emplace( typeid( Cmp::SpawnAction ),
                               Cmp::SpawnAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                 { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                 { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "proximity_action" )
        {
          npc.actions.emplace( typeid( Cmp::ProximityAction ),
                               Cmp::ProximityAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                     { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                     { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "sacrifice_action" )
        {
          npc.actions.emplace( typeid( Cmp::SacrificeAction ),
                               Cmp::SacrificeAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                     { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                     { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "projectile_action" )
        {
          npc.actions.emplace( typeid( Cmp::ProjectileAction ),
                               Cmp::ProjectileAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                      { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                      { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else if ( action_key == "collision_action" )
        {
          npc.actions.emplace( typeid( Cmp::CollisionAction ),
                               Cmp::CollisionAction( { health( action_value ) }, { fear( action_value ) }, { despair( action_value ) },
                                                     { infamy( action_value ) }, { toxicity( action_value ) }, { luck( action_value ) },
                                                     { tick( action_value ) }, toxidrome( action_value ) ) );
        }
        else { SPDLOG_WARN( "Unknown action key: {}", action_key ); }
      }
    }
    m_store.emplace( item_key, std::move( npc ) );
    SPDLOG_DEBUG( "Loaded item: {}", item_key );
  }
  SPDLOG_DEBUG( "NPC store loaded with {} items", m_store.size() );
}

} // namespace Game::Sys