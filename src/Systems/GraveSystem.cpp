
#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/AnimData.hpp>
#include <Components/Grave/Consequence.hpp>
#include <Components/Grave/MultiBlock.hpp>
#include <Components/Grave/Segment.hpp>
#include <Components/Particle/GraveHaloParticleSprite.hpp>
#include <Components/Persistent/AxeGraveDigWear.hpp>
#include <Components/Persistent/DiggingCooldownThreshold.hpp>
#include <Components/Persistent/DiggingDamagePerHit.hpp>
#include <Components/Persistent/PickaxeGraveDigWear.hpp>
#include <Components/Persistent/ShovelGraveDigWear.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/DiggingTimer.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Position.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SelectedPosition.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/CreateNpcEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Events/UpdateDmgEvent.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/GraveSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Maths.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <utility>

namespace Game::Sys
{

GraveSystem::GraveSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&GraveSystem::on_player_action>( this );
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

    if ( holding_flute and consequence.get( player_luck ) == Cmp::Grave::Consequence::Type::NPC_TRAP and wearlevel > 0 )
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

  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  float dmg = 0.f;
  if ( inventory_type == "item.pickaxe" ) { dmg = Sys::PersistSystem::get<Cmp::Persist::PickaxeGraveDigWear>( reg() ).get_value(); }
  else if ( inventory_type == "item.axe" ) { dmg = Sys::PersistSystem::get<Cmp::Persist::AxeGraveDigWear>( reg() ).get_value(); }
  else if ( inventory_type == "item.shovel" ) { dmg = Sys::PersistSystem::get<Cmp::Persist::ShovelGraveDigWear>( reg() ).get_value(); }
  get_systems_event_queue().trigger( Events::UpdateDmgEvent( Utils::Player::get_inventory_entt( reg() ), dmg ) );

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
  if ( std::string::size_type n = grave_anim_cmp.m_sprite_type.str().find( ".closed" ); n != std::string::npos )
  {
    grave_anim_cmp.m_sprite_type = Sys::SpriteKey( grave_anim_cmp.m_sprite_type.str().substr( 0, n ) + ".opened" );
    SPDLOG_DEBUG( "Grave Cmp::SpriteAnimation changed to opened type: {}", grave_anim_cmp.m_sprite_type );

    // select the final smash sound
    m_sound_bank.get_effect( "pickaxe_final" ).play();
  }

  auto *consequence = reg().try_get<Cmp::Grave::Consequence>( grave_entity );
  if ( not consequence ) return;
  switch ( consequence->get( Utils::Player::get_stats( reg() ).luck() ) )
  {
    case Cmp::Grave::Consequence::Type::NPC_TRAP:
      SPDLOG_DEBUG( "Grave activated NPC trap." );
      get_systems_event_queue().trigger( Events::CreateNpcEvent( "npc.ghost", reg().get<Cmp::Position>( grave_entity ) ) );
      m_sound_bank.get_effect( "spawn_ghost" ).play();
      break;
    case Cmp::Grave::Consequence::Type::BOMB_TRAP:
      SPDLOG_DEBUG( "Grave activated bomb trap." );
      get_systems_event_queue().trigger( Events::PlayerActionEvent( Events::PlayerActionEvent::GameActions::TRIGGER_BOMB ) );
      break;
    case Cmp::Grave::Consequence::Type::RELIC:
      spawn_grave_loot( { "item.relic1", "item.relic2", "item.relic3", "item.relic4" } );
      break;
    case Cmp::Grave::Consequence::Type::JEWELRY:
      spawn_grave_loot( { "item.jewelry_sapphire_necklace", "item.jewelry_amephyst_ring", "item.jewelry_ruby_ring", "item.jewelry_emerald_necklace",
                          "item.jewelry_emerald_gemstone", "item.jewelry_sapphire_gemstone", "item.jewelry_diamond_gemstone",
                          "item.jewelry_amephyst_gemstone" } );
      break;
    case Cmp::Grave::Consequence::Type::CURSE_TABLET:
      spawn_grave_loot( { "item.cursetablet" } );
      break;
  }
}

void GraveSystem::spawn_grave_loot( const std::vector<Sys::ItemKey> &loot_pool )
{
  Cmp::RandomInt loot_picker( 0, static_cast<int>( loot_pool.size() ) - 1 );
  const auto &selected_item_type = loot_pool.at( static_cast<std::size_t>( loot_picker.gen() ) );

  get_systems_event_queue().trigger( Events::CreateItemEvent( Utils::Player::get_position( reg() ), selected_item_type, "drop_loot" ) );
  const auto spawn_action = Utils::Player::apply_action_from_item_store<Cmp::SpawnAction>( reg(), selected_item_type );
  trigger_side_effects( spawn_action, Utils::Player::get_position( reg() ) );
}

void GraveSystem::on_player_action( const Events::PlayerActionEvent &event )
{
  if ( event.action == Events::PlayerActionEvent::GameActions::DIG ) { player_digging = true; }
}

} // namespace Game::Sys