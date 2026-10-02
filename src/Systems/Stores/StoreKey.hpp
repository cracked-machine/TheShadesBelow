#ifndef SRC_SYSTEMS_STORES_STOREKEY_HPP__
#define SRC_SYSTEMS_STORES_STOREKEY_HPP__

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace Game::Sys
{

// Forward declarations only: the store classes are used purely as tags here
class SpriteStore;
class ItemStore;
class NpcStore;

//! @brief Strongly-typed string key identifying an entry in one particular data store.
//! Keys for different stores are distinct types, so e.g. an item key cannot be passed where a
//! sprite key is expected. String literals convert implicitly; runtime strings (JSON, Tiled data,
//! concatenation) must be wrapped explicitly.
//! @tparam Tag The store class this key belongs to.
template <typename Tag>
class StoreKey
{
public:
  //! @brief Construct an empty key.
  StoreKey() = default;

  //! @brief Construct a key from a string literal. Implicit so that literals can be used directly as keys.
  //! @param literal
  StoreKey( const char *literal ) // NOLINT(google-explicit-constructor)
      : m_value( literal )
  {
  }

  //! @brief Construct a key from a runtime string.
  //! @param value
  explicit StoreKey( std::string value )
      : m_value( std::move( value ) )
  {
  }

  //! @brief Get the underlying string.
  [[nodiscard]] const std::string &str() const { return m_value; }

  //! @brief Check whether the key contains the given substring.
  //! @param s
  [[nodiscard]] bool contains( std::string_view s ) const { return m_value.contains( s ); }

  //! @brief Check whether the key is empty.
  [[nodiscard]] bool empty() const { return m_value.empty(); }

  //! @brief Keys of the same store compare and order by their underlying string.
  //! @note A member is enough here (no friend needed): since C++20 the compiler also tries the reversed
  //! operands for == and <=>, so a literal on the left-hand side (`"item.axe" == key`) still converts.
  auto operator<=>( const StoreKey & ) const = default;

private:
  //! @brief The underlying key string
  std::string m_value;
};

//! @brief Key into Sys::SpriteStore, e.g. "sprite.graveyard.pots"
using SpriteKey = StoreKey<SpriteStore>;
//! @brief Key into Sys::ItemStore, e.g. "item.axe"
using ItemKey = StoreKey<ItemStore>;
//! @brief Key into Sys::NpcStore, e.g. "npc.shadowhand"
using NpcKey = StoreKey<NpcStore>;

//! @brief Lets fmt/spdlog format a StoreKey as its underlying string (found via ADL).
template <typename Tag>
const std::string &format_as( const StoreKey<Tag> &key )
{
  return key.str();
}

} // namespace Game::Sys

//! @brief Hash support so StoreKey can be used in unordered containers.
template <typename Tag>
struct std::hash<Game::Sys::StoreKey<Tag>>
{
  std::size_t operator()( const Game::Sys::StoreKey<Tag> &key ) const noexcept { return std::hash<std::string>{}( key.str() ); }
};

#endif // SRC_SYSTEMS_STORES_STOREKEY_HPP__
