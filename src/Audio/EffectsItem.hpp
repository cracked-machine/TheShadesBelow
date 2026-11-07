#ifndef SRC_AUDIO_EFFECTSITEM_HPP__
#define SRC_AUDIO_EFFECTSITEM_HPP__

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <memory>

namespace Game::Audio
{

//! @brief Owns the loaded sound buffer and playback control for a single registered sound effect.
struct EffectsData
{
  //! @brief Sound buffer for the effect.
  //! @note Use unique_ptr for stable address.
  std::unique_ptr<sf::SoundBuffer> buffer;

  //! @brief Sound control for the effect.
  sf::Sound control;

  //! @brief The effect's own volume, captured around a pause/resume fade so it can be restored afterwards.
  float paused_volume = 100.f;

  //! @brief Construct a new Effects Data object, loading its buffer from `filepath`.
  //! @param filepath Path to the sound effect audio file.
  EffectsData( const std::filesystem::path &filepath );

  //! @brief Construct a new Effects Data object.
  //! @note Need to define move constructor since we have unique_ptr.
  //! @param other
  EffectsData( EffectsData &&other ) noexcept;

  //! @brief Move assignment operator.
  //! @note Need to define move assignment operator since we have unique_ptr.
  //! @param other
  //! @return EffectsData&
  auto operator=( EffectsData &&other ) noexcept -> EffectsData &;

  //! @brief Deleted copy constructor (buffer ownership is move-only).
  EffectsData( const EffectsData & ) = delete;
  //! @brief Deleted copy assignment operator (buffer ownership is move-only).
  auto operator=( const EffectsData & ) -> EffectsData & = delete;
};

} // namespace Game::Audio

#endif // SRC_AUDIO_EFFECTSITEM_HPP__
