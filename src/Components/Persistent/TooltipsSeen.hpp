#ifndef SRC_COMPONENTS_PERSISTENT_TOOLTIPSSEEN_HPP__
#define SRC_COMPONENTS_PERSISTENT_TOOLTIPSSEEN_HPP__

#include <nlohmann/json.hpp>
#include <set>
#include <string>

namespace Game::Cmp::Persist
{

//! @brief Persistent set of tooltip ids (see res/json/tooltips.json) the player has already seen.
//! Used by Sys::TooltipSystem so that "once" tooltips are only shown once, across scenes and restarts.
//! @note Stored as a std::set so the serialized array keeps a stable order in persist.json.
class TooltipsSeen
{
public:
  //! @brief Construct an empty seen-list.
  TooltipsSeen() = default;

  //! @brief Get the class name used for persistence/registry lookup.
  //! @return std::string the class name "TooltipsSeen"
  std::string class_name() const { return "TooltipsSeen"; }

  //! @brief Check whether a tooltip has already been seen.
  //! @param id the tooltip id
  //! @return true if the id is in the seen-list
  bool contains( const std::string &id ) const { return m_ids.contains( id ); }

  //! @brief Mark a tooltip as seen.
  //! @param id the tooltip id
  void insert( const std::string &id ) { m_ids.insert( id ); }

  //! @brief Get a mutable reference to the seen-list.
  //! @return std::set<std::string>& the tooltip ids
  std::set<std::string> &get_value() { return m_ids; }
  //! @brief Get a read-only reference to the seen-list.
  //! @return const std::set<std::string>& the tooltip ids
  const std::set<std::string> &get_value() const { return m_ids; }

  //! @brief Deserialize the seen-list from json (res/json/persist.json).
  //! A missing or non-array "value" is treated as an empty list, and non-string entries are skipped.
  //! @param json_data the json object to read the "value" field from
  void deserialize( const nlohmann::json &json_data )
  {
    m_ids.clear();
    if ( not json_data.contains( "value" ) or not json_data["value"].is_array() ) { return; }
    for ( const auto &id : json_data["value"] )
    {
      if ( id.is_string() ) { m_ids.insert( id.get<std::string>() ); }
    }
  }

  //! @brief Serialize the seen-list to json (res/json/persist.json).
  //! @return nlohmann::json object with "type" set to "std::set<std::string>" and "value" holding the ids
  nlohmann::json serialize() const
  {
    nlohmann::json json_data;
    json_data["type"] = "std::set<std::string>";
    json_data["value"] = m_ids;
    return json_data;
  }

private:
  //! @brief the ids of the tooltips already shown
  std::set<std::string> m_ids;
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_TOOLTIPSSEEN_HPP__
