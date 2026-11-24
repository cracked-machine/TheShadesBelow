#ifndef SRC_CMPS_CART_BARREL_HPP__
#define SRC_CMPS_CART_BARREL_HPP__

#include <Systems/Stores/StoreKey.hpp>

#include <array>
#include <string_view>
#include <utility>

namespace Game::Cmp::Cart
{

//! @brief One barrel on the cart. Each flag is one cadaver part slot in res/ui/barrel_slot.json, true once filled.
struct Barrel
{
  bool skull{ false };
  bool torso{ false };
  bool leftarm{ false };
  bool rightarm{ false };
  bool leftleg{ false };
  bool rightleg{ false };

  //! @brief Find the slot flag for a cadaver part sprite.
  //! @param sprite The part's sprite, e.g. "sprite.grave.cadaver.skull".
  //! @return Pointer to the flag, or nullptr if `sprite` is not a cadaver part.
  [[nodiscard]] bool *part( const Sys::SpriteKey &sprite ) { return find_part( *this, sprite ); }

  //! @brief Find the slot flag for a cadaver part sprite.
  //! @param sprite The part's sprite, e.g. "sprite.grave.cadaver.skull".
  //! @return Pointer to the flag, or nullptr if `sprite` is not a cadaver part.
  [[nodiscard]] const bool *part( const Sys::SpriteKey &sprite ) const { return find_part( *this, sprite ); }

private:
  //! @brief Shared implementation of part(), keeping the constness of `self`.
  template <typename Self>
  [[nodiscard]] static auto find_part( Self &self, const Sys::SpriteKey &sprite ) -> decltype( &self.skull )
  {
    static constexpr std::array<std::pair<std::string_view, bool Barrel::*>, 6> kParts{ {
        { "sprite.grave.cadaver.skull", &Barrel::skull },
        { "sprite.grave.cadaver.torso", &Barrel::torso },
        { "sprite.grave.cadaver.leftarm", &Barrel::leftarm },
        { "sprite.grave.cadaver.rightarm", &Barrel::rightarm },
        { "sprite.grave.cadaver.leftleg", &Barrel::leftleg },
        { "sprite.grave.cadaver.rightleg", &Barrel::rightleg },
    } };
    for ( const auto &[name, member] : kParts )
    {
      if ( sprite.str() == name ) { return &( self.*member ); }
    }
    return nullptr;
  }
};

} // namespace Game::Cmp::Cart

#endif // SRC_CMPS_CART_BARREL_HPP__
