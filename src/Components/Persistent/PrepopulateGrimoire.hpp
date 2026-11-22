#ifndef SRC_COMPONENTS_PERSISTENT_PREPOPULATEGRIMOIRE_HPP__
#define SRC_COMPONENTS_PERSISTENT_PREPOPULATEGRIMOIRE_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Whether the grimoire hover overlay (the grimoire subpage of the plant or NPC under the mouse) is drawn beside
//! the mouse cursor instead of at its fixed position in res/ui/grimoire_plant_hover.json or res/ui/grimoire_npc_hover.json.
//! @note Read every frame by Sys::GrimoireOverlaySystem::update.
class PrepopulateGrimoire : public BasePersistent<bool>
{
public:
  //! @brief Construct the grimoire hover follows mouse setting.
  //! @param value true to draw the grimoire hover overlay beside the mouse cursor.
  PrepopulateGrimoire( bool value = false )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "PrepopulateGrimoire"; }
  //! @brief Returns the display description for this setting (unset; see persist.json).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_PREPOPULATEGRIMOIRE_HPP__
