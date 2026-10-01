#ifndef SRC_SYSTEMS_STORES_SYSTEMSTORE_HPP__
#define SRC_SYSTEMS_STORES_SYSTEMSTORE_HPP__

#include <entt/fwd.hpp>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

// Forward declarations only
// clang-format off
namespace Game::Audio { class SoundBank; }
namespace sf { class RenderWindow; }
namespace Game::Sys::ProcGen { class CellAutomataSystem; class DLASystem; class LevelGenerator; }
// clang-format on

namespace Game::Sys
{

// Forward declare all system classes
class BaseSystem;
class AnimSystem;
class ArrowSystem;
class AltarSystem;
class BombSystem;
class CorruptionHazardSystem;
class CryptSystem;
class ActionSystem;
class ExitSystem;
class FootstepSystem;
class GraveSystem;
class GrimoireSystem;
class HealingSpringSystem;
class InventorySystem;
class ItemStore;
class SpriteStore;
class ItemSystem;
class NpcStore;
class LootSystem;
class NpcSystem;
class ParticleSystem;
class PassageSystem;
class PersistSystem;
class PlayerSystem;
class RenderDebugSystem;
class RenderGameSystem;
class RenderMenuSystem;
class RenderOverlaySystem;
class SceneInputRouter;
class SinkHoleHazardSystem;
class ShaderSystem;
class ShockwaveSystem;
class ShopSystem;
class LightningSystem;
class RuinSystem;
class WatchmanSystem;
class WispSystem;
class WormholeSystem;

//! @brief Owns and type-erases all Sys::* system instances, keyed by Store::Type, so the rest of the codebase can
//! look them up by enum without depending on every system's header.
class Store
{
public:
  //! @brief Key for the SystemStore concrete classes
  enum class Type {
    //! @brief Key for AnimSystem.
    AnimSystem,
    //! @brief Key for AltarSystem.
    AltarSystem,
    //! @brief Key for ArrowSystem.
    ArrowSystem,
    //! @brief Key for BombSystem.
    BombSystem,
    //! @brief Key for ProcGen::CellAutomataSystem.
    CellAutomataSystem,
    //! @brief Key for CorruptionHazardSystem.
    CorruptionHazardSystem,
    //! @brief Key for CryptSystem.
    CryptSystem,
    //! @brief Key for ProcGen::DLASystem.
    DiffusionLtdAggrSystem,
    //! @brief Key for ActionSystem.
    ActionSystem,
    //! @brief Key for ExitSystem.
    ExitSystem,
    //! @brief Key for FootstepSystem.
    FootstepSystem,
    //! @brief Key for GraveSystem.
    GraveSystem,
    //! @brief Key for GrimoireSystem.
    GrimoireSystem,
    //! @brief Key for HealingSpringSystem.
    HealingSpringSystem,
    //! @brief Key for InventorySystem.
    InventorySystem,
    //! @brief Key for ItemSystem.
    ItemSystem,
    //! @brief Key for RuinSystem.
    RuinSystem,
    //! @brief Key for LightningSystem.
    LightningSystem,
    //! @brief Key for LootSystem.
    LootSystem,
    //! @brief Key for NpcSystem.
    NpcSystem,
    //! @brief Key for ParticleSystem.
    ParticleSystem,
    //! @brief Key for PassageSystem.
    PassageSystem,
    //! @brief Key for PersistSystem.
    PersistSystem,
    //! @brief Key for PlayerSystem.
    PlayerSystem,
    //! @brief Key for ProcGen::LevelGenerator.
    LevelGenerator,
    //! @brief Key for RenderDebugSystem.
    RenderDebugSystem,
    //! @brief Key for RenderGameSystem.
    RenderGameSystem,
    //! @brief Key for RenderMenuSystem.
    RenderMenuSystem,
    //! @brief Key for RenderOverlaySystem.
    RenderOverlaySystem,
    //! @brief Key for SceneInputRouter.
    SceneInputRouter,
    //! @brief Key for ShaderSystem.
    ShaderSystem,
    //! @brief Key for ShockwaveSystem.
    ShockwaveSystem,
    //! @brief Key for ShopSystem.
    ShopSystem,
    //! @brief Key for SinkHoleHazardSystem.
    SinkHoleHazardSystem,
    //! @brief Key for WatchmanSystem.
    WatchmanSystem,
    //! @brief Key for WispSystem.
    WispSystem,
    //! @brief Key for WormholeSystem.
    WormholeSystem,
  };

  //! @brief System type traits - Maps each Sys::Store::Type enum value to its concrete class.
  //! @tparam T
  template <Type T>
  struct SystemTraits;

  //! @brief Construct a new Store object
  //! @param window
  //! @param sound_bank
  //! @param nav_event_dispatcher
  //! @param scenemanager_event_dispatcher
  Store( sf::RenderWindow &window, Audio::SoundBank &sound_bank, entt::dispatcher &nav_event_dispatcher,
         entt::dispatcher &scenemanager_event_dispatcher );

  //! @brief Destroy the Store object. Defined out of line so the data store types can stay forward declared here.
  ~Store();

  //! @brief Find the concrete class using the Sys::Store::Type enum
  //! @tparam T
  //! @return auto&
  template <Type T>
  auto &find()
  {
    using SystemType = typename SystemTraits<T>::type;

    auto it = m_sysmap.find( T );
    if ( it == m_sysmap.end() ) { throw std::runtime_error( "System not found in store: " + std::to_string( static_cast<int>( T ) ) ); }
    if ( !it->second ) { throw std::runtime_error( "System pointer is null: " + std::to_string( static_cast<int>( T ) ) ); }

    return static_cast<SystemType &>( *it->second );
  }

  //! @brief Get the first element of the SystemStore
  //! @return Iterator to the first element.
  auto begin() { return m_sysmap.begin(); }

  //! @brief Get the last element of the SystemStore
  //! @return Iterator past the last element.
  auto end() { return m_sysmap.end(); }

  //! @brief Get the size of the SystemStore
  //! @return The number of systems in the store.
  [[nodiscard]] auto size() const { return m_sysmap.size(); }

private:
  //! @brief Data stores, loaded once at startup. Declared before m_sysmap so they are constructed before, and
  //! outlive, all systems.
  std::unique_ptr<SpriteStore> m_sprite_store;
  std::unique_ptr<ItemStore> m_item_store;
  std::unique_ptr<NpcStore> m_npc_store;

  //! @brief The SystemStore container keyed by Type enum
  std::map<Type, std::unique_ptr<BaseSystem>> m_sysmap;

  //! @brief Empty registry for initialization
  entt::registry m_initial_reg;

  //! @brief Wrapper function that ensures matching pairs are emplaced into SystemStore.
  //! @tparam T
  //! @tparam Args
  //! @param args
  template <Type T, typename... Args>
  void emplace( Args &&...args )
  {
    using SystemType = typename SystemTraits<T>::type;
    m_sysmap.emplace( T, std::make_unique<SystemType>( std::forward<Args>( args )... ) );
  }
};

// Explicit template specializations for SystemTraits
// clang-format off
template<> struct Store::SystemTraits<Store::Type::ActionSystem>           { using type = ActionSystem; };
template<> struct Store::SystemTraits<Store::Type::AltarSystem>            { using type = AltarSystem; };
template<> struct Store::SystemTraits<Store::Type::AnimSystem>             { using type = AnimSystem; };
template<> struct Store::SystemTraits<Store::Type::ArrowSystem>            { using type = ArrowSystem; };
template<> struct Store::SystemTraits<Store::Type::BombSystem>             { using type = BombSystem; };
template<> struct Store::SystemTraits<Store::Type::CellAutomataSystem>     { using type = ProcGen::CellAutomataSystem; };
template<> struct Store::SystemTraits<Store::Type::CorruptionHazardSystem> { using type = CorruptionHazardSystem; };
template<> struct Store::SystemTraits<Store::Type::CryptSystem>            { using type = Game::Sys::CryptSystem; };
template<> struct Store::SystemTraits<Store::Type::DiffusionLtdAggrSystem> { using type = ProcGen::DLASystem; };
template<> struct Store::SystemTraits<Store::Type::ExitSystem>             { using type = ExitSystem; };
template<> struct Store::SystemTraits<Store::Type::FootstepSystem>         { using type = FootstepSystem; };
template<> struct Store::SystemTraits<Store::Type::GraveSystem>            { using type = GraveSystem; };
template<> struct Store::SystemTraits<Store::Type::GrimoireSystem>         { using type = GrimoireSystem; };
template<> struct Store::SystemTraits<Store::Type::HealingSpringSystem>    { using type = HealingSpringSystem; };
template<> struct Store::SystemTraits<Store::Type::InventorySystem>        { using type = InventorySystem; };
template<> struct Store::SystemTraits<Store::Type::ItemSystem>             { using type = ItemSystem; };
template<> struct Store::SystemTraits<Store::Type::RuinSystem>             { using type = RuinSystem; };
template<> struct Store::SystemTraits<Store::Type::LootSystem>             { using type = LootSystem; };
template<> struct Store::SystemTraits<Store::Type::LightningSystem>        { using type = LightningSystem; };
template<> struct Store::SystemTraits<Store::Type::NpcSystem>              { using type = NpcSystem; };
template<> struct Store::SystemTraits<Store::Type::ParticleSystem>         { using type = ParticleSystem; };
template<> struct Store::SystemTraits<Store::Type::PassageSystem>          { using type = Game::Sys::PassageSystem; };
template<> struct Store::SystemTraits<Store::Type::PersistSystem>          { using type = PersistSystem; };
template<> struct Store::SystemTraits<Store::Type::PlayerSystem>           { using type = PlayerSystem; };
template<> struct Store::SystemTraits<Store::Type::LevelGenerator>         { using type = ProcGen::LevelGenerator; };
template<> struct Store::SystemTraits<Store::Type::RenderDebugSystem>      { using type = RenderDebugSystem; };
template<> struct Store::SystemTraits<Store::Type::RenderGameSystem>       { using type = RenderGameSystem; };
template<> struct Store::SystemTraits<Store::Type::RenderOverlaySystem>    { using type = RenderOverlaySystem; };
template<> struct Store::SystemTraits<Store::Type::RenderMenuSystem>       { using type = RenderMenuSystem; };
template<> struct Store::SystemTraits<Store::Type::SceneInputRouter>       { using type = SceneInputRouter; };
template<> struct Store::SystemTraits<Store::Type::ShaderSystem>           { using type = ShaderSystem; };
template<> struct Store::SystemTraits<Store::Type::ShockwaveSystem>        { using type = ShockwaveSystem; };
template<> struct Store::SystemTraits<Store::Type::ShopSystem>             { using type = ShopSystem; };
template<> struct Store::SystemTraits<Store::Type::SinkHoleHazardSystem>   { using type = SinkHoleHazardSystem; };
template<> struct Store::SystemTraits<Store::Type::WatchmanSystem>         { using type = WatchmanSystem; };
template<> struct Store::SystemTraits<Store::Type::WispSystem>             { using type = WispSystem; };
template<> struct Store::SystemTraits<Store::Type::WormholeSystem>         { using type = WormholeSystem; };
// clang-format on

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_SYSTEMSTORE_HPP__
