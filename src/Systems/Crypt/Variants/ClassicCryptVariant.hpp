#ifndef SRC_SYSTEMS_CRYPT_VARIANTS_CLASSICCRYPTVARIANT_HPP__
#define SRC_SYSTEMS_CRYPT_VARIANTS_CLASSICCRYPTVARIANT_HPP__

#include <Systems/Crypt/Variants/CryptVariant.hpp>

namespace Game::Sys::Crypt
{

//! @brief The original crypt: all rooms start closed; every CryptShuffleTimeout (or lever pull) a few rooms open with
//! lava pits, a chest and a lever. Pulling enough levers opens the passage to the objective.
class ClassicCryptVariant : public CryptVariant
{
public:
  using CryptVariant::CryptVariant;

  [[nodiscard]] std::string_view name() const override { return "Classic"; }

  //! @brief Start the shuffle timer and open the first set of rooms
  void on_first_enter() override;

  //! @brief Advance the shuffle timer; shuffle when it expires (until the maze is unlocked)
  //! @param dt Time elapsed since the last frame
  void update( sf::Time dt ) override;

  //! @brief Close the open rooms and open a new random set, with fresh lava pits, chests, a lever and passages
  void shuffle() override;

  //! @brief Unlock the objective after kLeversToUnlock pulls; otherwise shuffle
  void on_lever_activated() override;

  //! @brief Also marks the maze unlocked, which stops the shuffle timer
  void unlock_exit() override;

private:
  //! @brief Number of closed rooms opened on each shuffle
  static constexpr std::size_t kRoomsOpenPerShuffle = 4;

  //! @brief Lever pulls needed to unlock the objective
  static constexpr unsigned kLeversToUnlock = 3;

  //! @brief Levers pulled so far
  unsigned m_enabled_levers{ 0 };

  //! @brief The objective/exit has been unlocked: stop shuffling
  bool m_maze_unlocked{ false };
};

} // namespace Game::Sys::Crypt

#endif // SRC_SYSTEMS_CRYPT_VARIANTS_CLASSICCRYPTVARIANT_HPP__
