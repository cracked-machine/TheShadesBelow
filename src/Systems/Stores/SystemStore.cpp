
#include <SceneControl/SceneInputRouter.hpp>
#include <Systems/ActionSystem.hpp>
#include <Systems/AltarSystem.hpp>
#include <Systems/AnimSystem.hpp>
#include <Systems/ArrowSystem.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/CryptSystem.hpp>
#include <Systems/ExitSystem.hpp>
#include <Systems/FootstepSystem.hpp>
#include <Systems/GraveSystem.hpp>
#include <Systems/GrimoireSystem.hpp>
#include <Systems/HealingSpringSystem.hpp>
#include <Systems/InventorySystem.hpp>
#include <Systems/ItemSystem.hpp>
#include <Systems/LootSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PlayerSystem.hpp>
#include <Systems/ProcGen/CellAutomataSystem.hpp>
#include <Systems/ProcGen/DLASystem.hpp>
#include <Systems/ProcGen/LevelGenerator.hpp>
#include <Systems/ProcGen/PassageSystem.hpp>
#include <Systems/Render/RenderDebugSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Render/RenderMenuSystem.hpp>
#include <Systems/Render/RenderOverlaySystem.hpp>
#include <Systems/RuinSystem.hpp>
#include <Systems/ShaderSystem.hpp>
#include <Systems/ShopSystem.hpp>
#include <Systems/Stores/BaseStore.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Systems/Threats/BombSystem.hpp>
#include <Systems/Threats/HazardFieldSystemImpl.hpp>
#include <Systems/Threats/LightningSystem.hpp>
#include <Systems/Threats/NpcSystem.hpp>
#include <Systems/Threats/ShockwaveSystem.hpp>
#include <Systems/Threats/WatchmanSystem.hpp>
#include <Systems/Threats/WispSystem.hpp>
#include <Systems/Threats/WormholeSystem.hpp>

namespace Game::Sys
{

Store::Store( sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank, entt::dispatcher &nav_event_dispatcher,
              entt::dispatcher &scenemanager_event_dispatcher )
{
  emplace<Type::ActionSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::ArrowSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::AltarSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::AnimSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::BombSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::CellAutomataSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::DiffusionLtdAggrSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::CorruptionHazardSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::CryptSystem>( m_initial_reg, window, sprite_factory, sound_bank, scenemanager_event_dispatcher );
  emplace<Type::ExitSystem>( m_initial_reg, window, sprite_factory, sound_bank, scenemanager_event_dispatcher );
  emplace<Type::FootstepSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::GraveSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::GrimoireSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::HealingSpringSystem>( m_initial_reg, window, sprite_factory, sound_bank, scenemanager_event_dispatcher );
  emplace<Type::InventorySystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::ItemStore>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::ItemSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::NpcStore>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::RuinSystem>( m_initial_reg, window, sprite_factory, sound_bank, scenemanager_event_dispatcher );
  emplace<Type::LightningSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::LootSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::NpcSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::ParticleSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::PassageSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::PlayerSystem>( m_initial_reg, window, sprite_factory, sound_bank, scenemanager_event_dispatcher );
  emplace<Type::PersistSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::LevelGenerator>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::RenderDebugSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::RenderGameSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::RenderMenuSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::RenderOverlaySystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::ShaderSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::SceneInputRouter>( m_initial_reg, window, sprite_factory, sound_bank, nav_event_dispatcher, scenemanager_event_dispatcher );
  emplace<Type::ShockwaveSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::ShopSystem>( m_initial_reg, window, sprite_factory, sound_bank, scenemanager_event_dispatcher );
  emplace<Type::SinkHoleHazardSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::WatchmanSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::WispSystem>( m_initial_reg, window, sprite_factory, sound_bank );
  emplace<Type::WormholeSystem>( m_initial_reg, window, sprite_factory, sound_bank );
}

} // namespace Game::Sys