
#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/AnimData.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Persistent/DiggingCooldownThreshold.hpp>
#include <Components/Persistent/DiggingDamagePerHit.hpp>
#include <Components/Persistent/WeaponDegradePerHit.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/DiggingTimer.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SelectedPosition.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Factory/BombFactory.hpp>
#include <Factory/LootFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/GraveSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

namespace Game::Sys
{

GraveSystem::GraveSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&GraveSystem::on_player_action>( this );
}

void GraveSystem::update()
{
  if ( not has_digging_tool_equipped() ) return;
  if ( is_dig_on_cooldown() ) return;

  clear_stale_grave_selections();

  // Iterate through all closed grave entities
  auto position_view = reg().view<Cmp::Position, Cmp::Grave::MultiBlock, Cmp::AnimData>( entt::exclude<Cmp::SelectedPosition> );
  for ( auto [grave_entity, grave_pos_cmp, grave_cmp, grave_anim_cmp] : position_view.each() )
  {
    if ( grave_anim_cmp.m_sprite_type.contains( ".opened" ) ) continue;

    auto mouse_position_bounds = Utils::get_mouse_bounds_in_gameview( m_window, RenderSystem::get_world_view() );
    if ( not mouse_position_bounds.findIntersection( grave_cmp ) ) continue;

    SPDLOG_DEBUG( "Found diggable entity at position: [{}, {}]!", grave_cmp.position.x, grave_cmp.position.y );

    // TODO: check player is facing the obstacle
    if ( not is_player_near( grave_cmp ) ) continue;

    // We are in proximity to an entity that is a candidate for a new SelectedPosition component.
    // Add a new SelectedPosition component to the entity
    reg().emplace_or_replace<Cmp::SelectedPosition>( grave_entity, grave_pos_cmp.position );
    reg().emplace_or_replace<Cmp::Player::DiggingTimer>( Utils::Player::get_entity( reg() ) );

    apply_dig_hit( grave_entity, grave_cmp, grave_anim_cmp );

    // apply_dig_hit() can open the grave and trigger a consequence (spawning loot/NPC/bomb entities via
    // Events::CreateItemEvent/PlayerActionEvent, dispatched synchronously through .trigger()). Those handlers
    // emplace Cmp::Position/Cmp::AnimData onto new entities, which can reallocate the pools this view iterates -
    // continuing to iterate afterward would be undefined behaviour. Only one grave can match the mouse position
    // at a time anyway, so stop here rather than advancing the now-possibly-invalidated iterator.
    break;
  }
}

bool GraveSystem::has_digging_tool_equipped()
{
  auto [_, inventory_slot_type, _] = Utils::Player::get_inventory( reg() );
  if ( not inventory_slot_type.contains( "pickaxe" ) and not inventory_slot_type.contains( "axe" ) and not inventory_slot_type.contains( "shovel" ) )
  {
    return false;
  }

  return Utils::Player::get_inventory_wear_level( reg() ) > 0;
}

bool GraveSystem::is_dig_on_cooldown()
{
  auto digging_cooldown_amount = Sys::PersistSystem::get<Cmp::Persist::DiggingCooldownThreshold>( reg() ).get_value();
  auto *dig_cooldown = reg().try_get<Cmp::Player::DiggingTimer>( Utils::Player::get_entity( reg() ) );
  return ( dig_cooldown != nullptr ) and * dig_cooldown < sf::seconds( digging_cooldown_amount );
}

void GraveSystem::clear_stale_grave_selections()
{
  // Cooldown has expired: remove any existing SelectedPosition from graves only. This function's own
  // cooldown clock only restarts on an actual grave dig, so it sits expired (and this runs) on every
  // DIG event while digging anything else — clearing the whole registry's SelectedPosition here would
  // also wipe unrelated selections (e.g. the obstacle currently being dug) set by other systems
  auto selected_position_view = reg().view<Cmp::SelectedPosition, Cmp::Grave::MultiBlock>();
  for ( auto [existing_sel_entity, sel_cmp, grave_mb_cmp] : selected_position_view.each() )
  {
    reg().remove<Cmp::SelectedPosition>( existing_sel_entity );
  }
}

bool GraveSystem::is_player_near( const Cmp::Grave::MultiBlock &grave_cmp )
{
  constexpr float kPlayerProximityScale = 1.5f;
  for ( auto [pc_entt, pc_cmp, pc_pos_cmp] : reg().view<Cmp::Player::Character, Cmp::Position>().each() )
  {
    auto player_hitbox = Cmp::RectBounds::scaled( pc_pos_cmp.position, Constants::kGridSizePxF, kPlayerProximityScale );
    if ( player_hitbox.findIntersection( grave_cmp ) ) return true;
  }
  return false;
}

void GraveSystem::apply_dig_hit( entt::entity grave_entity, Cmp::Grave::MultiBlock &grave_cmp, Cmp::AnimData &grave_anim_cmp )
{
  constexpr float kGraveMaxHp = 255.f;

  float reduction_amount = Sys::PersistSystem::get<Cmp::Persist::WeaponDegradePerHit>( reg() ).get_value();
  Utils::Player::reduce_inventory_wear_level( reg(), reduction_amount );

  grave_cmp.hp -= Utils::Maths::to_percent( kGraveMaxHp, Sys::PersistSystem::get<Cmp::Persist::DiggingDamagePerHit>( reg() ).get_value() );

  if ( grave_cmp.hp > 0 )
  {
    // play bashing animation
    m_sound_bank.get_effect( "hit_grave" ).play();
    return;
  }

  open_grave( grave_entity, grave_anim_cmp );
}

void GraveSystem::open_grave( entt::entity grave_entity, Cmp::AnimData &grave_anim_cmp )
{
  if ( std::string::size_type n = grave_anim_cmp.m_sprite_type.find( ".closed" ); n != std::string::npos )
  {
    grave_anim_cmp.m_sprite_type = grave_anim_cmp.m_sprite_type.substr( 0, n ) + ".opened";
    SPDLOG_DEBUG( "Grave Cmp::SpriteAnimation changed to opened type: {}", grave_anim_cmp.m_sprite_type );

    // select the final smash sound
    m_sound_bank.get_effect( "pickaxe_final" ).play();
  }

  trigger_grave_consequence( grave_entity );
}

void GraveSystem::trigger_grave_consequence( entt::entity grave_entity )
{
  // Roulette wheel. We take the inverse of player luck and use both numbers to gate the spawn probablities.
  // For example, if player luck is 50, then the badluck is also 50
  auto player_luck_stat = Utils::Player::get_stats( reg() ).luck();
  const int good_weight = player_luck_stat;
  const int bad_weight = 100 - good_weight;

  auto grave_activation_rng = Cmp::RandomInt( 0, 99 );
  const int roll = grave_activation_rng.gen();

  const int tier1_threshold = bad_weight / 2;                   // Tier1 is a roll below 25
  const int tier2_threshold = ( bad_weight / 2 ) + 5;           // Tier2 is a roll between 25 and 30
  const int tier3_threshold = bad_weight;                       // Tier3 is a roll between 30 and 50
  const int tier4_threshold = bad_weight + ( good_weight / 2 ); // Tier4 is a roll between 50 and 75
                                                                // Remaining rolls between 75 and 100

  GraveConsequence consequence;
  if ( roll < tier1_threshold ) { consequence = GraveConsequence::BOMB_TRAP; }
  else if ( roll < tier2_threshold ) { consequence = GraveConsequence::CURSE_TABLET; }
  else if ( roll < tier3_threshold ) { consequence = GraveConsequence::NPC_TRAP; }
  else if ( roll < tier4_threshold ) { consequence = GraveConsequence::RELIC; }
  else { consequence = GraveConsequence::JEWELRY; }

  switch ( consequence )
  {
    case GraveConsequence::NPC_TRAP: {
      SPDLOG_DEBUG( "Grave activated NPC trap." );
      Factory::Npc::create_npc( reg(), grave_entity, "npc.ghost" );
      m_sound_bank.get_effect( "spawn_ghost" ).play();
      break;
    }
    case GraveConsequence::BOMB_TRAP: {
      SPDLOG_DEBUG( "Grave activated bomb trap." );
      get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::TRIGGER_BOMB ) );
      break;
    }
    case GraveConsequence::RELIC: {
      spawn_grave_loot( { "item.relic1", "item.relic2", "item.relic3", "item.relic4" } );
      break;
    }
    case GraveConsequence::JEWELRY: {
      spawn_grave_loot( { "item.jewelry_sapphire_necklace", "item.jewelry_amephyst_ring", "item.jewelry_ruby_ring", "item.jewelry_emerald_necklace",
                          "item.jewelry_emerald_gemstone", "item.jewelry_sapphire_gemstone", "item.jewelry_diamond_gemstone",
                          "item.jewelry_amephyst_gemstone" } );
      break;
    }
    case GraveConsequence::CURSE_TABLET:
      spawn_grave_loot( { "item.cursetablet" } );
      break;
  }
}

void GraveSystem::spawn_grave_loot( const std::vector<Sprites::SpriteMetaType> &loot_pool )
{
  Cmp::RandomInt loot_picker( 0, static_cast<int>( loot_pool.size() ) - 1 );
  const auto &selected_item_type = loot_pool.at( static_cast<std::size_t>( loot_picker.gen() ) );

  get_systems_event_queue().trigger( Events::CreateItemEvent( Utils::Player::get_position( reg() ), selected_item_type, "drop_loot" ) );
  Utils::Player::apply_action_from_item_store<Cmp::SpawnAction>( reg(), selected_item_type );
}

void GraveSystem::on_player_action( const Events::PlayerActionEvent &event )
{
  if ( event.action == Events::PlayerActionEvent::GameActions::DIG )
  {
    // Check for collisions with diggable obstacles
    update();
  }
}

} // namespace Game::Sys