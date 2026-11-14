#ifndef SRC_SYSTEMS_CRYPT_VARIANTS_OPENCRYPTVARIANT_HPP__
#define SRC_SYSTEMS_CRYPT_VARIANTS_OPENCRYPTVARIANT_HPP__

#include <Systems/Crypt/Variants/CryptVariant.hpp>

namespace Game::Sys::Crypt
{

//! @brief A sprawling, fully open crypt: more and smaller rooms, a dense passage network, no shuffle timer and no
//! levers. Every room is open (with lava pits) from the start and skeletons roam it; collecting the cadaver
//! summons priests and ghosts.
class OpenCryptVariant : public CryptVariant
{
public:
  using CryptVariant::CryptVariant;

  [[nodiscard]] std::string_view name() const override { return "Open"; }

  //! @brief More, smaller rooms
  [[nodiscard]] LayoutParams layout_params() const override;

  //! @brief Rooms may receive several passages, giving a denser network
  [[nodiscard]] PassageConfig passage_config() const override;

  //! @brief Open and connect every room, add lava pits, and release skeletons
  void on_first_enter() override;

  //! @brief No-op: the end room is already connected
  void unlock_objective() override {}

  //! @brief Every room is already open: just clear the lava pits and spawn the exit wave
  void unlock_exit() override;

  //! @brief Priests and ghosts
  void spawn_exit_npcs() override;
};

} // namespace Game::Sys::Crypt

#endif // SRC_SYSTEMS_CRYPT_VARIANTS_OPENCRYPTVARIANT_HPP__
