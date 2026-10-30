#ifndef SRC_COMPONENTS_NPC_MIMIC_HPP__
#define SRC_COMPONENTS_NPC_MIMIC_HPP__

#include <SFML/System/Time.hpp>

#include <algorithm>

namespace Game::Cmp::Npc
{

//! @brief Marker component tagging an entity as the Mimic NPC (a non-hostile NPC type).
struct Mimic
{
  //! @brief Lifespan of a mimic spawned at max player luck.
  static constexpr float kMinTimeoutSecs{ 30.f };
  //! @brief Lifespan of a mimic spawned at zero player luck.
  static constexpr float kMaxTimeoutSecs{ 90.f };

  //! @brief Construct a Mimic whose lifespan scales inversely with the player's luck.
  //! High luck: short-lived mimic (and a low teleport chance). Low luck: long-lived mimic (and a high teleport chance).
  //! @param player_luck The player's luck stat at spawn time, clamped to [0,100].
  explicit Mimic( int player_luck )
      : timeout( sf::seconds( kMaxTimeoutSecs -
                              ( ( kMaxTimeoutSecs - kMinTimeoutSecs ) * ( static_cast<float>( std::clamp( player_luck, 0, 100 ) ) / 100.f ) ) ) )
  {
  }

  sf::Time timer{ sf::Time::Zero };
  sf::Time timeout{ sf::seconds( kMaxTimeoutSecs ) };
  //! @brief The mimic flashes for this long before it despawns, as a warning to the player.
  sf::Time flash_duration{ sf::seconds( 5.f ) };
  //! @brief How long the player fades out before the teleport to the mimic's position when it despawns.
  //! The teleport chance is 1 - (player luck / 100).
  sf::Time teleport_delay{ sf::seconds( 2.f ) };
};

} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_MIMIC_HPP__
