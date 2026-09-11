

#include <Components/Persistent/ArmedBlinkFreq.hpp>
#include <Components/Persistent/ArmedBlockColourBorder.hpp>
#include <Components/Persistent/ArmedBlockColourFill.hpp>
#include <Components/Persistent/ArmedOffDelay.hpp>
#include <Components/Persistent/ArmedOnDelay.hpp>
#include <Components/Persistent/BlastRadius.hpp>
#include <Components/Persistent/BombBonus.hpp>
#include <Components/Persistent/BombDamage.hpp>
#include <Components/Persistent/CameraSmoothSpeed.hpp>
#include <Components/Persistent/CorruptionDamage.hpp>
#include <Components/Persistent/CorruptionSeed.hpp>
#include <Components/Persistent/CryptNpcSpawnCount.hpp>
#include <Components/Persistent/CryptShuffleTimeout.hpp>
#include <Components/Persistent/DiggingCooldownThreshold.hpp>
#include <Components/Persistent/DiggingDamagePerHit.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/EffectsVolume.hpp>
#include <Components/Persistent/ExitKeyRequirement.hpp>
#include <Components/Persistent/FuseDelay.hpp>
#include <Components/Persistent/GraveNumMultiplier.hpp>
#include <Components/Persistent/GraveyardProcGenBirthThreshold.hpp>
#include <Components/Persistent/GraveyardProcGenInitChance.hpp>
#include <Components/Persistent/GraveyardProcGenMaxIterations.hpp>
#include <Components/Persistent/GraveyardProcGenSurvivalThreshold.hpp>
#include <Components/Persistent/HazardPushbackResist.hpp>
#include <Components/Persistent/HealthBonus.hpp>
#include <Components/Persistent/LightningDamage.hpp>
#include <Components/Persistent/MaxNumAltars.hpp>
#include <Components/Persistent/MaxNumCrypts.hpp>
#include <Components/Persistent/MusicVolume.hpp>
#include <Components/Persistent/NpcActivateScale.hpp>
#include <Components/Persistent/NpcDeathAnimFramerate.hpp>
#include <Components/Persistent/NpcPushBack.hpp>
#include <Components/Persistent/NpcShockwaveFreq.hpp>
#include <Components/Persistent/NpcShockwaveMaxRadius.hpp>
#include <Components/Persistent/NpcShockwaveResolution.hpp>
#include <Components/Persistent/NpcShockwaveSpeed.hpp>
#include <Components/Persistent/NpcWatchmanConeHalfAngle.hpp>
#include <Components/Persistent/NpcWatchmanConeLength.hpp>
#include <Components/Persistent/NpcWatchmanGunFireRate.hpp>
#include <Components/Persistent/NpcWatchmanGunReloadRate.hpp>
#include <Components/Persistent/NpcWatchmanIdleDirectionChangeInterval.hpp>
#include <Components/Persistent/NpcWatchmanSpawnCooldown.hpp>
#include <Components/Persistent/NpcWatchmanSpawnInfamy.hpp>
#include <Components/Persistent/NpcWatchmanSpawnMax.hpp>
#include <Components/Persistent/NpcWatchmanSweepAmplitude.hpp>
#include <Components/Persistent/NpcWatchmanSweepSpeed.hpp>
#include <Components/Persistent/PcDamageDelay.hpp>
#include <Components/Persistent/PlantBurnDuration.hpp>
#include <Components/Persistent/PlayerAnimStrideLength.hpp>
#include <Components/Persistent/PlayerDiagonalLerpSpeedModifier.hpp>
#include <Components/Persistent/PlayerFootstepAddDelay.hpp>
#include <Components/Persistent/PlayerFootstepFadeDelay.hpp>
#include <Components/Persistent/PlayerFootstepSfxStrideLength.hpp>
#include <Components/Persistent/PlayerLerpInterruptThreshold.hpp>
#include <Components/Persistent/PlayerMovementSpeed.hpp>
#include <Components/Persistent/PlayerShortcutLerpSpeedModifier.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Persistent/PostPullMovementDelay.hpp>
#include <Components/Persistent/RuinMaxCobwebs.hpp>
#include <Components/Persistent/RuinMaxSpiders.hpp>
#include <Components/Persistent/RuinProcGenBirthThreshold.hpp>
#include <Components/Persistent/RuinProcGenInitChance.hpp>
#include <Components/Persistent/RuinProcGenMaxIterations.hpp>
#include <Components/Persistent/RuinProcGenSurvivalThreshold.hpp>
#include <Components/Persistent/ShopMaxItems.hpp>
#include <Components/Persistent/ShopMaxPrice.hpp>
#include <Components/Persistent/ShopMinPrice.hpp>
#include <Components/Persistent/SinkholeSeed.hpp>
#include <Components/Persistent/WeaponDegradePerHit.hpp>
#include <Components/Persistent/WormholeAnimFramerate.hpp>
#include <Components/Persistent/WormholeSeed.hpp>
#include <Events/LoadSettingsEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace Game::Sys
{

PersistSystem::PersistSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::SaveSettingsEvent>().connect<&Sys::PersistSystem::on_save_settings_event>( this );
  std::ignore = get_systems_event_queue().sink<Events::LoadSettingsEvent>().connect<&Sys::PersistSystem::on_load_settings_event>( this );
  SPDLOG_DEBUG( "PersistSystem initialized" );
}

/* static */ std::optional<nlohmann::json> PersistSystem::load_json_file( const std::string &path )
{
  std::ifstream inputFile( path );
  if ( not inputFile.is_open() )
  {
    SPDLOG_ERROR( "Failed to open {}", path );
    return std::nullopt;
  }

  nlohmann::json data;
  inputFile >> data;
  return data;
}

void PersistSystem::initialize_component_registry()
{
  // Load component definitions from JSON
  const auto definitions = load_json_file( kPersistFilePath );
  if ( not definitions ) { return; }

  // Register the serialize/deserialize functions for every persistent component type.
  add_component<Cmp::Persist::ArmedBlinkFreq>();
  add_component<Cmp::Persist::ArmedBlockColourFill>();
  add_component<Cmp::Persist::ArmedBlockColourBorder>();
  add_component<Cmp::Persist::ArmedOffDelay>();
  add_component<Cmp::Persist::ArmedOnDelay>();
  add_component<Cmp::Persist::BlastRadius>();
  add_component<Cmp::Persist::BombBonus>();
  add_component<Cmp::Persist::BombDamage>();
  add_component<Cmp::Persist::CameraSmoothSpeed>();
  add_component<Cmp::Persist::CorruptionDamage>();
  add_component<Cmp::Persist::CryptShuffleTimeout>();
  add_component<Cmp::Persist::CryptNpcSpawnCount>();
  add_component<Cmp::Persist::DiggingCooldownThreshold>();
  add_component<Cmp::Persist::DiggingDamagePerHit>();
  add_component<Cmp::Persist::DisplayResolution>();
  add_component<Cmp::Persist::EffectsVolume>();
  add_component<Cmp::Persist::ExitKeyRequirement>();
  add_component<Cmp::Persist::FuseDelay>();
  add_component<Cmp::Persist::GraveNumMultiplier>();
  add_component<Cmp::Persist::GraveyardProcGenInitChance>();
  add_component<Cmp::Persist::GraveyardProcGenMaxIterations>();
  add_component<Cmp::Persist::GraveyardProcGenBirthThreshold>();
  add_component<Cmp::Persist::GraveyardProcGenSurvivalThreshold>();
  add_component<Cmp::Persist::HazardPushbackResist>();
  add_component<Cmp::Persist::HealthBonus>();
  add_component<Cmp::Persist::LightningDamage>();
  add_component<Cmp::Persist::MaxNumAltars>();
  add_component<Cmp::Persist::MaxNumCrypts>();
  add_component<Cmp::Persist::MusicVolume>();
  add_component<Cmp::Persist::NpcActivateScale>();
  add_component<Cmp::Persist::NpcDeathAnimFramerate>();
  add_component<Cmp::Persist::NpcPushBack>();
  add_component<Cmp::Persist::NpcShockwaveFreq>();
  add_component<Cmp::Persist::NpcShockwaveMaxRadius>();
  add_component<Cmp::Persist::NpcShockwaveResolution>();
  add_component<Cmp::Persist::NpcShockwaveSpeed>();
  add_component<Cmp::Persist::NpcWatchmanSpawnMax>();
  add_component<Cmp::Persist::NpcWatchmanSpawnCooldown>();
  add_component<Cmp::Persist::NpcWatchmanGunFireRate>();
  add_component<Cmp::Persist::NpcWatchmanGunReloadRate>();
  add_component<Cmp::Persist::NpcWatchmanSpawnInfamy>();
  add_component<Cmp::Persist::NpcWatchmanConeHalfAngle>();
  add_component<Cmp::Persist::NpcWatchmanConeLength>();
  add_component<Cmp::Persist::NpcWatchmanSweepSpeed>();
  add_component<Cmp::Persist::NpcWatchmanSweepAmplitude>();
  add_component<Cmp::Persist::NpcWatchmanIdleDirectionChangeInterval>();
  add_component<Cmp::Persist::PcDamageDelay>();
  add_component<Cmp::Persist::PlantBurnDuration>();
  add_component<Cmp::Persist::PlayerAnimStrideLength>();
  add_component<Cmp::Persist::PlayerDiagonalLerpSpeedModifier>();
  add_component<Cmp::Persist::PlayerFootstepAddDelay>();
  add_component<Cmp::Persist::PlayerFootstepFadeDelay>();
  add_component<Cmp::Persist::PlayerFootstepSfxStrideLength>();
  add_component<Cmp::Persist::PlayerLerpInterruptThreshold>();
  add_component<Cmp::Persist::PlayerMovementSpeed>();
  add_component<Cmp::Persist::PlayerShortcutLerpSpeedModifier>();
  add_component<Cmp::Persist::PlayerStartPosition>();
  add_component<Cmp::Persist::PostPullMovementDelay>();
  add_component<Cmp::Persist::RuinMaxCobwebs>();
  add_component<Cmp::Persist::RuinMaxSpiders>();
  add_component<Cmp::Persist::RuinProcGenInitChance>();
  add_component<Cmp::Persist::RuinProcGenMaxIterations>();
  add_component<Cmp::Persist::RuinProcGenBirthThreshold>();
  add_component<Cmp::Persist::RuinProcGenSurvivalThreshold>();
  add_component<Cmp::Persist::ShopMaxItems>();
  add_component<Cmp::Persist::ShopMinPrice>();
  add_component<Cmp::Persist::ShopMaxPrice>();
  add_component<Cmp::Persist::WeaponDegradePerHit>();
  add_component<Cmp::Persist::WormholeAnimFramerate>();

  // Add serdes functions to each component
  for ( const auto &[name, config] : definitions->items() )
  {
    if ( m_components.contains( name ) ) { m_components.at( name ).initialise( config ); }
    else { SPDLOG_WARN( "Unknown component type in definitions: {}", name ); }
  }

  // Add components not stored in JSON (runtime-only seeds)
  Sys::PersistSystem::add<Cmp::Persist::WormholeSeed>( reg(), 0 );
  Sys::PersistSystem::add<Cmp::Persist::SinkholeSeed>( reg(), 0 );
  Sys::PersistSystem::add<Cmp::Persist::CorruptionSeed>( reg(), 0 );
}

void PersistSystem::load_state()
{
  SPDLOG_DEBUG( "Loading persistent state..." );
  const auto jsonData = load_json_file( kPersistFilePath );
  if ( not jsonData ) { return; }

  for ( const auto &[key, value] : jsonData->items() )
  {
    if ( not m_components.contains( key ) )
    {
      SPDLOG_WARN( "Unknown component: {}", key );
      continue;
    }

    try
    {
      const auto &ops = m_components.at( key );
      if ( not ops.deserialiser )
      {
        throw std::runtime_error( "Persistent component " + key + " has not been initialised (no matching entry was present in " +
                                  std::string( kPersistFilePath ) + " at startup)" );
      }
      ops.deserialiser( value );
    } catch ( const std::exception &e )
    {
      SPDLOG_WARN( "Failed to load component {}: {}", key, e.what() );
    }
  }
}

void PersistSystem::save_state()
{
  SPDLOG_DEBUG( "Saving persistent state..." );
  nlohmann::json jsonData;

  // Use the component instances registered during initialize_component_registry()
  auto exclude = []( const auto &p ) { return p.first != "PlayerStartPosition"; };
  for ( const auto &[key, ops] : m_components | std::views::filter( exclude ) )
  {
    try
    {
      if ( not ops.serialiser )
      {
        throw std::runtime_error( "Persistent component " + key + " has not been initialised (no matching entry was present in " +
                                  std::string( kPersistFilePath ) + " at startup)" );
      }
      auto result = ops.serialiser();
      if ( not result.is_null() ) { jsonData[key] = result; }
    } catch ( const std::exception &e )
    {
      SPDLOG_WARN( "Failed to serialize component {}: {}", key, e.what() );
    }
  }

  std::ofstream outputFile( kPersistFilePath );
  if ( outputFile.is_open() )
  {
    outputFile << jsonData.dump( 4 );
    outputFile.close();
    SPDLOG_DEBUG( "Persistent state saved successfully" );
  }
  else { SPDLOG_ERROR( "Failed to open file for saving persistent state" ); }
}

} // namespace Game::Sys