#ifndef SRC_COMPONENTS_PERSISTENT_NPCDAMAGETEXTOFFSET_HPP__
#define SRC_COMPONENTS_PERSISTENT_NPCDAMAGETEXTOFFSET_HPP__

#include <Components/Persistent/BasePersistent.hpp>

namespace Game::Cmp::Persist
{

//! @brief Vertical offset (px) of an NPC's floating damage number (Cmp::DamagePopup) from the top of its sprite.
//! Positive moves the number down over the sprite, negative moves it up above the sprite. See Utils::Player::apply_damage().
struct NpcDamageTextOffset : public BasePersistent<float>
{
  //! @brief Construct with an initial offset and optional min/max bounds.
  //! @param value initial offset
  //! @param min_value minimum allowed offset
  //! @param max_value maximum allowed offset
  NpcDamageTextOffset( float value = 0.f, float min_value = 0.f, float max_value = 0.f )
      : BasePersistent<float>( value, min_value, max_value )
  {
  }
  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "NpcDamageTextOffset"
  [[nodiscard]] std::string class_name() const override { return "NpcDamageTextOffset"; }
  //! @brief Get the human-readable detail/description text (unset; see persist.json).
  //! @return std::string empty detail string
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_NPCDAMAGETEXTOFFSET_HPP__
