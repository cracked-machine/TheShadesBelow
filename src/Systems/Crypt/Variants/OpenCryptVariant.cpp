#include <Systems/Crypt/Variants/OpenCryptVariant.hpp>

#include <Audio/SoundBank.hpp>
#include <Components/Persistent/CryptNpcSpawnCount.hpp>
#include <Systems/CryptSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Utils/Player.hpp>

#include <algorithm>
#include <array>

namespace Game::Sys::Crypt
{

LayoutParams OpenCryptVariant::layout_params() const
{
  return LayoutParams{ .room_count = 28, .min_room_w = 3, .max_room_w = 6, .min_room_h = 3, .max_room_h = 6 };
}

PassageConfig OpenCryptVariant::passage_config() const
{
  return PassageConfig{ .walk_type = ProcGen::WalkingType::DRUNK, .sparse_connections = false, .spike_traps = true };
}

void OpenCryptVariant::on_first_enter()
{
  auto &reg = m_crypt.reg();
  auto uuid_map = m_crypt.build_non_obstacle_uuid_map();
  auto player_pos_cmp = Utils::Player::get_position( reg );

  m_crypt.open_all_rooms();
  m_crypt.empty_open_rooms( uuid_map );
  m_crypt.create_room_borders( uuid_map );
  m_crypt.add_lava_pit_open_rooms( player_pos_cmp );

  // the start room's own passages are carved now; the cached room-to-room network spawns over the next frames
  m_passages.connect_start_to_open_rooms( m_crypt.get_crypt_room_start().first );
  m_passages.open_passages();
  m_passages.connect_all_rooms();

  static const std::array<Sys::NpcKey, 1> kRoamingNpcs = { Sys::NpcKey( "npc.skeleton" ) };
  const int npc_count = Sys::PersistSystem::get<Cmp::Persist::CryptNpcSpawnCount>( reg ).get_value();
  m_crypt.spawn_npc_in_open_rooms( kRoamingNpcs, std::max( 1, npc_count / 2 ) );

  m_crypt.sound_bank().get_effect( "crypt_room_shuffle" ).play();
}

void OpenCryptVariant::unlock_exit()
{
  m_crypt.remove_lava_pit_open_rooms( Utils::Player::get_position( m_crypt.reg() ) );
  spawn_exit_npcs();
}

void OpenCryptVariant::spawn_exit_npcs()
{
  static const std::array<Sys::NpcKey, 2> kExitNpcs = { Sys::NpcKey( "npc.priest" ), Sys::NpcKey( "npc.ghost" ) };
  m_crypt.spawn_npc_in_open_rooms( kExitNpcs, Sys::PersistSystem::get<Cmp::Persist::CryptNpcSpawnCount>( m_crypt.reg() ).get_value() );
}

} // namespace Game::Sys::Crypt
