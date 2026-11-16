#ifndef SRC_COMPONENTS_CURRENTSCENE_HPP__
#define SRC_COMPONENTS_CURRENTSCENE_HPP__

namespace Game::Cmp::SceneSettings
{

//! @brief Identifies which scene a CurrentScene component belongs to.
enum class SceneId {
  //! @brief The graveyard scene
  GRAVEYARD,
  //! @brief The shop scene
  SHOP,
  //! @brief The healing spring scene
  HEALING_SPRING,
  //! @brief The barber surgeon scene
  BARBER_SURGEON,
  //! @brief The upper floor of the ruin scene
  RUIN_UPPER_FLOOR,
  //! @brief The lower floor of the ruin scene
  RUIN_LOWER_FLOOR,
  //! @brief The crypt scene
  CRYPT,
  //! @brief The title scene
  TITLE,
  //! @brief The paused menu overlay scene
  PAUSED_MENU,
  //! @brief The settings menu scene
  SETTINGS_MENU,
  //! @brief The game over scene
  GAME_OVER,
  //! @brief The level complete scene
  LEVEL_COMPLETE
};

//! @brief Identifies which scene an entity/context belongs to.
struct CurrentScene
{
  //! @brief The identifier of the current scene.
  SceneId id;
};

} // namespace Game::Cmp::SceneSettings

#endif // SRC_COMPONENTS_CURRENTSCENE_HPP__
