#ifndef SRC_AUDIO_SOUNDBANK_HPP__
#define SRC_AUDIO_SOUNDBANK_HPP__

#include <Audio/EffectsItem.hpp>
#include <Audio/MusicItem.hpp>
#include <filesystem>
#include <unordered_map>

namespace Game::Audio
{

//! @brief Central registry and pool of sound effects and music tracks, keyed by name.
class SoundBank
{
public:
  //! @brief Construct a new Sound Bank. Call init() before resolving any effect/music name.
  SoundBank();

  //! @brief Registers every sound effect and music track listed in `json_file_path` by name, loading their buffers.
  //! Must be called once before get_effect()/get_music() can resolve any name.
  //! @param json_file_path Path to the audio JSON file (see res/json/audio_schema.json).
  //! @throws std::runtime_error if the file is malformed, a name is duplicated, or an audio file is missing.
  void init( const std::filesystem::path &json_file_path = "res/json/audio.json" );

  //! @brief Applies `volume` to every registered sound effect.
  //! @param volume
  void update_effects_volume( float volume );

  //! @brief Applies `volume` to every registered music track.
  //! @param volume
  void update_music_volume( float volume );

  void stop_all();

  //! @brief Pauses every sound effect that is currently playing.
  void pause_effects();

  //! @brief Resumes every sound effect that was paused (e.g. by pause_effects()).
  void resume_effects();

  //! @brief Returns a reference to the pooled sf::Sound instance for `name`.
  //! Bind with `auto &`/`const auto &` - copying into a local (`auto`) creates a
  //! disconnected sf::Sound with its own OpenAL source, so calls like .play()/.stop()
  //! on the copy will not affect the managed instance.
  //! @param name
  sf::Sound &get_effect( const std::string &name );

  //! @brief Returns a reference to the pooled sf::Music instance for `name`.
  //! Bind with `auto &`/`const auto &` - copying into a local (`auto`) creates a
  //! disconnected sf::Music with its own OpenAL source, so calls like .play()/.stop()
  //! on the copy will not affect the managed instance.
  //! @param name
  sf::Music &get_music( const std::string &name );

private:
  //! @brief Synthesizes a sine wave tone into an in-memory sf::SoundBuffer.
  //! @param frequency
  //! @param duration
  //! @param sample_rate
  static sf::SoundBuffer generate_tone( float frequency = 440.0f, float duration = 0.5f, unsigned int sample_rate = 44100 );

  //! @brief Registered sound effects, keyed by the name passed to get_effect().
  std::unordered_map<std::string, EffectsData> sounds;

  //! @brief Registered music tracks, keyed by the name passed to get_music().
  std::unordered_map<std::string, MusicData> music;
};

} // namespace Game::Audio

#endif // SRC_AUDIO_SOUNDBANK_HPP__
