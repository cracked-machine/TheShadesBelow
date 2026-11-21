
#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/AnimData.hpp>
#include <Components/Grave/Consequence.hpp>
#include <Components/Grave/Exhumed.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Grave/KeyGrave.hpp>
#include <Components/Inventory/BlastProof.hpp>
#include <Components/Inventory/WorldItem.hpp>
#include <Components/Npc/Crow.hpp>
#include <Components/Particle/FlameParticleSprite.hpp>
#include <Components/Particle/GraveHaloParticleSprite.hpp>
#include <Components/Persistent/DiggingCooldownThreshold.hpp>
#include <Components/Persistent/DiggingDamagePerHit.hpp>
#include <Components/Persistent/ShovelGraveDigWear.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/DiggingTimer.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SelectedPosition.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/AltarActivatedEvent.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/CreateNpcEvent.hpp>
#include <Events/CrowGuideEvent.hpp>
#include <Events/ExhumeGraveEvent.hpp>
#include <Events/KeyGraveExhumedEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/GraveSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <entt/entity/fwd.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace Game::Sys
{

GraveSystem::GraveSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&GraveSystem::on_player_action>( this );
  std::ignore = get_systems_event_queue().sink<Events::AltarActivatedEvent>().connect<&GraveSystem::on_altar_activated>( this );
  std::ignore = get_systems_event_queue().sink<Events::ExhumeGraveEvent>().connect<&GraveSystem::on_exhume_grave>( this );
}

void GraveSystem::update()
{
  // must run before the digging early-outs below: the flute is not a digging tool
  update_grave_halo_particles();

  // consume the dig request on the frame it arrives: leaving it latched across the early-outs below would
  // land a hit on a grave once the cooldown expires, long after the click that was aimed at something else
  if ( not std::exchange( player_digging, false ) ) return;
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

    if ( not is_player_near( grave_cmp ) ) continue;

    // check player is facing the grave
    if ( not Utils::Player::get_projected_position( reg() ).findIntersection( grave_cmp ) ) continue;

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
  if ( inventory_slot_type != "item.shovel" ) { return false; }

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
  for ( auto [pc_entt, pc_cmp, pc_pos_cmp] : reg().view<Cmp::Player::Character, const Cmp::Position>().each() )
  {
    auto player_hitbox = Cmp::RectBounds::scaled( pc_pos_cmp.position, Constants::kGridSizePxF, kPlayerProximityScale );
    if ( player_hitbox.findIntersection( grave_cmp ) ) return true;
  }
  return false;
}

void GraveSystem::update_grave_halo_particles()
{
  auto ps_tag = std::string( Cmp::Particle::GraveHaloParticleSprite::kTag );
  auto ps_list = Sys::ParticleSystem::find( reg(), ps_tag );
  auto [inventory_entt, inventory_type, _] = Utils::Player::get_inventory( reg() );
  const bool holding_flute = inventory_type == "item.elderflute";
  auto wearlevel = Utils::Player::get_inventory_wear_level( reg() );
  const int player_luck = Utils::Player::get_stats( reg() ).luck();

  for ( auto [grave_mb_entt, grave_mb, grave_uuid, consequence] : reg().view<Cmp::Grave::MultiBlock, Cmp::UUID, Cmp::Grave::Consequence>().each() )
  {
    // only one halo per grave: find the one this grave already owns, if any
    auto halo = std::ranges::find_if( ps_list, [&]( const auto &entt_ps )
    {
      auto *ps_uuid = reg().try_get<Cmp::UUID>( entt_ps.first );
      return ps_uuid and * ps_uuid == grave_uuid;
    } );
    const bool has_halo = halo != ps_list.end();

    if ( holding_flute and consequence.roll( player_luck ) == Cmp::Grave::Consequence::Type::NPC_TRAP and wearlevel > 0 )
    {
      // restart is a no-op if already running
      if ( has_halo ) { halo->second.get().restart(); }
      else
      {
        Factory::Particle::add_grave_halo_ps( reg(), ps_tag, 1.f, 10.f, grave_uuid, grave_mb.getCenter(), grave_mb.position.y + grave_mb.size.y + 1 );
      }
    }
    else if ( has_halo )
    {
      // flute not held, or a luck change means this is no longer a ghost grave: let the existing particles die out
      halo->second.get().stop();
    }
  }
}

void GraveSystem::apply_dig_hit( entt::entity grave_entity, Cmp::Grave::MultiBlock &grave_cmp, Cmp::AnimData &grave_anim_cmp )
{
  constexpr float kGraveMaxHp = 255.f;

  // has_digging_tool_equipped() guarantees the shovel is equipped
  const float dmg = Sys::PersistSystem::get<Cmp::Persist::ShovelGraveDigWear>( reg() ).get_value();
  get_systems_event_queue().trigger( Events::UpdateDmgEvent( Utils::Player::get_inventory_entt( reg() ), dmg ) );

  grave_cmp.hp -= Utils::Maths::to_percent( kGraveMaxHp, Sys::PersistSystem::get<Cmp::Persist::DiggingDamagePerHit>( reg() ).get_value() );

  if ( grave_cmp.hp > 0 )
  {
    // play bashing animation
    m_sound_bank.get_effect( "hit_grave" ).play();
    return;
  }

  open_grave( grave_entity, grave_anim_cmp, ExhumedBy::PLAYER );
}

void GraveSystem::on_exhume_grave( const Events::ExhumeGraveEvent &event )
{
  const entt::entity grave_entity = event.m_grave_entt;
  if ( not reg().valid( grave_entity ) or not reg().all_of<Cmp::Grave::MultiBlock>( grave_entity ) ) return;
  if ( reg().all_of<Cmp::Grave::Exhumed>( grave_entity ) ) return;
  auto *grave_anim_cmp = reg().try_get<Cmp::AnimData>( grave_entity );
  if ( not grave_anim_cmp ) return;
  SPDLOG_INFO( "Grave {} exhumed by a bomb blast", static_cast<uint32_t>( grave_entity ) );
  open_grave( grave_entity, *grave_anim_cmp, ExhumedBy::BOMB );
}

Cmp::Position GraveSystem::grave_bounds( entt::entity grave_entity )
{
  const auto &grave_cmp = reg().get<Cmp::Grave::MultiBlock>( grave_entity );
  return Cmp::Position( grave_cmp.position, grave_cmp.size );
}

void GraveSystem::open_grave( entt::entity grave_entity, Cmp::AnimData &grave_anim_cmp, ExhumedBy by )
{
  if ( std::string::size_type n = grave_anim_cmp.m_sprite_type.str().find( ".closed" ); n != std::string::npos )
  {
    grave_anim_cmp.m_sprite_type = Sys::SpriteKey( grave_anim_cmp.m_sprite_type.str().substr( 0, n ) + ".opened" );
    SPDLOG_DEBUG( "Grave Cmp::SpriteAnimation changed to opened type: {}", grave_anim_cmp.m_sprite_type );

    // select the final smash sound; a blast is loud enough on its own
    if ( by == ExhumedBy::PLAYER ) { m_sound_bank.get_effect( "pickaxe_final" ).play(); }
  }

  reg().emplace_or_replace<Cmp::Grave::Exhumed>( grave_entity );

  // the key grave skips its own consequence: it holds the exit key
  if ( reg().all_of<Cmp::Grave::KeyGrave>( grave_entity ) )
  {
    reg().remove<Cmp::Grave::KeyGrave>( grave_entity );
    drop_exit_key( grave_entity );
    return;
  }

  auto *consequence = reg().try_get<Cmp::Grave::Consequence>( grave_entity );
  if ( not consequence ) return;
  switch ( consequence->roll( Utils::Player::get_stats( reg() ).luck() ) )
  {
    case Cmp::Grave::Consequence::Type::NPC_TRAP:
      SPDLOG_DEBUG( "Grave activated NPC trap." );
      get_systems_event_queue().trigger( Events::CreateNpcEvent( "npc.ghost", reg().get<Cmp::Position>( grave_entity ) ) );
      m_sound_bank.get_effect( "spawn_ghost" ).play();
      break;
    case Cmp::Grave::Consequence::Type::BOMB_TRAP:
      SPDLOG_DEBUG( "Grave activated bomb trap." );
      // a blasted grave sets its bomb off where it lies, chaining into any graves around it
      get_systems_event_queue().trigger(
          Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::TRIGGER_BOMB, by == ExhumedBy::BOMB ? grave_entity : entt::null ) );
      break;
    case Cmp::Grave::Consequence::Type::BLOOD:
      spawn_blood_loot( grave_entity, by );
      break;
    case Cmp::Grave::Consequence::Type::CURSE_TABLET:
      spawn_grave_loot(
          {
              "item.cursetablet.ghost",
              "item.cursetablet.skeleton",
              "item.cursetablet.adder",
          },
          grave_entity, by );
      break;
    case Cmp::Grave::Consequence::Type::TORCH:
      spawn_grave_loot( { "item.torch" }, grave_entity, by );
      break;
    case Cmp::Grave::Consequence::Type::TALISMAN:
      spawn_grave_loot( Sys::ItemStore::instance().get_item_matches( "item.talisman*" ), grave_entity, by );
      break;
  }
}

void GraveSystem::spawn_blood_loot( entt::entity grave_entity, ExhumedBy by )
{
  // any blood vial that heals will do, so new blood items join the pool through items.json alone
  std::vector<Sys::ItemKey> loot_pool;
  for ( const auto &item_key : Sys::ItemStore::instance().get_all() )
  {
    if ( Sys::ItemStore::instance().get( item_key ).get_blood_heal() > 0 ) loot_pool.push_back( item_key );
  }
  if ( loot_pool.empty() )
  {
    SPDLOG_WARN( "Grave rolled blood but no blood item in items.json has a consume_action health" );
    return;
  }
  spawn_grave_loot( loot_pool, grave_entity, by );
}

void GraveSystem::light_torch_loot( entt::entity item_entt )
{
  const auto *world_item_cmp = reg().try_get<Cmp::WorldItem>( item_entt );
  if ( not world_item_cmp or world_item_cmp->item_type != "item.torch" ) return;
  auto *uuid_cmp = reg().try_get<Cmp::UUID>( item_entt );
  const auto *pos_cmp = reg().try_get<Cmp::Position>( item_entt );
  const auto *zorder_cmp = reg().try_get<Cmp::ZOrderValue>( item_entt );
  if ( not uuid_cmp or not pos_cmp or not zorder_cmp ) return;
  // shares the torch's UUID so ParticleSystem keeps the flame on it and pickup/drop moves the flame with it
  Factory::Particle::add_flame( reg(), kTorchFlameTag, *uuid_cmp,
                                { pos_cmp->getCenter().x, pos_cmp->getCenter().y - Cmp::Particle::FlameParticleSprite::kVerticalOffset },
                                zorder_cmp->get() + 1.f, Cmp::Particle::kWorldScalePreset );
}

void GraveSystem::spawn_grave_loot( const std::vector<Sys::ItemKey> &loot_pool, entt::entity grave_entity, ExhumedBy exhumedby )
{
  Cmp::RandomInt loot_picker( 0, static_cast<int>( loot_pool.size() ) - 1 );
  const auto &selected_item_type = loot_pool.at( static_cast<std::size_t>( loot_picker.gen() ) );

  if ( exhumedby == ExhumedBy::BOMB )
  {
    // drop on a free cell beside the grave, and spare it from the blast that exhumed it
    const Cmp::Position bounds = grave_bounds( grave_entity );
    get_systems_event_queue().trigger( Events::CreateItemEvent( bounds, selected_item_type, "drop_loot", 0.f, true, 0, entt::null,
                                                                [this]( entt::entity item_entt )
    {
      reg().emplace_or_replace<Cmp::Inventory::BlastProof>( item_entt );
      light_torch_loot( item_entt );
    }, /*sting=*/true, /*player_reward=*/true ) );
    const auto spawn_action = Utils::Player::apply_action_from_item_store<Cmp::SpawnAction>( reg(), selected_item_type );
    play_action_sfx( spawn_action );
    trigger_side_effects( spawn_action, bounds );
    return;
  }

  get_systems_event_queue().trigger( Events::CreateItemEvent( Utils::Player::get_position( reg() ), selected_item_type, "drop_loot", 0.f, false, 0,
                                                              entt::null, [this]( entt::entity item_entt ) { light_torch_loot( item_entt ); },
                                                              /*sting=*/true, /*player_reward=*/true ) );
  const auto spawn_action = Utils::Player::apply_action_from_item_store<Cmp::SpawnAction>( reg(), selected_item_type );
  play_action_sfx( spawn_action );
  trigger_side_effects( spawn_action, Utils::Player::get_position( reg() ) );
}

void GraveSystem::drop_exit_key( entt::entity grave_entity )
{
  const Sys::ItemKey kExitKey{ "item.exitkey" };

  // the key is left beside the grave (keys survive blasts anyway) for the player to pick up and carry to the exit or an
  // altar. Events::ItemPickedUpEvent is sent when they pick it up.
  const Cmp::Position bounds = grave_bounds( grave_entity );
  get_systems_event_queue().trigger( Events::CreateItemEvent( bounds, kExitKey, "drop_loot", 0.f, true, 0, entt::null,
                                                              [this]( entt::entity item_entt )
  { reg().emplace_or_replace<Cmp::Inventory::BlastProof>( item_entt ); }, /*sting=*/true, /*player_reward=*/true ) );
  const auto spawn_action = Utils::Player::apply_action_from_item_store<Cmp::SpawnAction>( reg(), kExitKey );
  play_action_sfx( spawn_action );
  trigger_side_effects( spawn_action, bounds );
  SPDLOG_INFO( "Key grave {} exhumed, exit key dropped", static_cast<uint32_t>( grave_entity ) );

  // its job done, the guide crow flies away for good
  for ( auto [crow_entt, crow_cmp] : reg().view<Cmp::Npc::Crow>().each() )
  {
    if ( crow_cmp.guide_grave == grave_entity ) crow_cmp.flee = true;
  }

  // let AltarSystem release the activated altar so the player can activate another. Enqueued as the caller may be
  // iterating registry views
  get_systems_event_queue().enqueue( Events::KeyGraveExhumedEvent{} );
}

void GraveSystem::on_altar_activated( const Events::AltarActivatedEvent &event )
{
  // any un-exhumed grave can hold the key, but one out of view makes the crow's guidance worth following
  const sf::FloatRect view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );
  std::vector<entt::entity> hidden;
  std::vector<entt::entity> visible;
  for ( auto [grave_entt, grave_pos, grave_mb] : reg().view<Cmp::Position, Cmp::Grave::MultiBlock>( entt::exclude<Cmp::Grave::Exhumed> ).each() )
  {
    ( Utils::is_visible_in_view( view_bounds, grave_pos ) ? visible : hidden ).push_back( grave_entt );
  }
  const auto &pool = hidden.empty() ? visible : hidden;
  if ( pool.empty() )
  {
    // nowhere to hide the key: hand it over now so the level can still be exited
    SPDLOG_WARN( "No un-exhumed graves left to hold the exit key" );
    spawn_grave_loot( { "item.exitkey" }, entt::null, ExhumedBy::PLAYER );
    get_systems_event_queue().enqueue( Events::KeyGraveExhumedEvent{} );
    return;
  }

  Cmp::RandomInt grave_picker( 0, static_cast<int>( pool.size() ) - 1 );
  const entt::entity key_grave = pool.at( static_cast<std::size_t>( grave_picker.gen() ) );
  reg().emplace_or_replace<Cmp::Grave::KeyGrave>( key_grave );
  SPDLOG_INFO( "Altar activated: grave {} holds the exit key ({} hidden / {} visible candidates)", static_cast<uint32_t>( key_grave ),
               hidden.size(), visible.size() );
  // NpcSystem sends the guide crow from the altar
  get_systems_event_queue().enqueue( Events::CrowGuideEvent{ .key_grave = key_grave, .altar_position = event.altar_position } );
}

void GraveSystem::on_player_action( const Events::PlayerActionEvent &event )
{
  if ( event.action == Events::PlayerActionEvent::GameActions::DIG ) { player_digging = true; }
}

} // namespace Game::Sys