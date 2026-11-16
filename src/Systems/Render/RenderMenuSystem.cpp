

#include <Components/Persistent/AltarActivationAnimaCost.hpp>
#include <Components/Persistent/AltarOfferingCooldown.hpp>
#include <Components/Persistent/AltarSummonOfferingCooldown.hpp>
#include <Components/Persistent/ArmedBlinkFreq.hpp>
#include <Components/Persistent/ArmedBlockColourBorder.hpp>
#include <Components/Persistent/ArmedBlockColourFill.hpp>
#include <Components/Persistent/ArmedOffDelay.hpp>
#include <Components/Persistent/ArmedOnDelay.hpp>
#include <Components/Persistent/ArtefactMinChance.hpp>
#include <Components/Persistent/AutoPickupDrops.hpp>
#include <Components/Persistent/AxeGraveDigWear.hpp>
#include <Components/Persistent/AxeObstacleDigWear.hpp>
#include <Components/Persistent/AxePotSmashWear.hpp>
#include <Components/Persistent/AxeSkeletonKillWear.hpp>
#include <Components/Persistent/AzazelFireballArc.hpp>
#include <Components/Persistent/AzazelFireballColour.hpp>
#include <Components/Persistent/AzazelFireballCooldown.hpp>
#include <Components/Persistent/AzazelFireballDamage.hpp>
#include <Components/Persistent/AzazelFireballSpeed.hpp>
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
#include <Components/Persistent/GraveCountMargin.hpp>
#include <Components/Persistent/GraveNumMultiplier.hpp>
#include <Components/Persistent/GraveOddsArtefact.hpp>
#include <Components/Persistent/GraveOddsBombTrap.hpp>
#include <Components/Persistent/GraveOddsCurseTablet.hpp>
#include <Components/Persistent/GraveOddsNpcTrap.hpp>
#include <Components/Persistent/GraveOddsTalisman.hpp>
#include <Components/Persistent/GraveOddsTorch.hpp>
#include <Components/Persistent/GraveTrailGuideColour.hpp>
#include <Components/Persistent/GraveTrailNominalHopCount.hpp>
#include <Components/Persistent/GraveTrailNominalHopDistance.hpp>
#include <Components/Persistent/GraveTrailStartCount.hpp>
#include <Components/Persistent/GraveyardProcGenBirthThreshold.hpp>
#include <Components/Persistent/GraveyardProcGenInitChance.hpp>
#include <Components/Persistent/GraveyardProcGenMaxIterations.hpp>
#include <Components/Persistent/GraveyardProcGenSurvivalThreshold.hpp>
#include <Components/Persistent/GrimoireHoverFollowsMouse.hpp>
#include <Components/Persistent/HallucinogenHealthDmgPerSec.hpp>
#include <Components/Persistent/HazardBalanceOverlap.hpp>
#include <Components/Persistent/HazardBalanceTimeout.hpp>
#include <Components/Persistent/HealthBonus.hpp>
#include <Components/Persistent/HeldItemOffset.hpp>
#include <Components/Persistent/HeldItemRotation.hpp>
#include <Components/Persistent/HeldItemRotationSouth.hpp>
#include <Components/Persistent/HeldItemRotationWest.hpp>
#include <Components/Persistent/HeldItemScale.hpp>
#include <Components/Persistent/HeldItemSideOffsetX.hpp>
#include <Components/Persistent/HypoxiaHealthDmgPerSec.hpp>
#include <Components/Persistent/InventorySlotCount.hpp>
#include <Components/Persistent/InventorySlotRollover.hpp>
#include <Components/Persistent/InventorySwapCooldown.hpp>
#include <Components/Persistent/LavaFireballDamage.hpp>
#include <Components/Persistent/LightningDamage.hpp>
#include <Components/Persistent/LightningRodDebrisDamage.hpp>
#include <Components/Persistent/LuckDropRate.hpp>
#include <Components/Persistent/MaxLevelDepth.hpp>
#include <Components/Persistent/HealingBuildingOverride.hpp>
#include <Components/Persistent/PaidHealAmount.hpp>
#include <Components/Persistent/MinNumRuins.hpp>
#include <Components/Persistent/MusicVolume.hpp>
#include <Components/Persistent/NpcActivateScale.hpp>
#include <Components/Persistent/NpcCrowFlightSpeed.hpp>
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
#include <Components/Persistent/NumAltars.hpp>
#include <Components/Persistent/PickaxeGraveDigWear.hpp>
#include <Components/Persistent/PickaxeObstacleDigWear.hpp>
#include <Components/Persistent/PickaxePotSmashWear.hpp>
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
#include <Components/Persistent/PostPullMovementDelay.hpp>
#include <Components/Persistent/RepelNpcPlayerTint.hpp>
#include <Components/Persistent/ReservedArmedBlockColourBorder.hpp>
#include <Components/Persistent/ReservedArmedBlockColourFill.hpp>
#include <Components/Persistent/ResetTutorial.hpp>
#include <Components/Persistent/RetainAnimaBetweenLevels.hpp>
#include <Components/Persistent/RuinMaxCobwebs.hpp>
#include <Components/Persistent/RuinMaxSpiders.hpp>
#include <Components/Persistent/RuinProcGenBirthThreshold.hpp>
#include <Components/Persistent/RuinProcGenInitChance.hpp>
#include <Components/Persistent/RuinProcGenMaxIterations.hpp>
#include <Components/Persistent/RuinProcGenSurvivalThreshold.hpp>
#include <Components/Persistent/ShovelGraveDigWear.hpp>
#include <Components/Persistent/ShovelObstacleDigWear.hpp>
#include <Components/Persistent/ShovelPotSmashWear.hpp>
#include <Components/Persistent/SinkholeSeed.hpp>
#include <Components/Persistent/StartingAnima.hpp>
#include <Components/Persistent/StartingDespair.hpp>
#include <Components/Persistent/StartingFear.hpp>
#include <Components/Persistent/StartingHealth.hpp>
#include <Components/Persistent/StartingInfamy.hpp>
#include <Components/Persistent/StartingLuck.hpp>
#include <Components/Persistent/StatChangeDisplayDuration.hpp>
#include <Components/Persistent/StowedTorchLit.hpp>
#include <Components/Persistent/TachycardiaHealthDmgPerSec.hpp>
#include <Components/Persistent/TachycardiaMovementSpeed.hpp>
#include <Components/Persistent/ToxicImmunityMeterFill.hpp>
#include <Components/Persistent/ToxicImmunityMeterShimmer.hpp>
#include <Components/Persistent/ToxicImmunityMeterTint.hpp>
#include <Components/Persistent/VenomHealthDmgPerSec.hpp>
#include <Components/Persistent/VertigoHealthDmgPerSec.hpp>
#include <Components/Persistent/WormholeAbortThreshold.hpp>
#include <Components/Persistent/WormholeSeed.hpp>
#include <Components/Player/Cadaver.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/LevelDepth.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Shaders/BaseShaderSprite.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderMenuSystem.hpp>
#include <Systems/ShaderSystem.hpp>
#include <Systems/Threats/BombSystem.hpp>
#include <Utils/ImGuiTheme.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>

#include "imgui-SFML.h"
#include <format>
#include <imgui.h>

namespace Game::Sys
{

RenderMenuSystem::~RenderMenuSystem() = default;

void RenderMenuSystem::render_title()
{
  {
    auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() ) );

    // shaders
    auto shader_view = reg().view<Cmp::Shader::SpriteOwner>();
    for ( auto [entt, shader] : shader_view.each() )
    {
      if ( not shader.sprite ) continue;
      m_window.draw( *shader.sprite );
    }

    // text
    sf::Color txt_color{ 64, 96, 184 };
    sf::Color txt_outline_color{ 48, 48, 48 };

    render_text( "The Shades Below", static_cast<int>( display_size.x / 20 ), { display_size.x * 0.25f, display_size.y * 0.25f }, Alignment::CENTER,
                 2.f, txt_outline_color );

    render_text( "Press <Enter> key to start", static_cast<int>( display_size.x / 40 ), { display_size.x * 0.25f, display_size.y * 0.5f },
                 Alignment::CENTER, 3.f, txt_color );

    render_text( "Press <Q> key to quit", static_cast<int>( display_size.x / 40 ), { display_size.x * 0.25f, ( display_size.y * 0.5f ) + 100.f },
                 Alignment::CENTER, 3.f, txt_color );

    render_text( "Press <S> key for settings", static_cast<int>( display_size.x / 40 ), { display_size.x * 0.25f, ( display_size.y * 0.5f ) + 200.f },
                 Alignment::CENTER, 3.f, txt_color );
  }
}

void RenderMenuSystem::render_settings( sf::Time dt )
{
  sf::Vector2u display_size = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
  sf::Text title_text( m_font, "Settings", display_size.x / 20 );
  title_text.setFillColor( sf::Color::White );
  title_text.setPosition( { 10.f, 10.f } );
  m_window.draw( title_text );

  sf::Text restore_text( m_font, "Press <U> key to undo changes", display_size.x / 40 );
  restore_text.setFillColor( sf::Color::White );
  auto restore_right_align_px = static_cast<float>( display_size.x ) - restore_text.getGlobalBounds().size.x - 10.f;
  restore_text.setPosition( { restore_right_align_px, 30.f } );
  m_window.draw( restore_text );

  sf::Text exit_text( m_font, "Press <Esc> key to go back", display_size.x / 40 );
  exit_text.setFillColor( sf::Color::White );
  auto exit_right_align_px = static_cast<float>( display_size.x ) - exit_text.getGlobalBounds().size.x - 10.f;
  exit_text.setPosition( { exit_right_align_px, 80.f } );
  m_window.draw( exit_text );

  // ImGUI should be rendered last, on top of the SFML draws above
  render_settings_widgets( dt, title_text.getLocalBounds() );
}

const std::vector<sf::Vector2u> RenderMenuSystem::DisplaySettings::resolutions = { { 1920, 1080 }, { 1680, 1050 }, { 1600, 900 }, { 1440, 900 },
                                                                                   { 1366, 768 },  { 1280, 720 },  { 1024, 768 }, { 800, 600 } };

void RenderMenuSystem::render_settings_widgets( sf::Time dt, sf::FloatRect title_text_dimensions )
{
  auto padding_px = 10.f;
  bool resolution_change_pending = false;
  sf::Vector2u pending_resolution;

  // need to make sure we call Update() and Render() every frame
  ImGui::SFML::Update( m_window, dt );
  ImGui::Begin( "Settings", nullptr, kImGuiWindowOptions );
  try
  {
    // keep imgui window in sync with display resolution updates
    auto &display_resolution = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
    const float kFontScaleFactor = 1280.f;

    ImVec2 window_size = ImVec2( static_cast<float>( display_resolution.x ) - padding_px,
                                 static_cast<float>( display_resolution.y ) - ( title_text_dimensions.size.y * 2 ) - padding_px );
    ImGui::SetWindowSize( "Settings", window_size );
    ImGui::SetWindowPos( "Settings", ImVec2( padding_px, title_text_dimensions.size.y * 2 ) );
    ImGui::SetWindowFontScale( static_cast<float>( display_resolution.x ) / kFontScaleFactor );

    static int current_resolution = 0; // Default to first item

    // Capture resolutions by reference in the lambda
    if ( ImGui::Combo( "Display Resolution", &current_resolution, DisplaySettings::get, nullptr,
                       static_cast<int>( DisplaySettings::resolutions.size() ) ) )
    {
      // Don't recreate the window here: we're still mid-ImGui-frame (inside Begin/End,
      // before SFML::Render). Recreating it now tears down the GL context that ImGui-SFML's
      // font atlas/device objects are bound to without telling the backend, so the rest of
      // this frame - and every frame after it - renders with stale/dangling GL handles.
      // Defer the actual window recreation until after the frame has been rendered and
      // reinitialize ImGui-SFML against the new context (see below).
      resolution_change_pending = true;
      pending_resolution = DisplaySettings::resolutions[current_resolution];
    }

    ImGui::SeparatorText( "Player Settings" );

    Sys::PersistSystem::get<Cmp::Persist::PlayerAnimStrideLength>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerRenderOffset>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HeldItemOffset>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HeldItemSideOffsetX>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HeldItemRotation>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HeldItemRotationWest>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HeldItemRotationSouth>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HeldItemScale>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::InventorySwapCooldown>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::InventorySlotCount>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::InventorySlotRollover>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AutoPickupDrops>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::StowedTorchLit>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerMovementSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::TachycardiaMovementSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::BradycardiaMovementSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerDiagonalLerpSpeedModifier>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerFootstepAddDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerFootstepFadeDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerFootstepSfxStrideLength>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerShortcutLerpSpeedModifier>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::DiggingCooldownThreshold>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::DiggingDamagePerHit>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::CameraSmoothSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerCornerAssistTolerance>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerDiagonalGapCatch>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerLerpInterruptThreshold>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PostPullMovementDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::RetainAnimaBetweenLevels>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::LuckDropRate>( reg() ).render_widget();

    ImGui::TextUnformatted( "Starting Stats" );
    ImGui::TextDisabled( "applied at the start of every level" );
    {
      const std::array<Cmp::Persist::BasePersistent<int> *, 6> starting_stats{
          &Sys::PersistSystem::get<Cmp::Persist::StartingHealth>( reg() ),  &Sys::PersistSystem::get<Cmp::Persist::StartingFear>( reg() ),
          &Sys::PersistSystem::get<Cmp::Persist::StartingDespair>( reg() ), &Sys::PersistSystem::get<Cmp::Persist::StartingInfamy>( reg() ),
          &Sys::PersistSystem::get<Cmp::Persist::StartingLuck>( reg() ),    &Sys::PersistSystem::get<Cmp::Persist::StartingAnima>( reg() ),
      };
      render_vslider_row( starting_stats );
    }

    ImGui::SeparatorText( "Item Effect Settings" );

    Sys::PersistSystem::get<Cmp::Persist::RepelNpcPlayerTint>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ToxicImmunityMeterFill>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ToxicImmunityMeterTint>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ToxicImmunityMeterShimmer>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::StatChangeDisplayDuration>( reg() ).render_widget();

    ImGui::SeparatorText( "Weapon Wear Settings" );

    Sys::PersistSystem::get<Cmp::Persist::AxeObstacleDigWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PickaxeObstacleDigWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ShovelObstacleDigWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AxePotSmashWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PickaxePotSmashWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ShovelPotSmashWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AxeSkeletonKillWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AxeGraveDigWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PickaxeGraveDigWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ShovelGraveDigWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::BowReleaseWear>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlantWearPerForage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::GlovesWearPerForage>( reg() ).render_widget();

    ImGui::SeparatorText( "Burr Settings" );

    Sys::PersistSystem::get<Cmp::Persist::BurrExpiry>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::BurrSlowFactor>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::BurrThrowCooldown>( reg() ).render_widget();

    ImGui::SeparatorText( "Broom Flight Settings" );

    Sys::PersistSystem::get<Cmp::Persist::FlyingLiftOffset>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::FlyingOscillationAmplitude>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::FlyingTransitionTime>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::FlyingLandingOverlap>( reg() ).render_widget();

    // Bomb Settings
    ImGui::SeparatorText( "Bomb Settings" );

    Sys::PersistSystem::get<Cmp::Persist::BombDamage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::FuseDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::BombsExhumeGraves>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArmedOnDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArmedOffDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArmedBlockColourBorder>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArmedBlockColourFill>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ReservedArmedBlockColourBorder>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ReservedArmedBlockColourFill>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArmedBlinkFreq>( reg() ).render_widget();

    // Hazard Settings
    ImGui::SeparatorText( "Hazard Settings" );

    Sys::PersistSystem::get<Cmp::Persist::CorruptionDamage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HazardBalanceOverlap>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HazardBalanceTimeout>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::LightningDamage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::LavaFireballDamage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AzazelFireballDamage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AzazelFireballCooldown>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AzazelFireballSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AzazelFireballArc>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AzazelFireballColour>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AltarActivationAnimaCost>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AltarOfferingCooldown>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::AltarSummonOfferingCooldown>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::EmberDamage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::LightningRodDebrisDamage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::WormholeSeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::WormholeAbortThreshold>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::CorruptionSeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::SinkholeSeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::RuinMaxCobwebs>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::RuinMaxSpiders>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::CryptShuffleTimeout>( reg() ).render_widget();

    // Player Damage Settings
    ImGui::SeparatorText( "Player Damage Settings" );

    Sys::PersistSystem::get<Cmp::Persist::FearHealthDmgPerSec>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::DespairHealthDmgPerSec>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HypoxiaHealthDmgPerSec>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::TachycardiaHealthDmgPerSec>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::BradycardiaHealthDmgPerSec>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::VenomHealthDmgPerSec>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::VertigoHealthDmgPerSec>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HallucinogenHealthDmgPerSec>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerOnFireHealthDmgPerSec>( reg() ).render_widget();

    // Loot Settings
    ImGui::SeparatorText( "Loot Settings" );

    Sys::PersistSystem::get<Cmp::Persist::HealthBonus>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::BombBonus>( reg() ).render_widget();

    // NPC Settings
    ImGui::SeparatorText( "NPC Settings" );

    Sys::PersistSystem::get<Cmp::Persist::CryptNpcSpawnCount>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::CryptVariantOverride>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcShockwaveResolution>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcShockwaveSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcShockwaveFreq>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcShockwaveMaxRadius>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanSpawnMax>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanSpawnCooldown>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanGunFireRate>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanGunReloadRate>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanSpawnInfamy>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanConeHalfAngle>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanConeLength>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanSweepSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanSweepAmplitude>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcWatchmanIdleDirectionChangeInterval>( reg() ).render_widget();

    auto &npc_push_back = Sys::PersistSystem::get<Cmp::Persist::NpcPushBack>( reg() );
    if ( ImGui::SliderScalar( "NPC Push Back Distance", ImGuiDataType_Float, npc_push_back.get_value_ptr(), npc_push_back.get_min_value_ptr(),
                              npc_push_back.get_max_value_ptr(), "%.1f pixels" ) )
    {
      float step = Constants::kGridSizePxF.x;
      npc_push_back.get_value() = roundf( npc_push_back.get_value() / step ) * step;
    }

    Sys::PersistSystem::get<Cmp::Persist::NpcActivateScale>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcMimicTint>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcRenderOffset>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcCrowFlightSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcCrowPerchSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcCrowPerchRange>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcCrowMaxPerchWait>( reg() ).render_widget();

    // Audio
    ImGui::SeparatorText( "Audio" );

    Sys::PersistSystem::get<Cmp::Persist::MusicVolume>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::EffectsVolume>( reg() ).render_widget();

    ImGui::SeparatorText( "Interface" );

    Sys::PersistSystem::get<Cmp::Persist::GrimoireHoverFollowsMouse>( reg() ).render_widget();

    ImGui::SeparatorText( "Tutorial" );

    Sys::PersistSystem::get<Cmp::Persist::ResetTutorial>( reg() ).render_widget();

    // Procedural Generation
    ImGui::SeparatorText( "Procedural Generation" );

    Sys::PersistSystem::get<Cmp::Persist::GraveNumMultiplier>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::GraveTrailNominalHopDistance>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::GraveTrailNominalHopCount>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::GraveTrailStartCount>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::MaxLevelDepth>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArtefactMinChance>( reg() ).render_widget();

    // Grave consequence odds line, one vertical slider per consequence, worst (0) to best (100)
    ImGui::TextUnformatted( "Grave Consequence Odds" );
    ImGui::TextDisabled( "0 = worst, 100 = best; equal values share a band; applies next level" );
    {
      const std::array<Cmp::Persist::BasePersistent<int> *, 6> odds_settings{
          &Sys::PersistSystem::get<Cmp::Persist::GraveOddsCurseTablet>( reg() ), &Sys::PersistSystem::get<Cmp::Persist::GraveOddsBombTrap>( reg() ),
          &Sys::PersistSystem::get<Cmp::Persist::GraveOddsTorch>( reg() ),       &Sys::PersistSystem::get<Cmp::Persist::GraveOddsNpcTrap>( reg() ),
          &Sys::PersistSystem::get<Cmp::Persist::GraveOddsArtefact>( reg() ),    &Sys::PersistSystem::get<Cmp::Persist::GraveOddsTalisman>( reg() ),
      };
      render_vslider_row( odds_settings );
    }
    Sys::PersistSystem::get<Cmp::Persist::GraveCountMargin>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::GraveTrailGuideColour>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ExitKeyRequirement>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NumAltars>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HealingBuildingOverride>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PaidHealAmount>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::MinNumRuins>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::RuinProcGenInitChance>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::RuinProcGenMaxIterations>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::RuinProcGenBirthThreshold>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::RuinProcGenSurvivalThreshold>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::GraveyardProcGenInitChance>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::GraveyardProcGenMaxIterations>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::GraveyardProcGenBirthThreshold>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::GraveyardProcGenSurvivalThreshold>( reg() ).render_widget();
  } catch ( const std::exception &e )
  {
    ImGui::TextColored( ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ), "Error rendering settings: %s", e.what() );
  }
  ImGui::End();
  ImGui::SFML::Render( m_window );

  if ( resolution_change_pending )
  {
    // Only now, after the frame has been fully rendered with the *old* GL context, is it safe
    // to tear the window down. ImGui::SFML::Shutdown/Init rebuild the backend's font atlas and
    // device objects against the new context - skipping this step is what left ImGui-SFML
    // rendering with dangling GL handles (the cause of the settings menu visually corrupting).
    auto &display_resolution = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );
    display_resolution = pending_resolution;

    ImGui::SFML::Shutdown( m_window );
    m_window.create( sf::VideoMode( display_resolution ), "Your Game Title", sf::State::Fullscreen );
    m_window.setVerticalSyncEnabled( true );
    if ( not ImGui::SFML::Init( m_window ) ) { SPDLOG_ERROR( "Failed to reinitialize ImGui-SFML after display resolution change" ); }
    // ImGui::SFML::Init creates a brand new ImGuiContext, so the theme applied at startup
    // (Engine.cpp) doesn't carry over - it has to be re-applied here.
    Game::Utils::apply_imgui_theme();
    // the new context's font atlas only has the default font, so the grimoire's font has to be re-added too
    Game::Utils::load_imgui_fonts();

    auto shader_view = reg().view<Cmp::Shader::SpriteOwner>();
    for ( auto [entt, shader] : shader_view.each() )
    {
      if ( not shader.sprite ) continue;
      if ( shader.sprite->get_tag() == "TitleShader" || shader.sprite->get_tag() == "CircularDistortion" )
      {
        shader.sprite->resize_texture( display_resolution );
        SPDLOG_DEBUG( "Selected resolution: {}x{}", display_resolution.x, display_resolution.y );
      }
    }
  }
}

void RenderMenuSystem::render_vslider_row( std::span<Cmp::Persist::BasePersistent<int> *const> settings )
{
  // every column is as wide as the widest slider or label, so the sliders sit at equal x intervals
  const ImVec2 slider_size( 48.0f, 160.0f );
  float column_width = slider_size.x;
  for ( auto *setting : settings )
    column_width = std::max( column_width, ImGui::CalcTextSize( setting->get_detail_ptr()->c_str() ).x );
  column_width += ImGui::GetStyle().ItemSpacing.x;

  const float row_start_x = ImGui::GetCursorPosX();
  for ( std::size_t i = 0; i < settings.size(); ++i )
  {
    auto &setting = *settings[i];
    const float column_x = row_start_x + column_width * static_cast<float>( i );
    if ( i > 0 ) ImGui::SameLine( column_x );
    ImGui::PushID( &setting );
    ImGui::BeginGroup();
    // centre the slider and its label within the column
    ImGui::SetCursorPosX( column_x + ( column_width - slider_size.x ) * 0.5f );
    ImGui::VSliderScalar( "##vslider", slider_size, ImGuiDataType_S32, setting.get_value_ptr(), setting.get_min_value_ptr(),
                          setting.get_max_value_ptr(), setting.get_format().empty() ? "%d" : setting.get_format().c_str() );
    const char *label = setting.get_detail_ptr()->c_str();
    ImGui::SetCursorPosX( column_x + ( column_width - ImGui::CalcTextSize( label ).x ) * 0.5f );
    ImGui::TextUnformatted( label );
    ImGui::EndGroup();
    ImGui::PopID();
  }
}

void RenderMenuSystem::render_defeat_screen()
{
  {

    sf::Vector2u display_size = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );

    sf::Text title_text( m_font, "You died...again!", 96 );
    title_text.setFillColor( sf::Color::White );
    title_text.setPosition( { static_cast<float>( display_size.x ) / 4.f, 100.f } );
    m_window.draw( title_text );

    sf::Text start_text( m_font, "Press <R> key to continue", 48 );
    start_text.setFillColor( sf::Color::White );
    start_text.setPosition( { static_cast<float>( display_size.x ) / 4.f, 200.f } );
    m_window.draw( start_text );
  }
}

void RenderMenuSystem::render_victory_screen( bool allow_continue, int cadaver_wealth, int anima_wealth, bool retain_anima )
{
  sf::Vector2u display_size = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );

  const int max_level_depth = Sys::PersistSystem::get<Cmp::Persist::MaxLevelDepth>( reg() ).get_value();
  const unsigned int depth = max_level_depth - Utils::Player::get_level_depth( m_reg ).get_count();
  const bool final_depth = Utils::Player::is_final_level_depth( m_reg );

  sf::Text title_text( m_font, final_depth ? std::format( "Final Level {} Complete!", depth ) : std::format( "Level {} Complete!", depth ), 96 );
  title_text.setFillColor( sf::Color::White );
  title_text.setPosition( { static_cast<float>( display_size.x ) / 4.f, 100.f } );
  m_window.draw( title_text );

  if ( allow_continue )
  {
    sf::Text start_text( m_font, final_depth ? "Press <R> key to return to the title" : "Press <R> key to continue", 48 );
    start_text.setFillColor( sf::Color::White );
    start_text.setPosition( { static_cast<float>( display_size.x ) / 4.f, 200.f } );
    m_window.draw( start_text );
  }

  const auto &wealth = Utils::Player::get_wealth( m_reg );
  const auto &cadaver_cmp = Utils::Player::get_cadaver( m_reg );

  const int anima = Utils::Player::get_stats( m_reg ).anima();

  sf::Text cadaver_txt( m_font, std::format( "Cadaver count: {}   (+{} wealth)", cadaver_cmp.count(), cadaver_wealth ), 24 );
  cadaver_txt.setFillColor( sf::Color::White );
  cadaver_txt.setPosition( { static_cast<float>( display_size.x ) / 4.f, 500.f } );
  m_window.draw( cadaver_txt );

  sf::Text anima_txt( m_font,
                      retain_anima ? std::format( "Anima: {}   (carried to next level)", anima )
                                   : std::format( "Anima: {}   (+{} wealth)", anima, anima_wealth ),
                      24 );
  anima_txt.setFillColor( sf::Color( 255, 165, 0 ) ); // orange, matching the anima meter
  anima_txt.setPosition( { static_cast<float>( display_size.x ) / 4.f, 550.f } );
  m_window.draw( anima_txt );

  sf::Text wealth_txt( m_font, "Player wealth: " + std::to_string( wealth.wealth ), 24 );
  wealth_txt.setFillColor( sf::Color::White );
  wealth_txt.setPosition( { static_cast<float>( display_size.x ) / 4.f, 600.f } );
  m_window.draw( wealth_txt );
}

} // namespace Game::Sys