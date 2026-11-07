#include <Audio/EffectsItem.hpp>
#include <spdlog/spdlog.h>
namespace Game::Audio
{

EffectsData::EffectsData( const std::filesystem::path &filepath )
    : buffer(
          [&filepath]()
          {
            if ( not std::filesystem::exists( filepath ) )
            {
              SPDLOG_CRITICAL( "Sound effect file {} does not exist!", filepath.string() );
              throw std::runtime_error( "Sound effect file not found: " + filepath.string() );
            }
            return std::make_unique<sf::SoundBuffer>( filepath );
          }() ),
      control( *buffer )
{
}

EffectsData::EffectsData( EffectsData &&other ) noexcept
    : buffer( std::move( other.buffer ) ),
      control( *buffer ),
      paused_volume( other.paused_volume )
{
}

auto EffectsData::operator=( EffectsData &&other ) noexcept -> EffectsData &
{
  buffer = std::move( other.buffer );

  // rebind control to the new buffer location
  control.setBuffer( *buffer );
  paused_volume = other.paused_volume;
  // return the derefenced buffer object
  return *this;
}

} // namespace Game::Audio