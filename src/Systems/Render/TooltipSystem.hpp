#ifndef SRC_SYSTEMS_RENDER_TOOLTIPSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_TOOLTIPSYSTEM_HPP__

#include <Events/AddInventoryEvent.hpp>
#include <Events/DismissTooltipEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Systems/Render/RenderSystem.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Game::Sys
{

//! @brief Displays data-driven help messages (res/json/tooltips.json) in a screen-space panel in front of the game.
//! @details Listens to game events without consuming them. Event handlers only record a candidate; candidates are
//! evaluated in update(), which runs in the next scene update, so every other handler for the event has already
//! run and game state is up to date, regardless of the dispatcher's handler order.
class TooltipSystem : public RenderSystem
{
public:
  //! @brief Construct a new Tooltip System object and load res/json/tooltips.json
  //! @param reg
  //! @param window
  //! @param sound_bank
  TooltipSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Process the reset request, evaluate pending candidates and count down the active tooltip.
  //! @note Call from each gameplay scene's do_update(), so the countdown stops while the game is paused.
  //! @param dt the frame delta time
  void update( sf::Time dt );

  //! @brief Draw the active tooltip panel (if any) in screen space. Call after RenderOverlaySystem::render_overlay().
  void render();

  //! @brief Record a candidate for tooltips triggered by "AddInventoryEvent".
  //! @param event
  void on_add_inventory_event( const Events::AddInventoryEvent &event );

  //! @brief Record a candidate for tooltips triggered by "ItemPickedUpEvent".
  //! @param event
  void on_item_picked_up_event( const Events::ItemPickedUpEvent &event );

  //! @brief Hide the active tooltip before its timer expires.
  //! @param event
  void on_dismiss_tooltip_event( [[maybe_unused]] const Events::DismissTooltipEvent &event ) { m_active.reset(); }

private:
  //! @brief How often a tooltip may be shown
  enum class Frequency {
    //! @brief Shown the first time only, tracked in Cmp::Persist::TooltipsSeen
    ONCE,
    //! @brief Shown every time it is triggered
    ALWAYS,
    //! @brief Never shown
    DISABLED
  };

  //! @brief A tooltip definition from res/json/tooltips.json
  struct TooltipDef
  {
    //! @brief unique id, stored in Cmp::Persist::TooltipsSeen
    std::string id;
    //! @brief name of the event that triggers this tooltip, e.g. "AddInventoryEvent" or "ItemPickedUpEvent"
    std::string trigger;
    //! @brief optional item key condition, empty matches any item
    std::string item;
    //! @brief the message to display
    std::string text;
    //! @brief how often the tooltip may be shown
    Frequency frequency{ Frequency::ONCE };
    //! @brief how long the tooltip is displayed for
    sf::Time duration{ sf::seconds( 5.f ) };
  };

  //! @brief An event occurrence recorded by an event handler, waiting to be evaluated by update()
  struct Candidate
  {
    //! @brief name of the event that was received
    std::string trigger;
    //! @brief item key carried by the event, if any
    std::string item;
  };

  //! @brief The tooltip currently being displayed
  struct ActiveTooltip
  {
    //! @brief the message to display
    std::string text;
    //! @brief time left before the tooltip is hidden
    sf::Time remaining;
  };

  //! @brief Panel style from res/json/tooltips.json
  struct PanelStyle
  {
    //! @brief panel centre as a fraction of the display resolution
    sf::Vector2f position{ 0.5f, 0.85f };
    //! @brief font size in pixels
    unsigned int font_size{ 24 };
    //! @brief space between the text and the panel border in pixels
    float padding{ 12.f };
    //! @brief panel background colour
    sf::Color fill_colour{ 0, 0, 0, 180 };
    //! @brief panel border colour
    sf::Color line_colour{ 200, 200, 200, 255 };
    //! @brief text colour
    sf::Color text_colour{ sf::Color::White };
  };

  //! @brief Load the tooltip definitions and panel style. Malformed entries are logged and skipped.
  //! @param path the tooltips json file
  void load_definitions( const std::string &path );

  //! @brief If Cmp::Persist::ResetTutorial is set, forget all seen tooltips and clear the request.
  void check_reset();

  //! @brief Merge the current scene's Cmp::Persist::TooltipsSeen into m_seen and write the union back.
  //! @note Each scene registry holds its own copy loaded at scene init, so a scene lower in the stack can be stale.
  void sync_seen();

  //! @brief Find the first definition that matches the candidate and is allowed to show.
  //! @param candidate
  //! @return const TooltipDef* the matching definition, or nullptr
  const TooltipDef *find_match( const Candidate &candidate ) const;

  //! @brief global switch from res/json/tooltips.json, false disables all tooltips
  bool m_enabled{ true };

  //! @brief the tooltip definitions from res/json/tooltips.json
  std::vector<TooltipDef> m_definitions;

  //! @brief the panel style from res/json/tooltips.json
  PanelStyle m_style;

  //! @brief candidates recorded by event handlers since the last update()
  std::vector<Candidate> m_candidates;

  //! @brief the tooltip being displayed, if any
  std::optional<ActiveTooltip> m_active;

  //! @brief seen tooltip ids for the whole session, see sync_seen()
  std::set<std::string> m_seen;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_TOOLTIPSYSTEM_HPP__
