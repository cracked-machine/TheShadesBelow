#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteOffset.hpp>
#include <Components/AbsoluteRotation.hpp>
#include <Components/AnimData.hpp>
#include <Components/FootStepAlpha.hpp>
#include <Components/FootStepTimer.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Persistent/EffectsVolume.hpp>
#include <Components/Persistent/PlayerFootstepAddDelay.hpp>
#include <Components/Persistent/PlayerFootstepFadeDelay.hpp>
#include <Components/Persistent/PlayerFootstepSfxStrideLength.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/FootstepType.hpp>
#include <Components/Player/MovementDelta.hpp>
#include <Components/Random.hpp>
#include <Components/SceneSettings/Footsteps.hpp>
#include <Components/ZOrderValue.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/FootstepSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <SFML/System/Time.hpp>
#include <optional>

namespace
{

struct FootstepVariant
{
  size_t frame_index_offset;
  bool diagonal;
};

// tileset indices: [0: right] [1: left] [2: down] [3: up]
// diagonal footsteps reuse the cardinal sprite, rotated 45 degrees via AbsoluteRotation
std::optional<FootstepVariant> footstep_variant_for( const sf::Vector2f &dir )
{
  if ( dir == sf::Vector2f( 1.f, 0.f ) ) return FootstepVariant{ .frame_index_offset = 0, .diagonal = false };  // right
  if ( dir == sf::Vector2f( -1.f, 0.f ) ) return FootstepVariant{ .frame_index_offset = 1, .diagonal = false }; // left
  if ( dir == sf::Vector2f( 0.f, 1.f ) ) return FootstepVariant{ .frame_index_offset = 2, .diagonal = false };  // down
  if ( dir == sf::Vector2f( 0.f, -1.f ) ) return FootstepVariant{ .frame_index_offset = 3, .diagonal = false }; // up
  if ( dir == sf::Vector2f( 1.f, 1.f ) ) return FootstepVariant{ .frame_index_offset = 0, .diagonal = true };   // down/right
  if ( dir == sf::Vector2f( -1.f, 1.f ) ) return FootstepVariant{ .frame_index_offset = 2, .diagonal = true };  // down/left
  if ( dir == sf::Vector2f( -1.f, -1.f ) ) return FootstepVariant{ .frame_index_offset = 1, .diagonal = true }; // up/left
  if ( dir == sf::Vector2f( 1.f, -1.f ) ) return FootstepVariant{ .frame_index_offset = 3, .diagonal = true };  // up/right
  return std::nullopt;
}

} // namespace

namespace Game::Sys
{

void FootstepSystem::update( sf::Time dt )
{
  auto *movement_delta = reg().try_get<Cmp::Player::MovementDelta>( Utils::Player::get_entity( reg() ) );
  const bool player_moved = movement_delta and movement_delta->m_distance > 0.f;

  // only advance the footstep timer while the player is actually moving, so standing still
  // or pushing against a wall doesn't stack footsteps on the same spot
  if ( player_moved ) update_timer += dt;

  // add new footstep for player
  auto player_view = reg().view<Cmp::Player::Character, const Cmp::Position, Cmp::Direction>();
  for ( auto [entity, player, pos_cmp, dir_cmp] : player_view.each() )
  {
    if ( dir_cmp == sf::Vector2f( 0.0f, 0.0f ) or not player_moved ) { continue; }
    add_footstep( pos_cmp, dir_cmp );
  }

  // fade is time-based (not per-frame) so footsteps fade at the same rate regardless of framerate
  static const sf::Time kFootstepFadeDuration{ sf::seconds( 4.25f ) };
  const sf::Time fade_delay = sf::seconds( Sys::PersistSystem::get<Cmp::Persist::PlayerFootstepFadeDelay>( reg() ).get_value() );
  auto view = reg().view<Cmp::FootStepTimer, Cmp::FootStepAlpha>();
  for ( auto [entity, timer, alpha] : view.each() )
  {
    timer.m_elapsed += dt;
    if ( timer.m_elapsed <= fade_delay ) continue;

    const float fade_progress = ( timer.m_elapsed - fade_delay ) / kFootstepFadeDuration;
    if ( fade_progress >= 1.f )
    {
      SPDLOG_DEBUG( "Removing footstep entity {}", int( entity ) );
      reg().destroy( entity );
      continue;
    }
    alpha.m_alpha = static_cast<uint8_t>( 255.f * ( 1.f - fade_progress ) );
  }

  // Distance the player must cover between footstep sounds; tying the trigger to
  // distance rather than time makes the cadence scale with actual movement speed automatically,
  // including any active Cmp::Player::SpeedPenalty and being blocked by collision.
  const float footstep_sfx_stride_length = Sys::PersistSystem::get<Cmp::Persist::PlayerFootstepSfxStrideLength>( reg() ).get_value();
  if ( not movement_delta ) return;
  m_footstep_sfx_distance += movement_delta->m_distance;
  if ( m_footstep_sfx_distance >= footstep_sfx_stride_length )
  {
    // only consume the stride if a sound actually played; otherwise hold the step until the
    // previous clip finishes rather than silently dropping it (clamped to avoid a burst of steps)
    if ( play_footsteps_sound() ) { m_footstep_sfx_distance -= footstep_sfx_stride_length; }
    else { m_footstep_sfx_distance = footstep_sfx_stride_length; }
  }
}

void FootstepSystem::add_footstep( const Cmp::Position &pos, const Cmp::Direction &dir )
{
  if ( not Utils::scene_setting<Cmp::SceneSettings::Footsteps>( reg() ).enabled ) return;
  const sf::Time add_delay = sf::seconds( Sys::PersistSystem::get<Cmp::Persist::PlayerFootstepAddDelay>( reg() ).get_value() );
  if ( update_timer >= add_delay )
  {

    auto nopathfind_view = reg().view<Cmp::Npc::NoPathFinding, Cmp::Position>();
    for ( auto [nopath_entity, nopath_cmp, nopath_pos_cmp] : nopathfind_view.each() )
    {
      if ( nopath_pos_cmp.findIntersection( pos ) )
      {
        // Don't create footstep in no-pathfinding area
        return;
      }
    }

    auto variant = footstep_variant_for( dir );
    if ( not variant ) return;

    auto entity = reg().create();
    reg().emplace<Cmp::Direction>( entity, dir );
    reg().emplace<Cmp::FootStepTimer>( entity );
    reg().emplace<Cmp::FootStepAlpha>( entity );
    reg().emplace<Cmp::ZOrderValue>( entity, -8.f ); // footsteps render below player
    // clang-format off
    reg().emplace<Cmp::AnimData>( entity, Cmp::AnimData::Config{
          .sprite_type = "sprite.player.footsteps",
          .frame_index_offset = variant->frame_index_offset,
          .enabled = true
    });
    // clang-format on
    reg().emplace<Cmp::Position>( entity, pos.position, pos.size );

    // diagonal footsteps reuse the cardinal sprite, rotated via AbsoluteRotation
    if ( variant->diagonal )
    {
      reg().emplace<Cmp::AbsoluteRotation>( entity, 45.f );
      reg().emplace<Cmp::AbsoluteOffset>( entity, Constants::kGridSizePx.x / 2.f, Constants::kGridSizePx.y / 2.f );
    }

    // Carry over any overshoot so step spacing doesn't drift with frame timing
    update_timer -= add_delay;
    if ( update_timer >= add_delay ) update_timer = sf::Time::Zero;
  }
}

bool FootstepSystem::play_footsteps_sound()
{

  auto play = [this]( const std::string &sfx_name ) -> bool
  {
    if ( m_sound_bank.get_effect( sfx_name ).getStatus() == sf::Sound::Status::Playing ) return false;
    m_alternate_footsteps = not m_alternate_footsteps;
    auto &sfx = m_sound_bank.get_effect( sfx_name );
    // add some variance to the steps to make it sound more natural, scaled by the user's effects volume
    // setting so muting effects (SoundBank::update_effects_volume) actually mutes footsteps too
    const auto effects_volume = Sys::PersistSystem::get<Cmp::Persist::EffectsVolume>( reg() ).get_value();
    sfx.setVolume( effects_volume * Cmp::RandomFloat( 0.5f, 1.f ).gen() );
    sfx.play();
    return true;
  };

  auto &footstep_type = Utils::Player::get_footstep_type( reg() );

  switch ( footstep_type.m_type )
  {
    case Cmp::Player::Footstep::Type::NONE:
      return true; // silent surface: consume the stride
    case Cmp::Player::Footstep::Type::GRASS:
      return play( "footstep_grass" );
    case Cmp::Player::Footstep::Type::WOODFLOOR:
      return play( "footstep_woodfloor" );
    case Cmp::Player::Footstep::Type::MUD:
      return play( "footstep_mud" );
    case Cmp::Player::Footstep::Type::STONE:
      return play( "footstep_stone" );
  }
  return true;
}

void FootstepSystem::stop_footsteps_sound()
{
  // add more footstep sfx here when needed
  m_sound_bank.get_effect( "footstep_grass" ).stop();
  m_sound_bank.get_effect( "footstep_woodfloor" ).stop();
  m_sound_bank.get_effect( "footstep_mud" ).stop();
  m_sound_bank.get_effect( "footstep_stone" ).stop();
}

} // namespace Game::Sys