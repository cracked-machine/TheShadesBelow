#ifndef SRC_COMPONENTS_PLAYER_CADAVER_HPP__
#define SRC_COMPONENTS_PLAYER_CADAVER_HPP__

#include <Systems/Stores/StoreKey.hpp>
namespace Game::Cmp::Player
{

class Cadaver
{
public:
  Cadaver( const std::vector<Sys::SpriteKey> &sprites )
  {

    for ( auto sprite : sprites )
    {
      m_cadaver.emplace( sprite, false );
    }
  }

  void set( const Sys::SpriteKey &sprite )
  {
    auto it = m_cadaver.find( sprite );
    if ( it != m_cadaver.end() ) { it->second = true; }
  }

  [[nodiscard]] bool is_set( const Sys::SpriteKey &sprite ) const
  {
    auto it = m_cadaver.find( sprite );
    if ( it != m_cadaver.end() ) { return it->second; }
    return false;
  }

  [[nodiscard]] size_t count() const
  {
    size_t count = 0;
    for ( const auto &[key, value] : m_cadaver )
    {
      if ( value ) count++;
    }
    return count;
  }

  [[nodiscard]] std::vector<Sys::SpriteKey> keys( bool remaining_only = false ) const
  {
    std::vector<Sys::SpriteKey> keys;
    keys.reserve( m_cadaver.size() );
    for ( const auto &[key, value] : m_cadaver )
    {
      if ( remaining_only and is_set( key ) ) continue;
      keys.push_back( key );
    }
    return keys;
  }

  auto begin() { return m_cadaver.begin(); }
  auto end() { return m_cadaver.end(); }
  auto size() { return m_cadaver.size(); }
  void reset()
  {
    for ( auto &[key, value] : m_cadaver )
    {
      value = false;
    }
  }

private:
  std::map<Sys::SpriteKey, bool> m_cadaver;
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_CADAVER_HPP__
