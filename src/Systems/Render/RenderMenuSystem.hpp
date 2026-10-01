#ifndef SRC_SYSTEMS_RENDER_RENDERMENUSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_RENDERMENUSYSTEM_HPP__

#include <Shaders/TitleScreenShader.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Utils/Constants.hpp>

#include <SFML/System/Time.hpp>

namespace Game::Sprites
{
class TitleScreenShader;
} // namespace Game::Sprites

namespace Game::Sys
{

//! @brief Renders the non-gameplay menu screens: the title screen, settings menu, pause menu, defeat screen and victory screen.
class RenderMenuSystem : public RenderSystem
{
public:
  //! @brief Helper struct for display settings options
  struct DisplaySettings
  {
    //! @brief List of available display resolutions. Initialised in the cpp file.
    static const std::vector<sf::Vector2u> resolutions;

    //! @brief ImGui-compatible getter for display resolutions
    //! @param data Unused; required by the ImGui combo box callback signature.
    //! @param idx Index into `resolutions`.
    //! @param out_text Set to the "WIDTHxHEIGHT" string for `resolutions[idx]`.
    //! @return true if `idx` was valid and `out_text` was set, false otherwise.
    static bool get( [[maybe_unused]] void *data, int idx, const char **out_text )
    {
      if ( idx < 0 || idx >= static_cast<int>( resolutions.size() ) ) return false;
      static std::string str;
      str = std::to_string( resolutions[idx].x ) + "x" + std::to_string( resolutions[idx].y );
      *out_text = str.c_str();
      return true;
    }
  };

  //! @brief Construct a new Render Menu System object
  RenderMenuSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
      : RenderSystem( reg, window, sound_bank )
  {
  }

  //! @brief Destroy the Render Menu System object
  ~RenderMenuSystem();

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

  //! @brief Render the title screen: background shaders plus the title and prompt text.
  void render_title();

  //! @brief Render the ImGui widgets for all persistent/tunable game settings (player, bomb, hazard, loot, NPC, audio, procgen). Handles
  //! a pending display-resolution change by deferring the window recreation until after the current ImGui frame has rendered.
  //! @param dt
  //! @param title_text_dimensions Bounds of the "Settings" title text, used to position the ImGui window below it.
  void render_settings_widgets( sf::Time dt, sf::FloatRect title_text_dimensions );

  //! @brief Render the settings screen: title/hint text plus the settings ImGui widgets.
  //! @param dt
  void render_settings( sf::Time dt );

  //! @brief Render the pause menu overlay: title/hint text and music/effects volume sliders.
  //! @param dt
  void render_paused( sf::Time dt );

  //! @brief Render the defeat (game over) screen.
  void render_defeat_screen();

  //! @brief Render the victory (level complete) screen, including cadaver count and wealth stats.
  //! @param allow_continue Whether to show the "press to continue" prompt.
  void render_victory_screen( bool allow_continue );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_RENDERMENUSYSTEM_HPP__
