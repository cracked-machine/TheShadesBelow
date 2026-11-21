

#include <Components/Persistent/AltarBloodQuota.hpp>
#include <Components/Persistent/AltarOfferingCooldown.hpp>
#include <Components/Persistent/AltarSummonOfferingCooldown.hpp>
#include <Components/Persistent/ArmedBlinkFreq.hpp>
#include <Components/Persistent/ArmedBlockColourBorder.hpp>
#include <Components/Persistent/ArmedBlockColourFill.hpp>
#include <Components/Persistent/ArmedOffDelay.hpp>
#include <Components/Persistent/ArmedOnDelay.hpp>
#include <Components/Persistent/MeleeHitCooldown.hpp>
#include <Components/Persistent/NpcDamageTextOffset.hpp>
#include <Components/Persistent/ShowDamageText.hpp>
#include <Components/Persistent/ShowHealthBar.hpp>
#include <Components/Persistent/AzazelFireballArc.hpp>
#include <Components/Persistent/AzazelFireballColour.hpp>
#include <Components/Persistent/AzazelFireballCooldown.hpp>
#include <Components/Persistent/AzazelFireballDamage.hpp>
#include <Components/Persistent/AzazelFireballSpeed.hpp>
#include <Components/Persistent/BloodMinChance.hpp>
#include <Components/Persistent/BombBonus.hpp>
#include <Components/Persistent/BombDamage.hpp>
#include <Components/Persistent/BombsExhumeGraves.hpp>
#include <Components/Persistent/BowReleaseWear.hpp>
#include <Components/Persistent/BradycardiaHealthDmgPerSec.hpp>
#include <Components/Persistent/BradycardiaMovementSpeed.hpp>
#include <Components/Persistent/BurrExpiry.hpp>
#include <Components/Persistent/BurrSlowFactor.hpp>
#include <Components/Persistent/BurrThrowCooldown.hpp>
#include <Components/Persistent/CameraSmoothSpeed.hpp>
#include <Components/Persistent/CorruptionDamage.hpp>
#include <Components/Persistent/CorruptionSpreadPeriod.hpp>
#include <Components/Persistent/CorruptionSeed.hpp>
#include <Components/Persistent/CryptNpcSpawnCount.hpp>
#include <Components/Persistent/CryptShuffleTimeout.hpp>
#include <Components/Persistent/CryptVariantOverride.hpp>
#include <Components/Persistent/DespairHealthDmgPerSec.hpp>
#include <Components/Persistent/DiggingCooldownThreshold.hpp>
#include <Components/Persistent/DiggingDamagePerHit.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Persistent/EffectsVolume.hpp>
#include <Components/Persistent/EmberDamage.hpp>
#include <Components/Persistent/ExitKeyRequirement.hpp>
#include <Components/Persistent/FearHealthDmgPerSec.hpp>
#include <Components/Persistent/FlyingLandingOverlap.hpp>
#include <Components/Persistent/FlyingLiftOffset.hpp>
#include <Components/Persistent/FlyingOscillationAmplitude.hpp>
#include <Components/Persistent/FlyingTransitionTime.hpp>
#include <Components/Persistent/FuseDelay.hpp>
#include <Components/Persistent/GlovesWearPerForage.hpp>
#include <Components/Persistent/GraveClusterExternalPadding.hpp>
#include <Components/Persistent/GrassSlowFactor.hpp>
#include <Components/Persistent/GrassSpawnChance.hpp>
#include <Components/Persistent/GraveClusterGridSize.hpp>
#include <Components/Persistent/GraveNumMultiplier.hpp>
#include <Components/Persistent/GraveOddsBlood.hpp>
#include <Components/Persistent/GraveOddsBombTrap.hpp>
#include <Components/Persistent/GraveOddsCurseTablet.hpp>
#include <Components/Persistent/GraveOddsNpcTrap.hpp>
#include <Components/Persistent/GraveOddsTalisman.hpp>
#include <Components/Persistent/GraveOddsTorch.hpp>
#include <Components/Persistent/GraveyardProcGenBirthThreshold.hpp>
#include <Components/Persistent/GraveyardProcGenInitChance.hpp>
#include <Components/Persistent/GraveyardProcGenMaxIterations.hpp>
#include <Components/Persistent/GraveyardProcGenSurvivalThreshold.hpp>
#include <Components/Persistent/GrimoireHoverFollowsMouse.hpp>
#include <Components/Persistent/HallucinogenHealthDmgPerSec.hpp>
#include <Components/Persistent/HazardBalanceOverlap.hpp>
#include <Components/Persistent/HazardBalanceTimeout.hpp>
#include <Components/Persistent/HeldItemOffset.hpp>
#include <Components/Persistent/HeldItemRotation.hpp>
#include <Components/Persistent/HeldItemRotationSouth.hpp>
#include <Components/Persistent/HeldItemRotationWest.hpp>
#include <Components/Persistent/HeldItemScale.hpp>
#include <Components/Persistent/HeldItemSideOffsetX.hpp>
#include <Components/Persistent/HypoxiaHealthDmgPerSec.hpp>
#include <Components/Persistent/InventorySlotCount.hpp>
#include <Components/Persistent/InventoryStackSize.hpp>
#include <Components/Persistent/InventorySlotRollover.hpp>
#include <Components/Persistent/InventorySwapCooldown.hpp>
#include <Components/Persistent/LavaFireballDamage.hpp>
#include <Components/Persistent/LightningDamage.hpp>
#include <Components/Persistent/LightningRodDebrisDamage.hpp>
#include <Components/Persistent/LuckDropRate.hpp>
#include <Components/Persistent/MaxLevelDepth.hpp>
#include <Components/Persistent/MusicVolume.hpp>
#include <Components/Persistent/NpcActivateScale.hpp>
#include <Components/Persistent/NpcCrowFlightSpeed.hpp>
#include <Components/Persistent/NpcCrowGuideApproachDistance.hpp>
#include <Components/Persistent/NpcCrowMaxPerchWait.hpp>
#include <Components/Persistent/NpcCrowPerchRange.hpp>
#include <Components/Persistent/NpcCrowPerchSpeed.hpp>
#include <Components/Persistent/NpcMimicTint.hpp>
#include <Components/Persistent/NpcPushBack.hpp>
#include <Components/Persistent/NpcRenderOffset.hpp>
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
#include <Components/Persistent/PaidHealAmount.hpp>
#include <Components/Persistent/PlantBurnTime.hpp>
#include <Components/Persistent/PlantWearPerForage.hpp>
#include <Components/Persistent/PlayerAnimStrideLength.hpp>
#include <Components/Persistent/PlayerCornerAssistTolerance.hpp>
#include <Components/Persistent/PlayerDiagonalGapCatch.hpp>
#include <Components/Persistent/PlayerDiagonalLerpSpeedModifier.hpp>
#include <Components/Persistent/PlayerFootstepAddDelay.hpp>
#include <Components/Persistent/PlayerFootstepFadeDelay.hpp>
#include <Components/Persistent/PlayerFootstepSfxStrideLength.hpp>
#include <Components/Persistent/PlayerLerpInterruptThreshold.hpp>
#include <Components/Persistent/PlayerMovementSpeed.hpp>
#include <Components/Persistent/PlayerOnFireHealthDmgPerSec.hpp>
#include <Components/Persistent/PlayerRenderOffset.hpp>
#include <Components/Persistent/PlayerShortcutLerpSpeedModifier.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Persistent/PostPullMovementDelay.hpp>
#include <Components/Persistent/RepelNpcPlayerTint.hpp>
#include <Components/Persistent/ReservedArmedBlockColourBorder.hpp>
#include <Components/Persistent/ReservedArmedBlockColourFill.hpp>
#include <Components/Persistent/ResetTutorial.hpp>
#include <Components/Persistent/RuinMaxCobwebs.hpp>
#include <Components/Persistent/RuinMaxSpiders.hpp>
#include <Components/Persistent/RuinProcGenBirthThreshold.hpp>
#include <Components/Persistent/RuinProcGenInitChance.hpp>
#include <Components/Persistent/RuinProcGenMaxIterations.hpp>
#include <Components/Persistent/RuinProcGenSurvivalThreshold.hpp>
#include <Components/Persistent/ShopMaxItems.hpp>
#include <Components/Persistent/ShopMaxPrice.hpp>
#include <Components/Persistent/ShopMinPrice.hpp>
#include <Components/Persistent/ShovelGraveDigWear.hpp>
#include <Components/Persistent/ShovelObstacleDigWear.hpp>
#include <Components/Persistent/ShovelPotSmashWear.hpp>
#include <Components/Persistent/SinkholeSeed.hpp>
#include <Components/Persistent/SinkholeSpreadPeriod.hpp>
#include <Components/Persistent/StartingDespair.hpp>
#include <Components/Persistent/StartingFear.hpp>
#include <Components/Persistent/StartingHealth.hpp>
#include <Components/Persistent/StartingInfamy.hpp>
#include <Components/Persistent/StartingLuck.hpp>
#include <Components/Persistent/StatChangeDisplayDuration.hpp>
#include <Components/Persistent/StowedTorchLit.hpp>
#include <Components/Persistent/TachycardiaHealthDmgPerSec.hpp>
#include <Components/Persistent/TachycardiaMovementSpeed.hpp>
#include <Components/Persistent/TooltipsSeen.hpp>
#include <Components/Persistent/ToxicImmunityMeterFill.hpp>
#include <Components/Persistent/ToxicImmunityMeterShimmer.hpp>
#include <Components/Persistent/ToxicImmunityMeterTint.hpp>
#include <Components/Persistent/VenomHealthDmgPerSec.hpp>
#include <Components/Persistent/VertigoHealthDmgPerSec.hpp>
#include <Components/Persistent/WormholeAbortThreshold.hpp>
#include <Components/Persistent/WormholeSeed.hpp>
#include <Events/LoadSettingsEvent.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>

#include <array>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <utility>

namespace Game::Sys
{

PersistSystem::PersistSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
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
  add_component<Cmp::Persist::ReservedArmedBlockColourFill>();
  add_component<Cmp::Persist::ReservedArmedBlockColourBorder>();
  add_component<Cmp::Persist::ArmedOffDelay>();
  add_component<Cmp::Persist::ArmedOnDelay>();
  add_component<Cmp::Persist::BombBonus>();
  add_component<Cmp::Persist::BombDamage>();
  add_component<Cmp::Persist::BombsExhumeGraves>();
  add_component<Cmp::Persist::CameraSmoothSpeed>();
  add_component<Cmp::Persist::HeldItemOffset>();
  add_component<Cmp::Persist::HeldItemRotation>();
  add_component<Cmp::Persist::HeldItemRotationWest>();
  add_component<Cmp::Persist::HeldItemRotationSouth>();
  add_component<Cmp::Persist::HeldItemScale>();
  add_component<Cmp::Persist::HeldItemSideOffsetX>();
  add_component<Cmp::Persist::CorruptionDamage>();
  add_component<Cmp::Persist::CorruptionSpreadPeriod>();
  add_component<Cmp::Persist::SinkholeSpreadPeriod>();
  add_component<Cmp::Persist::CryptShuffleTimeout>();
  add_component<Cmp::Persist::CryptNpcSpawnCount>();
  add_component<Cmp::Persist::CryptVariantOverride>();
  add_component<Cmp::Persist::DiggingCooldownThreshold>();
  add_component<Cmp::Persist::DiggingDamagePerHit>();
  add_component<Cmp::Persist::DisplayResolution>();
  add_component<Cmp::Persist::EmberDamage>();
  add_component<Cmp::Persist::LightningRodDebrisDamage>();
  add_component<Cmp::Persist::EffectsVolume>();
  add_component<Cmp::Persist::ExitKeyRequirement>();
  add_component<Cmp::Persist::FuseDelay>();
  add_component<Cmp::Persist::GraveNumMultiplier>();
  add_component<Cmp::Persist::MaxLevelDepth>();
  add_component<Cmp::Persist::BloodMinChance>();
  add_component<Cmp::Persist::GraveOddsCurseTablet>();
  add_component<Cmp::Persist::GraveOddsBombTrap>();
  add_component<Cmp::Persist::GraveOddsTorch>();
  add_component<Cmp::Persist::GraveOddsNpcTrap>();
  add_component<Cmp::Persist::GraveOddsBlood>();
  add_component<Cmp::Persist::GraveOddsTalisman>();
  add_component<Cmp::Persist::GraveClusterGridSize>();
  add_component<Cmp::Persist::GraveClusterExternalPadding>();
  add_component<Cmp::Persist::GraveyardProcGenInitChance>();
  add_component<Cmp::Persist::GraveyardProcGenMaxIterations>();
  add_component<Cmp::Persist::GraveyardProcGenBirthThreshold>();
  add_component<Cmp::Persist::GraveyardProcGenSurvivalThreshold>();
  add_component<Cmp::Persist::HazardBalanceOverlap>();
  add_component<Cmp::Persist::HazardBalanceTimeout>();
  add_component<Cmp::Persist::LavaFireballDamage>();
  add_component<Cmp::Persist::AzazelFireballDamage>();
  add_component<Cmp::Persist::AzazelFireballCooldown>();
  add_component<Cmp::Persist::AzazelFireballSpeed>();
  add_component<Cmp::Persist::AzazelFireballArc>();
  add_component<Cmp::Persist::AzazelFireballColour>();
  add_component<Cmp::Persist::AltarBloodQuota>();
  add_component<Cmp::Persist::AltarOfferingCooldown>();
  add_component<Cmp::Persist::AltarSummonOfferingCooldown>();
  add_component<Cmp::Persist::LightningDamage>();
  add_component<Cmp::Persist::PaidHealAmount>();
  add_component<Cmp::Persist::MusicVolume>();
  add_component<Cmp::Persist::NpcActivateScale>();
  add_component<Cmp::Persist::NpcCrowFlightSpeed>();
  add_component<Cmp::Persist::NpcCrowPerchSpeed>();
  add_component<Cmp::Persist::NpcCrowPerchRange>();
  add_component<Cmp::Persist::NpcCrowMaxPerchWait>();
  add_component<Cmp::Persist::NpcCrowGuideApproachDistance>();
  add_component<Cmp::Persist::NpcMimicTint>();
  add_component<Cmp::Persist::NpcRenderOffset>();
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
  add_component<Cmp::Persist::PlayerAnimStrideLength>();
  add_component<Cmp::Persist::PlayerDiagonalLerpSpeedModifier>();
  add_component<Cmp::Persist::PlayerFootstepAddDelay>();
  add_component<Cmp::Persist::PlayerFootstepFadeDelay>();
  add_component<Cmp::Persist::PlayerFootstepSfxStrideLength>();
  add_component<Cmp::Persist::PlayerCornerAssistTolerance>();
  add_component<Cmp::Persist::PlayerDiagonalGapCatch>();
  add_component<Cmp::Persist::PlayerLerpInterruptThreshold>();
  add_component<Cmp::Persist::PlayerMovementSpeed>();
  add_component<Cmp::Persist::TachycardiaMovementSpeed>();
  add_component<Cmp::Persist::BradycardiaMovementSpeed>();
  add_component<Cmp::Persist::PlayerShortcutLerpSpeedModifier>();
  add_component<Cmp::Persist::PlayerStartPosition>();
  add_component<Cmp::Persist::PostPullMovementDelay>();
  add_component<Cmp::Persist::RepelNpcPlayerTint>();
  add_component<Cmp::Persist::RuinMaxCobwebs>();
  add_component<Cmp::Persist::RuinMaxSpiders>();
  add_component<Cmp::Persist::RuinProcGenInitChance>();
  add_component<Cmp::Persist::RuinProcGenMaxIterations>();
  add_component<Cmp::Persist::RuinProcGenBirthThreshold>();
  add_component<Cmp::Persist::RuinProcGenSurvivalThreshold>();
  add_component<Cmp::Persist::ShopMaxItems>();
  add_component<Cmp::Persist::ShopMinPrice>();
  add_component<Cmp::Persist::ShopMaxPrice>();
  add_component<Cmp::Persist::MeleeHitCooldown>();
  add_component<Cmp::Persist::FearHealthDmgPerSec>();
  add_component<Cmp::Persist::DespairHealthDmgPerSec>();
  add_component<Cmp::Persist::HypoxiaHealthDmgPerSec>();
  add_component<Cmp::Persist::TachycardiaHealthDmgPerSec>();
  add_component<Cmp::Persist::ToxicImmunityMeterFill>();
  add_component<Cmp::Persist::ToxicImmunityMeterShimmer>();
  add_component<Cmp::Persist::ToxicImmunityMeterTint>();
  add_component<Cmp::Persist::StatChangeDisplayDuration>();
  add_component<Cmp::Persist::StartingHealth>();
  add_component<Cmp::Persist::StartingFear>();
  add_component<Cmp::Persist::StartingDespair>();
  add_component<Cmp::Persist::StartingInfamy>();
  add_component<Cmp::Persist::StartingLuck>();
  add_component<Cmp::Persist::LuckDropRate>();
  add_component<Cmp::Persist::BradycardiaHealthDmgPerSec>();
  add_component<Cmp::Persist::VenomHealthDmgPerSec>();
  add_component<Cmp::Persist::VertigoHealthDmgPerSec>();
  add_component<Cmp::Persist::HallucinogenHealthDmgPerSec>();
  add_component<Cmp::Persist::PlayerOnFireHealthDmgPerSec>();
  add_component<Cmp::Persist::BowReleaseWear>();
  add_component<Cmp::Persist::BurrExpiry>();
  add_component<Cmp::Persist::BurrSlowFactor>();
  add_component<Cmp::Persist::BurrThrowCooldown>();
  add_component<Cmp::Persist::InventorySwapCooldown>();
  add_component<Cmp::Persist::InventorySlotCount>();
  add_component<Cmp::Persist::InventoryStackSize>();
  add_component<Cmp::Persist::StowedTorchLit>();
  add_component<Cmp::Persist::InventorySlotRollover>();
  add_component<Cmp::Persist::ShowHealthBar>();
  add_component<Cmp::Persist::ShowDamageText>();
  add_component<Cmp::Persist::NpcDamageTextOffset>();
  add_component<Cmp::Persist::FlyingOscillationAmplitude>();
  add_component<Cmp::Persist::FlyingLiftOffset>();
  add_component<Cmp::Persist::PlayerRenderOffset>();
  add_component<Cmp::Persist::FlyingTransitionTime>();
  add_component<Cmp::Persist::FlyingLandingOverlap>();
  add_component<Cmp::Persist::WormholeAbortThreshold>();
  add_component<Cmp::Persist::PlantBurnTime>();
  add_component<Cmp::Persist::GrassSlowFactor>();
  add_component<Cmp::Persist::GrassSpawnChance>();
  add_component<Cmp::Persist::PlantWearPerForage>();
  add_component<Cmp::Persist::GlovesWearPerForage>();
  add_component<Cmp::Persist::ShovelGraveDigWear>();
  add_component<Cmp::Persist::ShovelObstacleDigWear>();
  add_component<Cmp::Persist::ShovelPotSmashWear>();
  add_component<Cmp::Persist::TooltipsSeen>();
  add_component<Cmp::Persist::ResetTutorial>();
  add_component<Cmp::Persist::GrimoireHoverFollowsMouse>();

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

  write_json_file( kPersistFilePath, jsonData );
}

/* static */ void PersistSystem::write_json_file( const std::string &path, const nlohmann::json &json_data )
{
  std::ofstream outputFile( path );
  if ( outputFile.is_open() )
  {
    outputFile << json_data.dump( 4 );
    outputFile.close();
    SPDLOG_DEBUG( "Persistent state saved successfully" );
  }
  else { SPDLOG_ERROR( "Failed to open {} for saving persistent state", path ); }
}

} // namespace Game::Sys