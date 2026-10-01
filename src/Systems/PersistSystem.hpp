#ifndef SRC_SYSTEMS_PERSISTSYSTEM_HPP__
#define SRC_SYSTEMS_PERSISTSYSTEM_HPP__

#include <Components/Persistent/IBasePersistent.hpp>
#include <Events/LoadSettingsEvent.hpp>
#include <Events/SaveSettingsEvent.hpp>
#include <Systems/BaseSystem.hpp>

#include <entt/entity/fwd.hpp>
#include <nlohmann/json_fwd.hpp>

#include <functional>
#include <optional>
#include <unordered_map>

namespace Game::Sys
{

//! @brief Serializes and deserializes registered Cmp::Persist::IBasePersistent components to/from JSON,
//! so game settings and progress survive between sessions.
class PersistSystem : public BaseSystem
{
public:
  //! @brief Construct a new Persist System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  PersistSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Initializes the component registry for the persistent system.
  //! @note  You must call this function before calling `load_state()` or `save_state()`.
  void initialize_component_registry();

  //! @brief Loads the settings from persistent storage. See `kPersistFilePath`.
  //! @note Your scene must call `initialize_component_registry()` first.
  //! @throws std::runtime_error if the state file cannot be read or is corrupted
  //! @throws std::ios_base::failure if file I/O operations fail
  void load_state();

  //! @brief Saves the settings to persistent storage. See `kPersistFilePath`.
  //! @note Your scene must call `initialize_component_registry()` first.
  //! @throws std::ios_base::failure if file I/O operations fail
  void save_state();

  /// EVENTS
  //! @brief Event handler that forwards to save_state().
  //! @param event
  void on_save_settings_event( [[maybe_unused]] const Events::SaveSettingsEvent &event )
  {
    SPDLOG_DEBUG( "Save Settings Event received" );
    save_state();
  }

  //! @brief Event handler that forwards to load_state().
  //! @param event
  void on_load_settings_event( [[maybe_unused]] const Events::LoadSettingsEvent &event )
  {
    SPDLOG_DEBUG( "Load Settings Event received" );
    load_state();
  }

  //! @brief Add a persistent component to the registry's context if it doesn't already exist
  //! @tparam T
  //! @param reg
  template <typename T>
  static T &add( entt::registry &reg );

  //! @brief Add a persistent component with constructor arguments
  //! @tparam T
  //! @tparam Args
  //! @param reg
  //! @param args
  template <typename T, typename... Args>
  static T &add( entt::registry &reg, Args &&...args );

  //! @brief Get the persistent component object
  //! @tparam T
  //! @param reg
  //! @return T&
  template <typename T>
  static T &get( entt::registry &reg );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Path to the JSON file used to load and save persistent component state.
  static constexpr const char *kPersistFilePath = "res/json/persistent_components.json";

  //! @brief Reads and parses kPersistFilePath, logging and returning std::nullopt on failure.
  static std::optional<nlohmann::json> load_json_file( const std::string &path );

  //! @brief Adds the uninitialised component to the registry and sets the serdes initialise function for the component T.
  //! @note  The component `initialise` functions are called by `initialize_component_registry()`.
  //! @tparam T
  template <typename T>
  void add_component();

  struct ops
  {
    //! @brief Lazily creates the component in the registry and deserialises its initial value.
    std::function<void( const nlohmann::json & )> initialise;

    //! @brief Deserialise from JSON to object. Not every persistent component derives from
    //! IBasePersistent (e.g. DisplayResolution, PlayerStartPosition wrap sf::Vector2 instead), so
    //! this has to be a type-erased closure rather than a common-base pointer.
    std::function<void( const nlohmann::json & )> deserialiser;

    //! @brief Serialise to JSON from object. See deserialiser for why this can't be a pointer.
    std::function<nlohmann::json()> serialiser;
  };

  std::unordered_map<std::string, ops> m_components;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_PERSISTSYSTEM_HPP__
