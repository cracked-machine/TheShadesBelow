#ifndef SRC_SYSTEMS_RENDER_LEAVEGRAVEYARDOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_LEAVEGRAVEYARDOVERLAYSYSTEM_HPP__

#include <Components/SceneSettings/Paused.hpp>
#include <Events/OverlayEvent.hpp>
#include <Systems/Render/BaseOverlaySystem.hpp>

#include <SFML/System/Time.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace Game::Sys
{

class Store;

//! @brief The "Do you want to leave the grave yard?" Yes/No prompt, opened by activating the cart horse (see
//! CartHorseSystem). It is a modal overlay: gameplay is paused while the scene's Cmp::SceneSettings::Paused reason is
//! LEAVE_PROMPT. OverlayArbitrationSystem opens it on Events::OverlayEvent LEAVE_PROMPT_OPEN; this system handles the
//! LEAVE_PREV/LEAVE_NEXT/LEAVE_CONFIRM/LEAVE_YES/LEAVE_NO actions. No closes the prompt; Yes completes the level
//! (CartHorseSystem::leave_level).
class LeaveGraveyardOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new Leave Graveyard Overlay System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  //! @param sys The system store, to look up CartHorseSystem.
  LeaveGraveyardOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank, Store &sys );

  //! @brief True while the prompt is open (Cmp::SceneSettings::Paused::Reason::LEAVE_PROMPT).
  [[nodiscard]] bool is_open();

  //! @brief Open the prompt with "No" selected: cancels any bow draw and suspends looping sound effects. No-op if the
  //! scene is already paused for any reason.
  void open();

  //! @brief Close the prompt and resume looping sound effects. No-op if it is not open.
  void close();

  //! @brief Draw the prompt, if open.
  //! @param dt
  void update( sf::Time dt ) override;

private:
  //! @brief Handle the prompt's own overlay actions while it is open.
  //! @param ev
  void on_overlay_event( const Events::OverlayEvent &ev );

  //! @brief The scene's pause setting.
  //! @return Pointer to the component, or nullptr if the scene has none.
  Cmp::SceneSettings::Paused *find_paused();

  //! @brief The prompt rows, in display order.
  static constexpr std::array<std::string_view, 2> kRows{ "Yes", "No" };
  //! @brief Index of "Yes" in kRows.
  static constexpr std::size_t kYesRow{ 0 };
  //! @brief Index of "No" in kRows: the default selection, so a stray confirm never leaves the level.
  static constexpr std::size_t kNoRow{ 1 };

  //! @brief The system store, to look up CartHorseSystem.
  Store &m_sys;

  //! @brief The selected row of kRows.
  std::size_t m_selected{ kNoRow };
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_LEAVEGRAVEYARDOVERLAYSYSTEM_HPP__
