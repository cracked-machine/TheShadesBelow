#include <Audio/SoundBank.hpp>
#include <Components/Exit.hpp>
#include <Components/Inventory/FlashUIWealth.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/LastGraveyardPosition.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Position.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Spring/HealingSpringBuildingMultiBlock.hpp>
#include <Components/Spring/HealingSpringBuildingSegment.hpp>
#include <Components/Spring/HealingSpringEntrance.hpp>
#include <Components/Spring/HealingSpringMultiBlock.hpp>
#include <Components/Wall.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/HealingSpringSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Time.hpp>

namespace Game::Sys
{

void HealingSpringSystem::on_player_action( Events::PlayerActionEvent ev )
{
  if ( ev.action != Events::PlayerActionEvent::GameActions::ACTIVATE ) return;
  check_inventory_deposit();
}

void HealingSpringSystem::update_building_zorder()
{
  auto player_pos = Utils::Player::get_position( reg() );

  for ( auto [mb_entt, mb_cmp, mb_z_cmp] : reg().view<Cmp::HealingSpringBuildingMultiBlock, Cmp::ZOrderValue>().each() )
  {
    if ( not player_pos.findIntersection( mb_cmp ) ) continue;
    auto segment_view = reg().view<Cmp::HealingSpringBuildingSegment, Cmp::Position, Cmp::ZOrderValue>();
    for ( auto [segment_entt, segment_cmp, segment_pos_cmp, segment_z_cmp] : segment_view.each() )
    {
      if ( not player_pos.findIntersection( segment_pos_cmp ) ) continue;
      mb_z_cmp.set( segment_z_cmp.get() );
      SPDLOG_DEBUG( "Updated zorder to {}", segment_z_cmp.get() );
    }
  }
}

void HealingSpringSystem::check_entrance_collision()
{
  auto player_pos = Utils::Player::get_position( reg() );
  auto door_view = reg().view<Cmp::HealingSpringEntrance, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [door_entity, door_cmp, door_pos_cmp] : door_view.each() )
  {
    // optimize: skip if not visible
    if ( !Utils::is_visible_in_view( view_bounds, door_pos_cmp ) ) continue;

    // shrink entrance bounds slightly for better UX
    auto decreased_entrance_bounds = Cmp::RectBounds::scaled( door_pos_cmp.position, door_pos_cmp.size, 0.1f, Cmp::RectBounds::ScaleAxis::XY );

    if ( not player_pos.findIntersection( decreased_entrance_bounds.getBounds() ) ) continue;
    m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::ENTER_SACREDSPRING );

    Factory::Player::remove_player_last_graveyard_pos( reg() );
    Cmp::Position last_known_pos(
        {
            door_pos_cmp.position.x,
            door_pos_cmp.position.y + Constants::kGridSizePxF.y,
        },
        Constants::kGridSizePxF );
    SPDLOG_INFO( "Last known graveyard position {}, {}", last_known_pos.position.x, last_known_pos.position.y );
    Factory::Player::add_player_last_graveyard_pos( reg(), last_known_pos );
    break;
  }
}

void HealingSpringSystem::check_exit_collision()
{
  auto player_pos = Utils::Player::get_position( reg() );
  auto door_view = reg().view<Cmp::Exit, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [door_entity, door_cmp, door_pos_cmp] : door_view.each() )
  {
    // optimize: skip if not visible
    if ( !Utils::is_visible_in_view( view_bounds, door_pos_cmp ) ) continue;

    auto decreased_entrance_bounds = Cmp::RectBounds::scaled( door_pos_cmp.position, door_pos_cmp.size, 0.1f,
                                                              Cmp::RectBounds::ScaleAxis::XY ); // shrink entrance bounds slightly for better UX

    if ( not player_pos.findIntersection( decreased_entrance_bounds.getBounds() ) ) continue;
    m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::EXIT_SACREDSPRING );
  }
}

void HealingSpringSystem::check_inventory_deposit()
{

  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( not inventory_type.contains( "item.cursetablet" ) ) return;

  auto &wealth = Utils::Player::get_wealth( m_reg );

  // check if we're near a healing spring
  auto player_hitbox = Cmp::RectBounds::scaled( Utils::Player::get_position( reg() ).position, Constants::kGridSizePxF, 1.5f );
  for ( auto [well_entt, well_mb_cmp] : reg().view<Cmp::HealingSpringMultiBlock>().each() )
  {
    if ( not player_hitbox.findIntersection( well_mb_cmp ) ) continue;
    Factory::Player::destroy_inventory( reg(), inventory_type );
    wealth.wealth += 2;
    m_sound_bank.get_effect( "get_key" ).play();

    // signal UI to flash
    auto flash_entt = reg().create();
    reg().emplace_or_replace<Cmp::FlashUIWealth>( flash_entt );
  }
  m_inventory_deposit_interval = sf::Time::Zero;
}

} // namespace Game::Sys