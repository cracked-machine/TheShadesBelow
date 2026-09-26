
#include <Audio/SoundBank.hpp>
#include <SFML/Audio/PlaybackDevice.hpp>
#include <SFML/Audio/SoundChannel.hpp>
#include <spdlog/spdlog.h>

namespace Game::Audio
{

SoundBank::SoundBank() { sounds.emplace( "fallback", EffectsData{ "res/audio/fallback.wav" } ); }

void SoundBank::init()
{
  SPDLOG_DEBUG( "Initializing SoundBank..." );
  // Initialize sound effects
  sounds.emplace( "wormhole_jump", EffectsData{ "res/audio/wormhole_jump.wav" } );
  sounds.emplace( "shrine_lighting", EffectsData{ "res/audio/shrine_lighting.wav" } );

  sounds.emplace( "secret", EffectsData{ "res/audio/secret.wav" } );
  sounds.emplace( "drop_relic", EffectsData{ "res/audio/drop_relic.wav" } );
  sounds.emplace( "spawn_ghost", EffectsData{ "res/audio/spawn_ghost.wav" } );
  sounds.emplace( "skeleton_moving", EffectsData{ "res/audio/skeleton_moving.mp3" } );
  sounds.emplace( "damage_player", EffectsData{ "res/audio/damage_player.wav" } );
  sounds.emplace( "cobweb_movement", EffectsData{ "res/audio/cobweb_movement.mp3" } );
  sounds.emplace( "spider_moving", EffectsData{ "res/audio/spider_moving.mp3" } );
  sounds.emplace( "shotgun_rack", EffectsData{ "res/audio/shotgun_rack.mp3" } );
  sounds.emplace( "shotgun_fire", EffectsData{ "res/audio/shotgun_fire.mp3" } );
  sounds.emplace( "rattling_bones", EffectsData{ "res/audio/rattling_bones.mp3" } );

  sounds.emplace( "footstep_grass", EffectsData{ "res/audio/footstep_grass.mp3" } );
  sounds.emplace( "footstep_woodfloor", EffectsData{ "res/audio/footstep_woodfloor.mp3" } );
  sounds.emplace( "footstep_mud", EffectsData{ "res/audio/footstep_mud.mp3" } );
  sounds.emplace( "footstep_stone", EffectsData{ "res/audio/footstep_stone.mp3" } );
  sounds.emplace( "bomb_fuse", EffectsData{ "res/audio/fuse.wav" } );
  sounds.emplace( "bomb_detonate", EffectsData{ "res/audio/detonate.wav" } );
  sounds.emplace( "drop_loot", EffectsData{ "res/audio/drop_loot.wav" } );
  sounds.emplace( "get_loot", EffectsData{ "res/audio/get_loot.wav" } );
  sounds.emplace( "get_key", EffectsData{ "res/audio/get_key.wav" } );
  sounds.emplace( "break_pot", EffectsData{ "res/audio/break_pot.wav" } );
  sounds.emplace( "hit_pot", EffectsData{ "res/audio/hit_pot.mp3" } );
  sounds.emplace( "hit_grave", EffectsData{ "res/audio/hit_grave.mp3" } );
  sounds.emplace( "digging_earth", EffectsData{ "res/audio/digging_plant.mp3" } );
  sounds.emplace( "chopping", EffectsData{ "res/audio/chopping.mp3" } );
  sounds.emplace( "chopping_final", EffectsData{ "res/audio/chopping_final.mp3" } );
  sounds.emplace( "axe_whip", EffectsData{ "res/audio/axe_whip.wav" } );
  sounds.emplace( "skele_death", EffectsData{ "res/audio/skele_death.wav" } );
  sounds.emplace( "eating", EffectsData{ "res/audio/eating.mp3" } );
  sounds.emplace( "burning", EffectsData{ "res/audio/burning.mp3" } );
  sounds.emplace( "equip_inventory", EffectsData{ "res/audio/equip_inventory.mp3" } );
  sounds.emplace( "drop_inventory", EffectsData{ "res/audio/drop_inventory.mp3" } );
  sounds.emplace( "heartbeat", EffectsData{ "res/audio/heartbeat.mp3" } );

  sounds.emplace( "draw_bow", EffectsData{ "res/audio/draw_bow.mp3" } );
  sounds.emplace( "release_bow", EffectsData{ "res/audio/release_bow.mp3" } );

  sounds.emplace( "arrow_hit1", EffectsData{ "res/audio/arrow_hit1.mp3" } );
  sounds.emplace( "arrow_hit2", EffectsData{ "res/audio/arrow_hit2.mp3" } );
  sounds.emplace( "arrow_hit3", EffectsData{ "res/audio/arrow_hit3.mp3" } );
  sounds.emplace( "arrow_hit4", EffectsData{ "res/audio/arrow_hit4.mp3" } );

  sounds.emplace( "pickaxe1", EffectsData{ "res/audio/pickaxe1.wav" } );
  sounds.emplace( "pickaxe2", EffectsData{ "res/audio/pickaxe2.wav" } );
  sounds.emplace( "pickaxe3", EffectsData{ "res/audio/pickaxe3.wav" } );
  sounds.emplace( "pickaxe4", EffectsData{ "res/audio/pickaxe4.wav" } );
  sounds.emplace( "pickaxe5", EffectsData{ "res/audio/pickaxe5.wav" } );
  sounds.emplace( "pickaxe6", EffectsData{ "res/audio/pickaxe6.wav" } );
  sounds.emplace( "pickaxe_final", EffectsData{ "res/audio/pickaxe_final.wav" } );

  sounds.emplace( "crypt_open", EffectsData{ "res/audio/crypt_open2.wav" } );
  sounds.emplace( "crypt_locked", EffectsData{ "res/audio/crypt_locked.wav" } );
  sounds.emplace( "crypt_room_shuffle", EffectsData{ "res/audio/crypt_room_shuffle.wav" } );
  sounds.emplace( "crypt_lever_open", EffectsData{ "res/audio/crypt_lever_open.mp3" } );
  sounds.emplace( "crypt_chest_open", EffectsData{ "res/audio/crypt_chest_open.mp3" } );

  sounds.emplace( "player_blood_splat", EffectsData{ "res/audio/player_blood_splat.mp3" } );
  sounds.emplace( "bubbling_lava", EffectsData{ "res/audio/lava.mp3" } );
  sounds.emplace( "spike_trap", EffectsData{ "res/audio/spike_trap.wav" } );
  sounds.emplace( "banging_smashing_sounds", EffectsData{ "res/audio/BangingSmashingSounds.mp3" } );
  sounds.emplace( "witch_scream", EffectsData{ "res/audio/witch_scream.mp3" } );
  sounds.emplace( "lightning_strike", EffectsData{ "res/audio/lightning_strike.mp3" } );

  sounds.emplace( "emit_shockwave_low", EffectsData{ "res/audio/shockwave3.mp3" } );
  sounds.emplace( "emit_shockwave_high", EffectsData{ "res/audio/shockwave5.mp3" } );

  sounds.emplace( "witches_jar_sacrifice", EffectsData{ "res/audio/witches_jar_sacrifice.mp3" } );
  sounds.emplace( "player_respawn", EffectsData{ "res/audio/player_respawn.mp3" } );
  sounds.emplace( "crypt_altar_activate", EffectsData{ "res/audio/crypt_altar_activate.mp3" } );

  // Initialize music (can't use initializer list because sf::Music is move-only)
  music.emplace( "title_music", MusicData{ "res/audio/EerieScifi.mp3" } );
  music.emplace( "graveyard_music", MusicData{ "res/audio/SadWindyOrgan.mp3" } );
  music.emplace( "ruin_creaking_rope", MusicData{ "res/audio/rope.mp3" } );
  music.emplace( "ruin_music", MusicData{ "res/audio/PhaseTones.mp3" } );
  music.emplace( "healing_spring", MusicData{ "res/audio/HealingSpring.mp3" } );
}

void SoundBank::update_effects_volume( float volume )
{
  for ( auto &[name, sound_data] : sounds )
  {
    sound_data.control.setVolume( volume );
  }
}

void SoundBank::update_music_volume( float volume )
{
  for ( auto &[name, music_data] : music )
  {
    music_data.control.setVolume( volume );
  }
}

sf::Sound &SoundBank::get_effect( const std::string &name )
{
  if ( sounds.find( name ) != sounds.end() ) { return sounds.at( name ).control; }
  SPDLOG_CRITICAL( "Audio effect {} not found!", name );
  throw std::runtime_error( "Audio effect not found: " + name );
}

sf::Music &SoundBank::get_music( const std::string &name )
{
  if ( music.find( name ) != music.end() ) { return music.at( name ).control; }
  SPDLOG_CRITICAL( "Music {} not found!", name );
  throw std::runtime_error( "Music not found: " + name );
}

// Generate a simple tone
sf::SoundBuffer SoundBank::generate_tone( float frequency, float duration, unsigned int sample_rate )
{
  unsigned int sample_count = static_cast<unsigned int>( duration * sample_rate );
  std::vector<int16_t> samples( sample_count );

  const float amplitude = 30000.0f; // Max amplitude for 16-bit audio
  const float increment = frequency / sample_rate;
  float x = 0.0f;

  for ( unsigned int i = 0; i < sample_count; ++i )
  {
    samples[i] = static_cast<int16_t>( amplitude * std::sin( x * 2.0f * 3.14159f ) );
    x += increment;
  }

  sf::SoundBuffer buffer;
  if ( buffer.loadFromSamples( samples.data(), sample_count, 1, sample_rate, { sf::SoundChannel::Mono } ) ) { return buffer; }
  SPDLOG_CRITICAL( "Failed to generate tone" );
  throw std::runtime_error( "Failed to generate tone" );
}

} // namespace Game::Audio