#ifndef SRC_COMPONENTS_WEAPONS_BURR_HPP__
#define SRC_COMPONENTS_WEAPONS_BURR_HPP__

#include <Components/Position.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <entt/entity/entity.hpp>

#include <optional>

namespace Game::Cmp::Weapons::Projectiles
{

//! @brief Thrown burr projectile
//! @note User input triggered by SceneInputRouter.
//! @note Burr physics are managed by BurrSystem.
class Burr
{
public:
  //! @brief Construct a new Burr projectile.
  //! @param origin The position when the burr was thrown.
  //! @param destination Where the burr will land, scaled by how long the throw was charged.
  Burr( sf::Vector2f origin, sf::Vector2f destination )
      : m_origin( origin ),
        m_destination( destination )
  {
  }

  //! @brief The position when the burr was thrown.
  sf::Vector2f m_origin;
  //! @brief Where the burr will land.
  sf::Vector2f m_destination;
  //! @brief The update delay counter
  sf::Time m_fixed_time_step_accumulator{ sf::Time::Zero };

  //! @brief The last grid cell the burr flew over that doesn't block the player. An obstacle hit drops the burr here.
  std::optional<Cmp::Position> m_last_free_cell;

  //! @brief The obstacle the burr is stuck to once it has hit one, otherwise entt::null.
  entt::entity m_stuck_to{ entt::null };

  //! @brief Time elapsed since the burr stuck to an obstacle; the burr is removed after Cmp::Persist::BurrExpiry
  sf::Time m_stuck_elapsed{ sf::Time::Zero };

  //! @brief Get the speed member
  //! @return float
  [[nodiscard]] float speed() const { return m_speed; }

  //! @brief get the update delay threshold
  [[nodiscard]] sf::Time fixed_time_step_max() const { return m_fixed_time_step_max; }

private:
  //! @brief The speed modifier for the burr velocity
  float m_speed{ 6.f };

  //! @brief The update delay threshold.
  sf::Time m_fixed_time_step_max{ sf::microseconds( 1.f ) };
};

} // namespace Game::Cmp::Weapons::Projectiles

#endif // SRC_COMPONENTS_WEAPONS_BURR_HPP__
