#include <Systems/Crypt/Variants/CryptVariant.hpp>

#include <Audio/SoundBank.hpp>
#include <Components/Persistent/CryptNpcSpawnCount.hpp>
#include <Systems/CryptSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Utils/Player.hpp>

#include <array>

namespace Game::Sys::Crypt
{

void CryptVariant::generate( sf::Vector2u map_size_grid )
{
  m_crypt.create_end_room( map_size_grid );
  m_crypt.create_initial_closed_rooms( map_size_grid, layout_params() );
  m_passages.cache_all_room_connections();
  m_crypt.gen_crypt_initial_interior();
}

void CryptVariant::unlock_objective()
{
  auto uuid_map = m_crypt.build_non_obstacle_uuid_map();
  auto player_pos_cmp = Utils::Player::get_position( m_crypt.reg() );

  // reset rooms/passages
  m_crypt.remove_lava_pit_open_rooms( player_pos_cmp );
  m_crypt.remove_lever_open_rooms( player_pos_cmp );
  m_crypt.remove_chest_open_rooms( player_pos_cmp );
  m_crypt.close_open_rooms( player_pos_cmp );
  m_crypt.remove_all_levers();
  m_crypt.fill_closed_rooms( uuid_map );
  m_passages.reset_passages();
  m_crypt.empty_open_rooms( uuid_map );
  m_crypt.create_room_borders( uuid_map );

  // open new rooms/passages
  SPDLOG_DEBUG( "~~~~~~~~~~~ OPENING FINAL PASSAGE ~~~~~~~~~~~~~~~" );
  m_passages.connect_occupied_to_end_room();
  m_passages.open_passages();
  m_crypt.sound_bank().get_effect( "crypt_room_shuffle" ).play();
}

void CryptVariant::unlock_exit()
{
  auto uuid_map = m_crypt.build_non_obstacle_uuid_map();
  auto player_pos_cmp = Utils::Player::get_position( m_crypt.reg() );

  m_crypt.close_open_rooms( player_pos_cmp );
  m_crypt.remove_all_chests();
  m_crypt.remove_all_levers();
  m_crypt.remove_lava_pit_open_rooms( player_pos_cmp );
  m_crypt.open_all_rooms();
  m_crypt.empty_open_rooms( uuid_map );
  m_crypt.create_room_borders( uuid_map );

  // make sure player can reach exit
  m_passages.connect_start_to_open_rooms( m_crypt.get_crypt_room_start().first );
  m_passages.connect_all_rooms();

  spawn_exit_npcs();
}

void CryptVariant::spawn_exit_npcs()
{
  static const std::array<Sys::NpcKey, 1> kExitNpcs = { Sys::NpcKey( "npc.priest" ) };
  m_crypt.spawn_npc_in_open_rooms( kExitNpcs, Sys::PersistSystem::get<Cmp::Persist::CryptNpcSpawnCount>( m_crypt.reg() ).get_value() );
}

} // namespace Game::Sys::Crypt
