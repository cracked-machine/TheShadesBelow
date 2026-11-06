#ifndef SRC_COMPONENTS_CRYPT_PASSAGESPIKETRAP_HPP__
#define SRC_COMPONENTS_CRYPT_PASSAGESPIKETRAP_HPP__

namespace Game::Cmp::Crypt
{

//! @brief A hazard trap placed on a Cmp::Crypt::PassageBlock that periodically triggers against the
//! player, gated by a per-instance cooldown.
class PassageSpikeTrap : public sf::Vector2f
{
public:
  //! @brief Construct a new spike trap at the given position.
  //! @param pos World position of the trap.
  //! @param passage_id Id of the passage this trap belongs to.
  PassageSpikeTrap( sf::Vector2f pos, unsigned int passage_id )
      : sf::Vector2f( pos ),
        m_passage_id( passage_id ) {

        };
  //! @brief Construct a new spike trap at the given position.
  //! @param x World x position of the trap.
  //! @param y World y position of the trap.
  //! @param passage_id Id of the passage this trap belongs to.
  PassageSpikeTrap( float x, float y, unsigned int passage_id )
      : sf::Vector2f( x, y ),
        m_passage_id( passage_id ) {

        };

  //! @brief Id of the passage this trap belongs to.
  unsigned int m_passage_id = 0;
  //! @brief Minimum time that must elapse between trap activations.
  sf::Time m_cooldown_threshold{ sf::seconds( 3 ) };
  //! @brief Tracks elapsed time since the trap last triggered, checked against m_cooldown_threshold.
  //! Starts at the threshold so the first activation is immediate.
  sf::Time m_cooldown_timer{ m_cooldown_threshold };
};

} // namespace Game::Cmp::Crypt

#endif // SRC_COMPONENTS_CRYPT_PASSAGESPIKETRAP_HPP__
