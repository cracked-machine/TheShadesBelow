#ifndef SRC_SCENECONTROL_EVENTS_SCENEMANAGEREVENT_HPP__
#define SRC_SCENECONTROL_EVENTS_SCENEMANAGEREVENT_HPP__
namespace Game::Events
{

//! @brief Requests that the SceneManager switch which scene is active, e.g. entering/exiting a level area or menu.
struct SceneManagerEvent
{
  //! @brief The scene transition being requested.
  enum class Type
  {
    //! @brief Close the application entirely.
    EXIT_GAME,
    //! @brief Open the settings menu scene.
    SETTINGS_MENU,
    //! @brief Close the settings menu scene, returning to the previous scene.
    EXIT_SETTINGS_MENU,
    //! @brief Begin a new game from the title scene.
    START_GAME,
    //! @brief Quit the current game, returning to the title scene.
    QUIT_GAME,
    //! @brief Enter the game-over scene, e.g. after the player dies.
    GAME_OVER,
    //! @brief Enter the level-complete scene.
    LEVEL_COMPLETE,
    //! @brief Enter the crypt scene.
    ENTER_CRYPT,
    //! @brief Exit the crypt scene, returning to the graveyard scene.
    EXIT_CRYPT,
    //! @brief Enter the healing spring scene.
    ENTER_SACREDSPRING,
    //! @brief Exit the healing spring scene, returning to the graveyard scene.
    EXIT_SACREDSPRING,
    //! @brief Enter the lower floor of the ruin scene.
    ENTER_RUIN_LOWER,
    //! @brief Enter the upper floor of the ruin scene.
    ENTER_RUIN_UPPER,
    //! @brief Exit the upper floor of the ruin scene, returning to the lower floor.
    EXIT_RUIN_UPPER,
    //! @brief Exit the ruin scene, returning to the graveyard scene.
    EXIT_RUIN,
    //! @brief Enter the shop scene.
    ENTER_SHOP,
    //! @brief Exit the shop scene, returning to the graveyard scene.
    EXIT_SHOP,
    //! @brief Return to the title scene.
    RETURN_TO_TITLE
  };

  //! @brief The scene transition requested by this event.
  Type m_type;
  //! @brief Construct a new SceneManagerEvent object
  //! @param type The scene transition being requested.
  explicit SceneManagerEvent( Type type )
      : m_type( type )
  {
  }
};

} // namespace Game::Events
#endif // SRC_SCENECONTROL_EVENTS_SCENEMANAGEREVENT_HPP__
