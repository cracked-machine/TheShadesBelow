
#include <Audio/SoundBank.hpp>
#include <SFML/Audio/PlaybackDevice.hpp>
#include <SFML/Audio/SoundChannel.hpp>
#include <Utils/JsonDeserializer.hpp>
#include <spdlog/spdlog.h>

namespace Game::Audio
{

SoundBank::SoundBank() { sounds.emplace( "fallback", EffectsData{ "res/audio/fallback.wav" } ); }

void SoundBank::init( const std::filesystem::path &json_file_path )
{
  using Utils::JsonDeserializer;
  SPDLOG_DEBUG( "Initializing SoundBank from {}...", json_file_path.string() );
  const auto json = JsonDeserializer::load_json_file( json_file_path );

  // Initialize sound effects
  for ( const auto &entry : JsonDeserializer::get_array( json, "sounds" ) )
  {
    const auto name = JsonDeserializer::get_string( entry, "name" );
    if ( not sounds.emplace( name, EffectsData{ JsonDeserializer::get_string( entry, "path" ) } ).second )
      throw std::runtime_error( "Duplicate sound effect name in " + json_file_path.string() + ": " + name );
  }

  // Initialize music
  for ( const auto &entry : JsonDeserializer::get_array( json, "music" ) )
  {
    const auto name = JsonDeserializer::get_string( entry, "name" );
    if ( not music.emplace( name, MusicData{ JsonDeserializer::get_string( entry, "path" ) } ).second )
      throw std::runtime_error( "Duplicate music name in " + json_file_path.string() + ": " + name );
  }
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

void SoundBank::stop_all()
{
  for ( auto &[name, music_data] : music )
  {
    music_data.control.stop();
  }
  for ( auto &[name, music_data] : music )
  {
    music_data.control.stop();
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