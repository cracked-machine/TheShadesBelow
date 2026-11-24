#include <Systems/Crypt/Variants/ClassicCryptVariant.hpp>

#include <Audio/SoundBank.hpp>
#include <Components/Crypt/RoomClosed.hpp>
#include <Components/Crypt/RoomEnd.hpp>
#include <Components/Crypt/RoomStart.hpp>
#include <Components/Persistent/CryptShuffleTimeout.hpp>
#include <Factory/CryptFactory.hpp>
#include <Shaders/ScreenShudderShader.hpp>
#include <Systems/CryptSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/ShaderSystem.hpp>
#include <Utils/Crypt.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>

namespace Game::Sys::Crypt
{

void ClassicCryptVariant::on_first_enter()
{
  const auto timeout = Sys::PersistSystem::get<Cmp::Persist::CryptShuffleTimeout>( m_crypt.reg() ).get_value();
  Factory::Crypt::create_crypt_shuffle_timer( m_crypt.reg(), timeout );
  shuffle();
}

void ClassicCryptVariant::update( sf::Time dt )
{
  auto &reg = m_crypt.reg();
  if ( m_maze_unlocked )
  {
    Factory::Crypt::destroy_crypt_shuffle_timer( reg );
    return;
  }

  Utils::Crypt::update_crypt_shuffle_timer( reg, dt );
  if ( Utils::Crypt::is_crypt_shuffle_timer_expired( reg ) ) shuffle();
}

void ClassicCryptVariant::shuffle()
{
  auto &reg = m_crypt.reg();
  if ( auto shudder = Sys::ShaderSystem::find<Sprites::ScreenShudderShader>( reg, "ScreenShudder" ) )
    shudder->get().trigger( /*duration=*/sf::seconds( 0.5f ), /*amplitude=*/3.f, /*frequency=*/16.f );
  auto uuid_map = m_crypt.build_non_obstacle_uuid_map();

  auto selected_rooms = Utils::Rnd::get_n_rand_components<Cmp::Crypt::RoomClosed>(
      reg, kRoomsOpenPerShuffle, {}, Utils::Rnd::ExcludePack<Cmp::Crypt::RoomStart, Cmp::Crypt::RoomEnd>{}, 0 );

  auto player_pos_cmp = Utils::Player::get_position( reg );
  // reset rooms/passages
  m_crypt.remove_lava_pit_open_rooms( player_pos_cmp );
  m_crypt.remove_lever_open_rooms( player_pos_cmp );
  m_crypt.remove_chest_open_rooms( player_pos_cmp );
  m_crypt.close_open_rooms( player_pos_cmp );
  m_crypt.fill_closed_rooms( uuid_map );
  m_passages.reset_passages();

  // open new rooms/passages
  m_crypt.open_selected_rooms( selected_rooms );
  m_crypt.empty_open_rooms( uuid_map );
  m_crypt.create_room_borders( uuid_map );
  m_crypt.add_lava_pit_open_rooms( player_pos_cmp );

  // try to open passages for the occupied room: only do start room if player is currently there
  auto [start_room_entt, start_room_cmp] = m_crypt.get_crypt_room_start();
  if ( Utils::Player::get_position( reg ).findIntersection( start_room_cmp ) ) { m_passages.connect_start_to_open_rooms( start_room_entt ); }
  else { m_passages.connect_occupied_to_open_rooms(); }

  m_crypt.add_chest_to_open_rooms( player_pos_cmp );
  m_passages.open_passages();
  m_crypt.add_lever_to_open_rooms();

  m_crypt.sound_bank().get_effect( "crypt_room_shuffle" ).play();
  Utils::Crypt::reset_crypt_shuffle_timer( reg );
}

void ClassicCryptVariant::on_lever_activated()
{
  if ( m_maze_unlocked ) return;
  ++m_enabled_levers;

  // check if we have activated enough levers to access the maze objective
  if ( m_enabled_levers >= kLeversToUnlock )
  {
    m_maze_unlocked = true;
    unlock_objective();
    Utils::Crypt::reset_crypt_shuffle_timer( m_crypt.reg() );
  }
  else { shuffle(); }
}

void ClassicCryptVariant::unlock_exit()
{
  // if we unlocked the maze by picking up the preserved cat, then cancel the timer
  m_maze_unlocked = true;
  CryptVariant::unlock_exit();
}

} // namespace Game::Sys::Crypt
