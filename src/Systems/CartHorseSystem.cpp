#include <Audio/SoundBank.hpp>
#include <Components/Cart/Barrel.hpp>
#include <Components/Cart/BarrelOverlayOpen.hpp>
#include <Components/Cart/MultiBlock.hpp>
#include <Components/Horse/MultiBlock.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Position.hpp>
#include <Events/OverlayEvent.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Systems/CartHorseSystem.hpp>
#include <Utils/Player.hpp>

namespace Game::Sys
{

CartHorseSystem::CartHorseSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank,
                                  entt::dispatcher &scenemanager_event_dispatcher )
    : BaseSystem( reg, window, sound_bank ),
      m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
{
  SPDLOG_DEBUG( "CartHorseSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&CartHorseSystem::on_player_action>( this );
}

void CartHorseSystem::update( sf::Time dt )
{
  m_activate_timer += dt;

  for ( auto [cart_entt, cart_mb_cmp] : reg().view<Cmp::Cart::MultiBlock>().each() )
  {
    if ( is_player_facing( cart_mb_cmp ) ) { reg().emplace_or_replace<Cmp::Cart::BarrelOverlayOpen>( cart_entt ); }
    else { reg().remove<Cmp::Cart::BarrelOverlayOpen>( cart_entt ); }
  }
}

void CartHorseSystem::on_player_action( Events::PlayerActionEvent ev )
{
  if ( ev.action != Events::PlayerActionEvent::GameActions::ACTIVATE ) return;

  if ( m_activate_timer < m_activate_timeout ) return;

  for ( auto [horse_entt, horse_mb_cmp, horse_pos_cmp] : reg().view<Cmp::Horse::MultiBlock, Cmp::Position>().each() )
  {
    if ( not is_player_facing( horse_pos_cmp ) ) continue;
    // no scene transitions mid-flight - the player must land first
    if ( Utils::Player::is_flying( reg() ) ) continue;
    if ( m_sound_bank.get_effect( "horse_neigh" ).getStatus() != sf::Sound::Status::Playing ) m_sound_bank.get_effect( "horse_neigh" ).play();
    m_activate_timer = sf::Time::Zero;
    // OverlayArbitrationSystem decides whether the prompt may open
    get_systems_event_queue().trigger( Events::OverlayEvent( Events::OverlayEvent::OverlayActions::LEAVE_PROMPT_OPEN ) );
    return;
  }
  for ( auto [cart_entt, cart_mb_cmp] : reg().view<Cmp::Cart::MultiBlock>().each() )
  {
    if ( not is_player_facing( cart_mb_cmp ) ) continue;
    if ( transfer_selected_item() ) { m_activate_timer = sf::Time::Zero; }
    break;
  }
}

void CartHorseSystem::leave_level()
{
  SPDLOG_INFO( "Player left the graveyard by the horse" );
  // the exit key is only consumed by unlocking the exit; drop it so it doesn't follow the player into the next scene
  // and keep the compass's exit needle alive
  Factory::Player::destroy_inventory( reg(), "item.exitkey" );
  m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::LEVEL_COMPLETE );
  Factory::Player::remove_player_last_graveyard_pos( reg() );
}

bool CartHorseSystem::is_player_facing( const sf::FloatRect &bounds )
{
  return Utils::Player::is_player_near( reg(), bounds ) and Utils::Player::get_projected_position( reg() ).findIntersection( bounds ).has_value();
}

bool CartHorseSystem::transfer_selected_item()
{
  if ( Utils::Player::is_player_eating( reg() ) ) return false;

  const auto slot_entt = Utils::Player::get_selected_slot_entt( reg() );
  if ( slot_entt == entt::null ) return false;
  const auto &slot_cmp = reg().get<Cmp::PlayerInventorySlot>( slot_entt );
  if ( slot_cmp.empty() ) return false;

  // a copy: taking the last item empties the slot
  const Sys::SpriteKey item_sprite = slot_cmp.m_item->sprite_type;
  for ( auto [barrel_entt, barrel_cmp] : reg().view<Cmp::Cart::Barrel>().each() )
  {
    bool *part = barrel_cmp.part( item_sprite );
    if ( not part ) return false; // not a cadaver part
    if ( *part ) continue;

    *part = true;
    Factory::Player::take_one_from_slot( reg(), slot_entt );
    m_sound_bank.get_effect( "drop_inventory" ).play();
    return true;
  }
  return false;
}

} // namespace Game::Sys
