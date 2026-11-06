#ifndef SRC_CMPS_ALTAR_TALISMANCOUNT_HPP__
#define SRC_CMPS_ALTAR_TALISMANCOUNT_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <cstddef>
#include <iterator>
#include <map>
#include <optional>
#include <vector>

namespace Game::Cmp::Altar
{

class TalismanCount
{
public:
  TalismanCount( const std::vector<Sys::ItemKey> &keys )
  {
    m_data.clear();
    for ( auto key : keys )
    {
      m_data.emplace( key, false );
    }
  }

  [[nodiscard]] size_t count() const
  {
    size_t count = 0;
    for ( const auto &[key, val] : m_data )
    {
      if ( val ) count++;
    }
    return count;
  }

  bool activate( const Sys::ItemKey &key )
  {
    auto it = m_data.find( key );
    if ( it == m_data.end() ) return false;
    it->second = true;
    return true;
  }

  [[nodiscard]] bool activated( const Sys::ItemKey &key ) const
  {
    auto it = m_data.find( key );
    if ( it == m_data.end() ) return false;
    return it->second;
  }

  //! @brief Index of a talisman: its position in key order (e.g. item.talisman1 -> 0 ... item.talisman4 -> 3).
  //! @details Used as the slot for both its altar candle (particle_offsets) and its drawn sprite (talisman_offsets).
  //! @param key
  //! @return The index, or std::nullopt if the key is not one of this altar's talismans.
  [[nodiscard]] std::optional<std::size_t> index( const Sys::ItemKey &key ) const
  {
    auto it = m_data.find( key );
    if ( it == m_data.end() ) return std::nullopt;
    return static_cast<std::size_t>( std::distance( m_data.begin(), it ) );
  }

  [[nodiscard]] size_t size() const { return m_data.size(); }

  //! @brief Iterate the talismans in index order as (key, activated) pairs.
  [[nodiscard]] auto begin() const { return m_data.cbegin(); }
  [[nodiscard]] auto end() const { return m_data.cend(); }

private:
  std::map<Sys::ItemKey, bool> m_data;
};

} // namespace Game::Cmp::Altar

#endif // SRC_CMPS_ALTAR_TALISMANCOUNT_HPP__