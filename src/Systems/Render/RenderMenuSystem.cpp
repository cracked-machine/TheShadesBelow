

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
#include <Components/Persistent/PostPullMovementDelay.hpp>
#include <Components/Persistent/RuinMaxCobwebs.hpp>
#include <Components/Persistent/RuinMaxSpiders.hpp>
#include <Components/Persistent/RuinProcGenBirthThreshold.hpp>
#include <Components/Persistent/RuinProcGenInitChance.hpp>
#include <Components/Persistent/RuinProcGenMaxIterations.hpp>
#include <Components/Persistent/RuinProcGenSurvivalThreshold.hpp>
#include <Components/Persistent/SinkholeSeed.hpp>
#include <Components/Persistent/WeaponDegradePerHit.hpp>
#include <Components/Persistent/WormholeAnimFramerate.hpp>
#include <Components/Persistent/WormholeSeed.hpp>
#include <Components/Player/CadaverCount.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/Wealth.hpp>
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
#include <imgui.h>

namespace Game::Sys
{

RenderMenuSystem::~RenderMenuSystem() = default;

void RenderMenuSystem::render_title()
{
  // main render begin
  m_window.clear();
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

  m_window.display();
  // main render end
}

void RenderMenuSystem::render_settings( sf::Time dt )
{
  m_window.clear();
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

  // ImGUI should be rendered before window.display() or SFML wipes the display buffer prematurely
  render_settings_widgets( dt, title_text.getLocalBounds() );

  m_window.display();
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

    Sys::PersistSystem::get<Cmp::Persist::BlastRadius>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerAnimStrideLength>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerMovementSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerDiagonalLerpSpeedModifier>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerFootstepAddDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerFootstepFadeDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerFootstepSfxStrideLength>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerShortcutLerpSpeedModifier>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::DiggingCooldownThreshold>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::DiggingDamagePerHit>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::WeaponDegradePerHit>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PcDamageDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::CameraSmoothSpeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlayerLerpInterruptThreshold>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PostPullMovementDelay>( reg() ).render_widget();

    // Bomb Settings
    ImGui::SeparatorText( "Bomb Settings" );

    Sys::PersistSystem::get<Cmp::Persist::BombDamage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::FuseDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArmedOnDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArmedOffDelay>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArmedBlockColourBorder>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArmedBlockColourFill>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ArmedBlinkFreq>( reg() ).render_widget();

    // Hazard Settings
    ImGui::SeparatorText( "Hazard Settings" );

    Sys::PersistSystem::get<Cmp::Persist::CorruptionDamage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::HazardPushbackResist>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::LightningDamage>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::WormholeAnimFramerate>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::WormholeSeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::CorruptionSeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::SinkholeSeed>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::RuinMaxCobwebs>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::RuinMaxSpiders>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::CryptShuffleTimeout>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::PlantBurnDuration>( reg() ).render_widget();

    // Loot Settings
    ImGui::SeparatorText( "Loot Settings" );

    Sys::PersistSystem::get<Cmp::Persist::HealthBonus>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::BombBonus>( reg() ).render_widget();

    // NPC Settings
    ImGui::SeparatorText( "NPC Settings" );

    Sys::PersistSystem::get<Cmp::Persist::CryptNpcSpawnCount>( reg() ).render_widget();
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

    Sys::PersistSystem::get<Cmp::Persist::NpcDeathAnimFramerate>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::NpcActivateScale>( reg() ).render_widget();

    // Audio
    ImGui::SeparatorText( "Audio" );

    Sys::PersistSystem::get<Cmp::Persist::MusicVolume>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::EffectsVolume>( reg() ).render_widget();

    // Procedural Generation
    ImGui::SeparatorText( "Procedural Generation" );

    Sys::PersistSystem::get<Cmp::Persist::MaxNumAltars>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::GraveNumMultiplier>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::ExitKeyRequirement>( reg() ).render_widget();
    Sys::PersistSystem::get<Cmp::Persist::MaxNumCrypts>( reg() ).render_widget();
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

void RenderMenuSystem::render_paused( sf::Time dt )
{
  // main render begin
  m_window.clear();

  ImGui::SFML::Update( m_window, dt );
  ImGui::Begin( "PausedMenu", nullptr, kImGuiWindowOptions );

  sf::Vector2u display_size = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );

  sf::Text title_text( m_font, "Paused", 96 );
  title_text.setFillColor( sf::Color::White );
  title_text.setPosition( { static_cast<float>( display_size.x ) / 4.f, 100.f } );
  m_window.draw( title_text );

  sf::Text start_text( m_font, "Press P to continue", 48 );
  start_text.setFillColor( sf::Color::White );
  start_text.setPosition( { static_cast<float>( display_size.x ) / 4.f, 200.f } );
  m_window.draw( start_text );

  auto &music_volume = Sys::PersistSystem::get<Cmp::Persist::MusicVolume>( reg() );
  ImGui::SliderFloat( "Music Volume", &music_volume.get_value(), 0.f, 100.f, "%.1f" );

  auto &effects_volume = Sys::PersistSystem::get<Cmp::Persist::EffectsVolume>( reg() );
  ImGui::SliderFloat( "Effects Volume", &effects_volume.get_value(), 0.f, 100.f, "%.1f" );

  ImGui::End();
  ImGui::SFML::Render( m_window );

  m_window.display();
  // main render end
}

void RenderMenuSystem::render_defeat_screen()
{
  // main render begin
  m_window.clear();
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

  m_window.display();
  // main render end
}

void RenderMenuSystem::render_victory_screen( bool allow_continue )
{
  // main render begin
  m_window.clear();

  sf::Vector2u display_size = Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( reg() );

  sf::Text title_text( m_font, "Level Complete!", 96 );
  title_text.setFillColor( sf::Color::White );
  title_text.setPosition( { static_cast<float>( display_size.x ) / 4.f, 100.f } );
  m_window.draw( title_text );

  if ( allow_continue )
  {
    sf::Text start_text( m_font, "Press <R> key to continue", 48 );
    start_text.setFillColor( sf::Color::White );
    start_text.setPosition( { static_cast<float>( display_size.x ) / 4.f, 200.f } );
    m_window.draw( start_text );
  }

  auto wealth = Utils::Player::get_wealth( m_reg );
  auto cadaver_count = Utils::Player::get_cadaver_count( m_reg );

  sf::Text cadaver_txt( m_font, "Cadaver count: " + std::to_string( cadaver_count.get_count() ), 24 );
  cadaver_txt.setFillColor( sf::Color::White );
  cadaver_txt.setPosition( { static_cast<float>( display_size.x ) / 4.f, 500.f } );
  m_window.draw( cadaver_txt );

  sf::Text wealth_txt( m_font, "Player wealth: " + std::to_string( wealth.wealth ), 24 );
  wealth_txt.setFillColor( sf::Color::White );
  wealth_txt.setPosition( { static_cast<float>( display_size.x ) / 4.f, 600.f } );
  m_window.draw( wealth_txt );

  m_window.display();
  // main render end
}

} // namespace Game::Sys