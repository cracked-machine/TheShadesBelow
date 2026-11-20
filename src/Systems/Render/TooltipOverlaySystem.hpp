#ifndef SRC_SYSTEMS_RENDER_TOOLTIPOVERLAYSYSTEM_HPP__
#define SRC_SYSTEMS_RENDER_TOOLTIPOVERLAYSYSTEM_HPP__

#include <Events/AddInventoryEvent.hpp>
#include <Events/AltarApproachedEvent.hpp>
#include <Events/AltarBloodOfferedEvent.hpp>
#include <Events/BloodRefusedEvent.hpp>
#include <Events/ConsumeRefusedEvent.hpp>
#include <Events/CryptEntryRefusedEvent.hpp>
#include <Events/DismissTooltipEvent.hpp>
#include <Events/GrimoireDiscoveryEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Systems/Render/BaseOverlaySystem.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <nlohmann/json_fwd.hpp>

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Game::Sys
{

//! @brief Displays data-driven help messages (res/json/tooltips.json) in a panel in front of the game. Each tooltip's
//! "position" places the panel at a fixed point on screen, above the world object that triggered it (e.g. an altar),
//! or above the player.
//! @details Listens to game events without consuming them. Event handlers only record a candidate; candidates are
//! evaluated in update(), which runs in the next scene update, so every other handler for the event has already
//! run and game state is up to date, regardless of the dispatcher's handler order.
//! A repeating ("always") tooltip never replaces an active one-time ("once") tooltip, so a notification raised in the
//! same frame as a tutorial (e.g. picking up an item) cannot hide it. A tooltip's text may contain "{item}", which is
//! replaced by the display name of the item carried by the triggering event.
//! A tooltip with "pause": true pauses gameplay while it is shown (Cmp::SceneSettings::Paused::Reason::TOOLTIP). It keeps
//! counting down in real time, so gameplay resumes when it expires or is dismissed with Enter. Without "duration_s" it
//! stays until dismissed with Enter, and "Press <Enter> to dismiss." is appended to its text. It never starts while the
//! pause menu is open. Pausing would cut off the sound effects playing when it triggered (e.g. the pickup sound), so it
//! is held back until they finish, see PendingTooltip.
class TooltipOverlaySystem : public BaseOverlaySystem
{
public:
  //! @brief Construct a new Tooltip System object and load res/json/tooltips.json
  //! @param reg
  //! @param window
  //! @param sound_bank
  TooltipOverlaySystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Process the reset request, evaluate pending candidates, count down the active tooltip and draw it.
  //! @note OverlayArbitrationSystem skips this while the pause menu or grimoire is open, so the countdown stops too. It
  //! still runs during this system's own tooltip pause.
  //! @param dt the frame delta time
  void update( sf::Time dt ) override;

  //! @brief Record a candidate for tooltips triggered by "AddInventoryEvent".
  //! @param event
  void on_add_inventory_event( const Events::AddInventoryEvent &event );

  //! @brief Record a candidate for tooltips triggered by "ItemPickedUpEvent".
  //! @param event
  void on_item_picked_up_event( const Events::ItemPickedUpEvent &event );

  //! @brief Record a candidate for tooltips triggered by "GrimoireDiscoveryEvent".
  //! @param event
  void on_grimoire_discovery_event( const Events::GrimoireDiscoveryEvent &event );

  //! @brief Record a candidate for tooltips triggered by "BloodRefusedEvent".
  //! @param event
  void on_blood_refused_event( const Events::BloodRefusedEvent &event );

  //! @brief Record a candidate for tooltips triggered by "AltarApproachedEvent".
  //! @param event
  void on_altar_approached_event( [[maybe_unused]] const Events::AltarApproachedEvent &event );

  //! @brief Record a candidate for tooltips triggered by "AltarBloodOfferedEvent".
  //! @param event
  void on_altar_blood_offered_event( const Events::AltarBloodOfferedEvent &event );

  //! @brief Record a candidate for tooltips triggered by "ConsumeRefusedEvent".
  //! @param event
  void on_consume_refused_event( const Events::ConsumeRefusedEvent &event );

  //! @brief Record a candidate for tooltips triggered by "CryptEntryRefusedEvent".
  //! @param event
  void on_crypt_entry_refused_event( [[maybe_unused]] const Events::CryptEntryRefusedEvent &event );

  //! @brief Hide the active tooltip before its timer expires, resuming gameplay if it paused it.
  //! @param event
  void on_dismiss_tooltip_event( [[maybe_unused]] const Events::DismissTooltipEvent &event );

private:
  //! @brief Draw the active tooltip panel (if any) in screen space.
  void render();

  //! @brief How often a tooltip may be shown
  enum class Frequency {
    //! @brief Shown the first time only, tracked in Cmp::Persist::TooltipsSeen
    ONCE,
    //! @brief Shown every time it is triggered
    ALWAYS,
    //! @brief Never shown
    DISABLED
  };

  //! @brief Where a tooltip's panel is drawn, "position" in res/json/tooltips.json
  struct TooltipPosition
  {
    //! @brief What the panel is anchored to
    enum class Anchor
    {
      //! @brief centred at `screen`
      SCREEN,
      //! @brief "above_source": centred above the world bounds carried by the triggering event, following the camera.
      //! Falls back to `screen` if the event carries no bounds.
      SOURCE,
      //! @brief "above_player": centred above the player, following the camera. Falls back to `screen` if there is no
      //! player in the scene.
      PLAYER
    };
    Anchor anchor{ Anchor::SCREEN };
    //! @brief panel centre as a fraction of the display resolution
    sf::Vector2f screen{ 0.5f, 0.85f };
  };

  //! @brief A tooltip definition from res/json/tooltips.json
  struct TooltipDef
  {
    //! @brief unique id, stored in Cmp::Persist::TooltipsSeen
    std::string id;
    //! @brief name of the event that triggers this tooltip, e.g. "AddInventoryEvent", "ItemPickedUpEvent",
    //! "GrimoireDiscoveryEvent", "BloodRefusedEvent", "AltarApproachedEvent", "AltarBloodOfferedEvent",
    //! "ConsumeRefusedEvent" or "CryptEntryRefusedEvent"
    std::string trigger;
    //! @brief optional item key condition, empty matches any item. A trailing '*' matches any item key with that
    //! prefix, e.g. "item.grave.blood*"
    std::string item;
    //! @brief the message to display
    std::string text;
    //! @brief how often the tooltip may be shown
    Frequency frequency{ Frequency::ONCE };
    //! @brief how long the tooltip is displayed for, std::nullopt to stay until dismissed with Enter (a pausing
    //! tooltip without "duration_s")
    std::optional<sf::Time> duration{ sf::seconds( 5.f ) };
    //! @brief where the panel is drawn
    TooltipPosition position;
    //! @brief optional "pause": true pauses gameplay while the tooltip is shown
    bool pause{ false };
  };

  //! @brief An event occurrence recorded by an event handler, waiting to be evaluated by update()
  struct Candidate
  {
    //! @brief name of the event that was received
    std::string trigger;
    //! @brief item key carried by the event, if any
    std::string item;
    //! @brief world bounds of the object that raised the event, if any (see TooltipPosition::Anchor::SOURCE)
    std::optional<sf::FloatRect> source_bounds{};
  };

  //! @brief The tooltip currently being displayed
  struct ActiveTooltip
  {
    //! @brief the message to display
    std::string text;
    //! @brief time left before the tooltip is hidden, std::nullopt to stay until dismissed with Enter
    std::optional<sf::Time> remaining;
    //! @brief the frequency of the definition being displayed, see update()
    Frequency frequency;
    //! @brief where the panel is drawn
    TooltipPosition position;
    //! @brief world bounds of the object that raised the event, if any
    std::optional<sf::FloatRect> source_bounds;
    //! @brief true if gameplay is paused while this tooltip is shown
    bool pause{ false };
  };

  //! @brief A pausing tooltip waiting for the sound effects that were playing when it triggered to finish. Effects
  //! started afterwards don't extend the wait, so continuously replayed effects (e.g. footsteps) can't hold it back.
  struct PendingTooltip
  {
    //! @brief the tooltip to show once the delay has passed
    ActiveTooltip tooltip;
    //! @brief time left before the tooltip is shown
    sf::Time delay;
  };

  //! @brief Panel style from res/json/tooltips.json
  struct PanelStyle
  {
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

  //! @brief Make `tooltip` the active tooltip, unless it is repeating ("always") and a one-time ("once") tooltip is
  //! already shown: a repeating notification must not hide a one-time tutorial.
  //! @param tooltip
  void activate( ActiveTooltip tooltip );

  //! @brief Pause gameplay (Cmp::SceneSettings::Paused::Reason::TOOLTIP) while a pausing tooltip is active, and resume it
  //! when there is none. Never overrides the pause menu (PAUSEMENU). No-op for scenes without a pause setting.
  void sync_pause_setting();

  //! @brief Find the first definition that matches the candidate and is allowed to show.
  //! @param candidate
  //! @return const TooltipDef* the matching definition, or nullptr
  const TooltipDef *find_match( const Candidate &candidate ) const;

  //! @brief Read a tooltip's "position": an {"x", "y"} screen fraction, "above_source" or "above_player".
  //! @param entry the tooltip's json object
  //! @return std::optional<TooltipPosition> the position (the default if omitted), or std::nullopt if malformed
  static std::optional<TooltipPosition> parse_position( const nlohmann::json &entry );

  //! @brief The panel centre in window pixels for the active tooltip.
  //! @param panel_size the panel size in pixels
  //! @param display_size the display resolution
  sf::Vector2f panel_centre( sf::Vector2f panel_size, sf::Vector2f display_size ) const;

  //! @brief Get the definition's text with "{item}" replaced by the display name of the candidate's item, and
  //! "Press <Enter> to dismiss." appended if it has no duration.
  //! @param def
  //! @param candidate
  //! @return std::string the text to display
  static std::string format_text( const TooltipDef &def, const Candidate &candidate );

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

  //! @brief a pausing tooltip waiting for the current sound effects to finish, if any
  std::optional<PendingTooltip> m_pending;

  //! @brief seen tooltip ids for the whole session, see sync_seen()
  std::set<std::string> m_seen;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_RENDER_TOOLTIPOVERLAYSYSTEM_HPP__
