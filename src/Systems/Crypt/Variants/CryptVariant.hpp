#ifndef SRC_SYSTEMS_CRYPT_VARIANTS_CRYPTVARIANT_HPP__
#define SRC_SYSTEMS_CRYPT_VARIANTS_CRYPTVARIANT_HPP__

#include <Systems/Crypt/Variants/CryptLayoutParams.hpp>
#include <Systems/ProcGen/PassageSystem.hpp>

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <string_view>

// clang-format off
namespace Game::Sys { class CryptSystem; }
// clang-format on

namespace Game::Sys::Crypt
{

//! @brief A crypt design: the policy for its layout, passages, open/shuffle behaviour, hazards and NPCs.
//! Composes the primitives of Sys::CryptSystem and Sys::PassageSystem. Override the hooks to make a new variant,
//! then register it in CryptVariantArbiter::with_default_variants().
//! @note One instance per CryptScene, created by CryptVariantArbiter when the scene is created; so per-crypt state
//! (lever counts, unlock flags) can live in the subclass and needs no reset.
class CryptVariant
{
public:
  //! @brief Construct a new CryptVariant
  //! @param crypt Room/content toolkit
  //! @param passages Passage toolkit
  CryptVariant( CryptSystem &crypt, PassageSystem &passages )
      : m_crypt( crypt ),
        m_passages( passages )
  {
  }

  virtual ~CryptVariant() = default;
  CryptVariant( const CryptVariant & ) = delete;
  CryptVariant &operator=( const CryptVariant & ) = delete;

  //! @brief Name used for logging and the CryptVariantOverride setting
  [[nodiscard]] virtual std::string_view name() const = 0;

  // ---- generation: called once from CryptScene::on_init() ----

  //! @brief Room count/size tunables used by the default generate()
  [[nodiscard]] virtual LayoutParams layout_params() const { return {}; }

  //! @brief Passage tunables, applied to PassageSystem before generate()
  [[nodiscard]] virtual PassageConfig passage_config() const { return {}; }

  //! @brief Build the rooms. Default: end room, closed rooms (layout_params()), cached passage connections, interior walls.
  //! @param map_size_grid Map size in grid cells
  virtual void generate( sf::Vector2u map_size_grid );

  // ---- lifecycle ----

  //! @brief Called once, when the player first arrives in the crypt (and is not dead). Set up the initial room state.
  virtual void on_first_enter() = 0;

  //! @brief Per-frame policy, e.g. a shuffle timer. Called from CryptSystem::update().
  //! @param dt Time elapsed since the last frame
  virtual void update( [[maybe_unused]] sf::Time dt ) {}

  //! @brief Reconfigure the open rooms/passages. Also requested by Events::CryptRoomEvent::Type::SHUFFLE_PASSAGES.
  virtual void shuffle() {}

  //! @brief The player pulled a lever.
  virtual void on_lever_activated() {}

  //! @brief Open the passage from the occupied room to the end room (objective).
  //! Default: close every other open room, remove levers/hazards, and carve the end room passage.
  virtual void unlock_objective();

  //! @brief The preserved cat was collected: open every room so the player can get back to the exit.
  //! Default: open and connect all rooms, clear chests/levers/lava, then spawn_exit_npcs().
  virtual void unlock_exit();

  //! @brief Spawn the NPC wave that chases the player out. Default: CryptNpcSpawnCount priests.
  virtual void spawn_exit_npcs();

protected:
  //! @brief Room/content toolkit
  CryptSystem &m_crypt;

  //! @brief Passage toolkit
  PassageSystem &m_passages;
};

} // namespace Game::Sys::Crypt

#endif // SRC_SYSTEMS_CRYPT_VARIANTS_CRYPTVARIANT_HPP__
