#include <Audio/SoundBank.hpp>
#include <Components/Cart/MultiBlock.hpp>
#include <Components/Horse/MultiBlock.hpp>
#include <Components/Position.hpp>
#include <Systems/CartHorseSystem.hpp>
#include <Utils/Player.hpp>

namespace Game::Sys
{

CartHorseSystem::CartHorseSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank,
                                  entt::dispatcher &scenemanager_event_dispatcher )
    : BaseSystem( reg, window, sound_bank ),
      m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
{
  SPDLOG_DEBUG( "ExitSystem initialized" );
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&CartHorseSystem::on_player_action>( this );
}

void CartHorseSystem::update( [[maybe_unused]] sf::Time dt )
{
  //
  m_activate_timer += dt;
}

void CartHorseSystem::on_player_action( Events::PlayerActionEvent ev )
{
  if ( ev.action != Events::PlayerActionEvent::GameActions::ACTIVATE ) return;

  if ( m_activate_timer < m_activate_timeout ) return;
  m_activate_timer = sf::Time::Zero;

  for ( auto [horse_entt, horse_mb_cmp, horse_pos_cmp] : reg().view<Cmp::Horse::MultiBlock, Cmp::Position>().each() )
  {
    if ( not Utils::Player::is_player_near( reg(), horse_pos_cmp ) ) continue;
    if ( not Utils::Player::get_projected_position( reg() ).findIntersection( horse_pos_cmp ) ) continue;
    if ( m_sound_bank.get_effect( "horse_neigh" ).getStatus() != sf::Sound::Status::Playing ) m_sound_bank.get_effect( "horse_neigh" ).play();
  }
  for ( auto [cart_entt, cart_mb_cmp, cart_pos_cmp] : reg().view<Cmp::Cart::MultiBlock, Cmp::Position>().each() )
  {
    if ( not Utils::Player::is_player_near( reg(), cart_pos_cmp ) ) continue;
    if ( not Utils::Player::get_projected_position( reg() ).findIntersection( cart_pos_cmp ) ) continue;
    SPDLOG_INFO( "Open Cart Barrels" );
  }
}

} // namespace Game::Sys