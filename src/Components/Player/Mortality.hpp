#ifndef SRC_COMPONENTS_PLAYER_MORTALITY_HPP__
#define SRC_COMPONENTS_PLAYER_MORTALITY_HPP__

#include <cstdint>
namespace Game::Cmp::Player
{

//! @brief Tracks the player's life/death state and the progress of any in-flight death animation.
class Mortality
{

public:
  //! @brief The player's current life/death state, and cause of death when dying/dead.
  enum class State : uint8_t {
    //! @brief Player is alive and well. Game is ongoing.
    ALIVE = 0,
    //! @brief Death in process: Player is falling
    FALLING = 1,
    //! @brief Death in process: Player is decaying
    DECAYING = 2,
    //! @brief Player kill by NPC collision or Fear
    HAUNTED = 3,
    //! @brief Death in process: Player is exploding
    EXPLODING = 4,
    //! @brief Death in process: Player is drowning.
    DROWNING = 5,
    //! @brief Killed by crypt walls closing on player
    SQUISHED = 6,
    //! @brief killed by Despair stat == 100
    SUICIDE = 7,
    //! @brief Killed by lava collision
    IGNITED = 8,
    //! @brief Killed by spike trap collision
    SKEWERED = 9,
    //! @brief killed by Lightning strike
    SHOCKED = 10,
    //! @brief killed by Shadow Hand
    SHADOWCURSED = 11,
    //! @brief When player Fear stat == 100
    TERRIFIED = 12,
    //! @brief When player health == 0 and hypoxia == 100
    SUFFOCATED = 13,
    //! @brief When player health == 0 and tacycardia/bradycardia == 100
    CARDIACARREST = 14,
    //! @brief When player health == 0 and venom == 100
    POISONED = 15,

    //! @brief Player is dead. final state. end the game.
    DEAD = 255
  };

  //! @brief Construct with an initial state and death progress.
  //! @param initial_state initial life/death state
  //! @param death_progress_init initial death animation progress
  explicit Mortality( State initial_state = State::ALIVE, float death_progress_init = 0.0f )
      : state( initial_state ),
        death_progress( death_progress_init )
  {
  }

  //! @brief Current life/death state.
  State state{ State::ALIVE };

  //! @brief Progress of death animation from 0.0 (start) to 1.0 (complete)
  //! @details Render systems update this value. Logic systems monitor it
  //!          to transition state to DEAD when >= 1.0
  float death_progress{ 0.0f };
};

} // namespace Game::Cmp::Player

#endif // SRC_COMPONENTS_PLAYER_MORTALITY_HPP__
