#ifndef SRC_COMPONENTS_NPC_CROW_HPP__
#define SRC_COMPONENTS_NPC_CROW_HPP__

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <optional>

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Crow NPC (a passive NPC type).
struct Crow
{
  sf::Time sfx_cooldown{ sf::Time::Zero };
  //! @brief World x of the centre of the grave the crow is perched on, which it moves either side of.
  float perch_center_x{ 0.0f };
  //! @brief World x the perched crow's centre is moving to. std::nullopt while standing still.
  std::optional<float> perch_target_x{ std::nullopt };
  //! @brief How much longer the perched crow stands still before picking its next move.
  sf::Time perch_wait{ sf::Time::Zero };
  //! @brief Set to send the crow flying off screen. NpcSystem destroys it once it has left the view.
  bool flee{ false };
  //! @brief Unit vector the crow flies along, set by NpcSystem when it takes off. std::nullopt while perched.
  std::optional<sf::Vector2f> flight_direction{ std::nullopt };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_CROW_HPP__
