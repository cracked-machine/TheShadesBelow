#ifndef SRC_COMPONENTS_PERSISTENT_IBASEPERSISTENT_HPP__
#define SRC_COMPONENTS_PERSISTENT_IBASEPERSISTENT_HPP__

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace Game::Cmp::Persist
{

//! @brief Abstract base interface for persistent components that can be serialized and deserialized.
//! @details IBasePersistent defines the contract for components that need to maintain their state
//! across application sessions through serialization. This interface provides the foundation
//! for a persistent component system where objects can save their state to JSON format
//! and restore it later.
//!
//! Classes implementing this interface must provide:
//! - A unique class name identifier for type identification
//! - Serialization capability to convert object state to JSON
//! - Deserialization capability to restore object state from JSON
//!
//! This interface is typically used in game engines, configuration systems, or any
//! application requiring persistent state management.
//! @note All derived classes must implement the three pure virtual methods to ensure
//!       proper serialization/deserialization functionality.
//! @note The virtual destructor ensures proper cleanup in polymorphic scenarios.
class IBasePersistent
{
public:
  //! @brief polymorphic destructor for derived classes
  virtual ~IBasePersistent() = default;

  //! @brief Returns the class name as a string for identification purposes.
  //! @details This pure virtual method must be implemented by derived classes to provide
  //! a string representation of their class name. This is typically used for
  //! serialization, debugging, or runtime type identification in persistent
  //! component systems.
  //! @return std::string The name of the implementing class
  [[nodiscard]] virtual std::string class_name() const = 0;

  //! @brief Deserializes object state from JSON data.
  //! @details This pure virtual method must be implemented by derived classes to restore
  //! the object's state from the provided JSON data. The implementation should
  //! parse the JSON and set the appropriate member variables to reconstruct
  //! the object's previous state.
  //! @param json_data The JSON object containing the serialized data to be loaded
  //! @note May throw exceptions if the JSON data is malformed or incompatible
  virtual void deserialize( const nlohmann::json &json_data ) = 0;

  //! @brief Serializes the persistent component to JSON format.
  //! @details This pure virtual method must be implemented by derived classes to convert
  //! the component's data into a JSON representation for persistence purposes.
  //! The serialized data should contain all necessary information to reconstruct
  //! the component's state during deserialization.
  //! @return nlohmann::json A JSON object containing the serialized component data.
  virtual nlohmann::json serialize() const = 0;

  //! @brief Renders the ImGui widget used to view/edit this component's value.
  virtual void render_widget() = 0;

  //! @brief Returns the display name/description for this component.
  [[nodiscard]] virtual const std::string get_detail() const = 0;
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_IBASEPERSISTENT_HPP__
