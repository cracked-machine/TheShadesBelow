#ifndef SRC_COMPONENTS_PERSISTENT_BASEPERSISTENT_HPP__
#define SRC_COMPONENTS_PERSISTENT_BASEPERSISTENT_HPP__

#include <Components/Persistent/IBasePersistent.hpp>

#include <nlohmann/json_fwd.hpp>
#include <source_location>

namespace Game::Cmp::Persist
{

//! @brief Log helper that tags a message with its call site.
//! @note Defined in BasePersistent.cpp.
//! @param msg the message to log
//! @param loc call site info, defaults to the caller's location
void log_msg( const std::string &msg, const std::source_location &loc = std::source_location::current() );

//! @brief Base class for a persistent, tunable game setting backed by a value of type T.
//! @note These types should be added to the registry as context variables. If added as
//! components to entities instead, they will be destroyed on game reset and lose their values.
//! @tparam T the underlying value type of the setting
template <typename T>
class BasePersistent : public IBasePersistent
{
public:
  //! @brief Construct a persistent value with optional min/max bounds.
  //! @param value_in initial value
  //! @param min_value_in minimum allowed value
  //! @param max_value_in maximum allowed value
  BasePersistent( T value_in, T min_value_in = T{}, T max_value_in = T{} );

  //! @brief polymorphic destructor for derived classes
  virtual ~BasePersistent() = default;

  //! @brief Get a mutable reference to the current value.
  //! @return T& the current value
  T &get_value() { return value; };
  //! @brief Get a read-only reference to the current value.
  //! @return const T& the current value
  const T &get_value() const { return value; };
  //! @brief Get a mutable reference to the minimum allowed value.
  //! @return T& the minimum value
  T &get_min_value() { return min_value; };
  //! @brief Get a mutable reference to the maximum allowed value.
  //! @return T& the maximum value
  T &get_max_value() { return max_value; };
  //! @brief Get a mutable reference to the human-readable detail string.
  //! @return std::string& the detail string
  std::string &get_detail() { return detail; };
  //! @brief Get a mutable reference to the ImGui format string used to display the value.
  //! @return std::string& the format string
  std::string &get_format() { return format; };

  //! @brief Pointer accessors for ImGui SliderScalar compatibility.
  //! @return T* pointer to the current value
  T *get_value_ptr() { return &value; };
  //! @brief Get a pointer to the minimum allowed value, for ImGui SliderScalar compatibility.
  //! @return T* pointer to the minimum value
  T *get_min_value_ptr() { return &min_value; };
  //! @brief Get a pointer to the maximum allowed value, for ImGui SliderScalar compatibility.
  //! @return T* pointer to the maximum value
  T *get_max_value_ptr() { return &max_value; };
  //! @brief Get a pointer to the detail string, for ImGui SliderScalar compatibility.
  //! @return std::string* pointer to the detail string
  std::string *get_detail_ptr() { return &detail; };
  //! @brief Get a pointer to the format string, for ImGui SliderScalar compatibility.
  //! @return std::string* pointer to the format string
  std::string *get_format_ptr() { return &format; };

  //! @brief serialise from peristent component object to json (res/json/persist.json)
  //! Declarations only - defined in .cpp
  //! @return nlohmann::json
  virtual nlohmann::json serialize() const override;

  //! @brief deserialise from json (res/json/persist.json) to peristent component object
  //! Declarations only - defined in .cpp
  //! @param json_data
  virtual void deserialize( const nlohmann::json &json_data ) override;

  //! @brief call imgui widget function for data type T
  //! Declarations only - defined in .cpp
  void render_widget() override;

protected:
  //! @brief the current value of this setting
  T value{};
  //! @brief the minimum allowed value
  T min_value{};
  //! @brief the maximum allowed value
  T max_value{};
  //! @brief human-readable detail/description string shown in the UI
  std::string detail{ "" };
  //! @brief ImGui format string used to display the value
  std::string format{ "" };
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_BASEPERSISTENT_HPP__
