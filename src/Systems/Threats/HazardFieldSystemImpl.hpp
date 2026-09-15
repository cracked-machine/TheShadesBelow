#ifndef SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEMIMPL_HPP__
#define SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEMIMPL_HPP__

#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
#include <Components/Exit.hpp>
#include <Components/Hazard/CollisionResist.hpp>
#include <Components/Hazard/CorruptionCell.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Persistent/HazardPushbackResist.hpp>
#include <Components/Player/Character.hpp>
#include <Components/RectBounds.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/PauseClocksEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Events/ResumeClocksEvent.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/ObstacleFactory.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/Threats/HazardFieldSystem.hpp>
#include <Systems/Threats/HazardTraits.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

namespace Game::Sys
{

namespace Detail
{
//! @brief Always false, but depends on T so static_assert(dependent_false<T>) only fires when the
//! enclosing `if constexpr` branch is actually instantiated, not for every instantiation of the template.
template <typename T>
inline constexpr bool dependent_false = false;
} // namespace Detail

//! @copydoc HazardFieldSystem::HazardFieldSystem(entt::registry&,sf::RenderWindow&,Sprites::SpriteFactory&,Audio::SoundBank&)
template <ValidHazard HazardType>
HazardFieldSystem<HazardType>::HazardFieldSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory,
                                                  Audio::SoundBank &sound_bank )
    : Sys::BaseSystem( reg, window, sprite_factory, sound_bank )
{
  get_systems_event_queue().sink<Events::PauseClocksEvent>().connect<&Sys::HazardFieldSystem<HazardType>::on_pause>( this );
  get_systems_event_queue().sink<Events::ResumeClocksEvent>().connect<&Sys::HazardFieldSystem<HazardType>::on_resume>( this );
}

//! @copydoc HazardFieldSystem::update()
template <ValidHazard HazardType>
sf::Vector2f HazardFieldSystem<HazardType>::update( sf::Time dt )
{
  auto add_hazard_cell = update_hazard_field();
  check_npc_hazard_field_collision();

  m_dmg_timer += dt;
  static constexpr sf::Time dmg_timeout = sf::seconds( 0.2f );
  if ( m_dmg_timer > dmg_timeout )
  {
    if ( Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled ) { check_player_hazard_field_collision(); }
    m_dmg_timer = sf::Time::Zero;
  }

  return add_hazard_cell;
}

//! @copydoc HazardFieldSystem::init_hazard_field()
template <ValidHazard HazardType>
sf::Vector2f HazardFieldSystem<HazardType>::init_hazard_field()
{
  auto hazard_field_view = reg().template view<HazardType>( entt::exclude<typename Traits::ExcludeHazard> );
  if ( hazard_field_view.size_hint() > 0 ) { return {}; }

  unsigned long seed = Sys::PersistSystem::get<typename Traits::SeedType>( reg() ).get_value();
  auto [random_entity, random_pos] = Utils::Rnd::get_random_position(
      reg(), Utils::Rnd::IncludePack<Cmp::Obstacle>{}, Utils::Rnd::ExcludePack<Cmp::Wall, Cmp::Exit, Cmp::Player::Character, Cmp::Npc::NPC>(), seed );
  if ( random_entity == entt::null ) { return {}; }
  auto reserved_sm = m_reserved_sm.lock();
  if ( reserved_sm && not reserved_sm->at( random_pos ).empty() ) { return {}; }

  promote_to_hazard_cell( random_entity, random_pos, reserved_sm );
  SPDLOG_INFO( "{} hazard spawned at position [{}, {}].", std::string( Traits::sprite_type ), random_pos.position.x, random_pos.position.y );

  return random_pos.position;
}

//! @copydoc HazardFieldSystem::promote_to_hazard_cell()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::promote_to_hazard_cell( entt::entity entity, const Cmp::Position &pos,
                                                            const PathFinding::SpatialHashGridSharedPtr &reserved_sm )
{
  Factory::Obstacle::remove_obstacle( reg(), entity, Factory::Obstacle::DeleteExtras::Yes, reserved_sm );
  // remove_obstacle() above un-reserves this position; re-reserve it now the entity is a hazard cell,
  // otherwise BombSystem's blast-arming sweep will still find it via its leftover Cmp::Armable
  if ( reserved_sm ) { reserved_sm->insert( entity, pos ); }
  // clang-format off
  reg().template emplace_or_replace<HazardType>( entity );
  reg().template emplace_or_replace<Cmp::AnimData>( entity, Cmp::AnimData::Config{
        .sprite_type =  std::string( Traits::sprite_type ),
        .enabled = true
  });
  // clang-format on
  reg().template emplace_or_replace<Cmp::ZOrderValue>( entity, pos.position.y - 1.f );
  reg().template emplace_or_replace<Cmp::Npc::NoPathFinding>( entity );
  // corruption is a gradual damage field, not an instant kill, so it never gets an initial pushback
  if constexpr ( not std::is_same_v<HazardType, Cmp::CorruptionCell> )
  {
    auto resist_seconds = Sys::PersistSystem::get<Cmp::Persist::HazardPushbackResist>( reg() ).get_value();
    reg().template emplace_or_replace<Cmp::Hazard::CollisionResist>( entity, resist_seconds );
  }
}

//! @copydoc HazardFieldSystem::on_pause()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::on_pause()
{
  m_spread_update_clock.stop();
}

//! @copydoc HazardFieldSystem::on_resume()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::on_resume()
{
  m_spread_update_clock.start();
}

//! @copydoc HazardFieldSystem::update_hazard_field()
template <ValidHazard HazardType>
sf::Vector2f HazardFieldSystem<HazardType>::update_hazard_field()
{
  if ( m_spread_update_clock.getElapsedTime() < m_update_period ) return {};
  m_spread_update_clock.restart();

  auto reserved_sm = m_reserved_sm.lock();
  auto hazard_view = reg().template view<HazardType, Cmp::Position>();
  auto obstacle_view = reg().template view<Cmp::Obstacle, Cmp::Position, Cmp::AnimData>();

  Cmp::RandomInt hazard_spread_picker( 0, Traits::odds ); // 1 in 8 chance for picking an adjacent obstacle

  for ( auto [hazard_entity, hazard_cmp, position_cmp] : hazard_view.each() )
  {
    SPDLOG_DEBUG( "Hazard {} active: {}", static_cast<uint32_t>( hazard_entity ), hazard_cmp.active );
    if ( not hazard_cmp.active ) continue;

    // make the hazard field hitbox slightly larger to find adjacent obstacles
    auto hazard_hitbox = sf::FloatRect( position_cmp.position - Constants::kGridSizePxF, Constants::kGridSizePxF * 3.f );
    int adjacent_hazard_fields = 0;

    // add new hazard cell
    for ( auto [obstacle_entity, obstacle_cmp, obst_pos_cmp, obst_anim_cmp] : obstacle_view.each() )
    {
      // only search for main obstacles, cap obstacles are removed implicitly by Factory::Obstacle::remove_obstacle()
      if ( not obst_anim_cmp.m_sprite_type.contains( ".main" ) ) continue;
      if ( not hazard_hitbox.findIntersection( obst_pos_cmp ) ) continue;
      if ( reserved_sm && not reserved_sm->at( obst_pos_cmp ).empty() ) continue;
      SPDLOG_DEBUG( "Hazard intersected with object {}", static_cast<uint32_t>( obstacle_entity ) );

      if ( reg().template all_of<HazardType>( obstacle_entity ) ) continue;
      SPDLOG_DEBUG( "Hazard not found at entity {}", static_cast<uint32_t>( obstacle_entity ) );

      auto hazard_pick = hazard_spread_picker.gen();
      SPDLOG_DEBUG( "hazard_pick:{}", hazard_pick );
      if ( hazard_pick == 0 )
      {
        promote_to_hazard_cell( obstacle_entity, obst_pos_cmp, reserved_sm );
        SPDLOG_DEBUG( "New hazard field created at entity {}", static_cast<uint32_t>( obstacle_entity ) );
        return obst_pos_cmp.position; // only add one hazard cell per update period
      }
    }

    // update adjacent hazard cell count
    for ( auto [adj_hazard_entity, adj_hazard_cmp, adj_pos_cmp] : hazard_view.each() )
    {
      if ( hazard_entity == adj_hazard_entity ) continue;
      if ( hazard_hitbox.findIntersection( adj_pos_cmp ) ) { adjacent_hazard_fields++; }
    }

    // if the hazard field is surrounded by hazard fields, then we can exclude it from future
    // searches
    SPDLOG_DEBUG( "Hazard {} adjacent count: {}", static_cast<uint32_t>( hazard_entity ), adjacent_hazard_fields );
    if ( adjacent_hazard_fields >= 4 ) { hazard_cmp.active = false; }
  }
  return {};
}

//! @copydoc HazardFieldSystem::check_player_hazard_field_collision()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::check_player_hazard_field_collision()
{
  auto hazard_view = reg().template view<HazardType, Cmp::Position>();
  auto player_view = reg().template view<Cmp::Player::Character, Cmp::PlayerStats, Cmp::Player::Mortality, Cmp::Position>();
  const auto &player_position = Utils::Player::get_position( reg() );
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [pc_entt, player_cmp, player_stats_cmp, player_mort_cmp, player_pos_cmp] : player_view.each() )
  {
    if ( not Utils::is_visible_in_view( view_bounds, player_pos_cmp ) ) continue;

    // dont spam death events if the player is already dead
    if ( player_mort_cmp.state == Cmp::Player::Mortality::State::DEAD ) continue;

    if constexpr ( Traits::sprite_type == "sprite.graveyard.hazard.corruption" )
    {
      // default to the corruption-field footstep SFX; a hazard hit below overrides it with MUD
      reg().template emplace_or_replace<Cmp::Player::Footstep>( Utils::Player::get_entity( m_reg ), Cmp::Player::Footstep::Type::GRASS );
    }

    for ( auto [hazard_entt, hazard_cmp, hazard_pos_cmp] : hazard_view.each() )
    {

      if constexpr ( Traits::sprite_type == "sprite.graveyard.hazard.sinkhole" )
      {
        // full size hitbox - Cmp::Hazard::CollisionResist now guards against accidental entry
        if ( hazard_pos_cmp.findIntersection( player_pos_cmp ) )
        {
          // trigger death animation
          get_systems_event_queue().trigger( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::FALLING, player_position ) );
          return;
        }
      }
      else if constexpr ( Traits::sprite_type == "sprite.graveyard.hazard.corruption" )
      {
        // normal size hitbox for corruption for full area
        if ( hazard_pos_cmp.findIntersection( player_pos_cmp ) )
        {
          // Set the default footstep SFX for this scene
          reg().template emplace_or_replace<Cmp::Player::Footstep>( Utils::Player::get_entity( m_reg ), Cmp::Player::Footstep::Type::MUD );
          auto corruption_dmg = Sys::PersistSystem::get<Cmp::Persist::CorruptionDamage>( reg() ).get_value();
          player_stats_cmp.apply( { Cmp::Stats::Health{ -corruption_dmg }, {}, {}, {}, {}, {}, {}, {} } );
          SPDLOG_DEBUG( "Applying corruption damage {}", corruption_dmg );
          // trigger death animation
          if ( player_stats_cmp.health() <= 0 )
          {
            get_systems_event_queue().trigger( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::DECAYING, player_position ) );
          }
          return;
        }
      }
      else
      {
        // new HazardType added without a matching sprite_type branch here - fails to compile instead of
        // silently skipping player collision handling for it
        static_assert( Detail::dependent_false<HazardType>, "check_player_hazard_field_collision: unhandled Traits::sprite_type" );
      }
    }
  }
}

//! @copydoc HazardFieldSystem::check_npc_hazard_field_collision()
template <ValidHazard HazardType>
void HazardFieldSystem<HazardType>::check_npc_hazard_field_collision()
{
  auto hazard_view = reg().template view<HazardType, Cmp::Position>();
  auto npc_view = reg().template view<Cmp::Npc::NPC, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [npc_entt, npc_cmp, npc_pos_cmp] : npc_view.each() )
  {
    // optimization
    if ( !Utils::is_visible_in_view( view_bounds, npc_pos_cmp ) ) continue;

    for ( auto [hazard_entt, hazard_cmp, hazard_pos_cmp] : hazard_view.each() )
    {
      if ( not npc_pos_cmp.findIntersection( hazard_pos_cmp ) ) continue;

      auto loot_entity = Factory::Npc::destroy_npc( reg(), npc_entt );
      if ( loot_entity != entt::null )
      {
        SPDLOG_DEBUG( "Dropped RELIC_DROP loot at NPC death position." );
        m_sound_bank.get_effect( "drop_relic" ).play();
      }
      SPDLOG_DEBUG( "NPC fell into a hazard field at position ({}, {})!", hazard_pos_cmp.x, hazard_pos_cmp.y );
      return;
    }
  }
}

//! @brief Concrete derived HazardFieldSystem using CRTP
class SinkHoleHazardSystem final : public HazardFieldSystem<Cmp::SinkholeCell>
{
public:
  using HazardFieldSystem<Cmp::SinkholeCell>::HazardFieldSystem;
};

//! @brief Concrete derived HazardFieldSystem using CRTP
class CorruptionHazardSystem final : public HazardFieldSystem<Cmp::CorruptionCell>
{
public:
  using HazardFieldSystem<Cmp::CorruptionCell>::HazardFieldSystem;

  // Add corruption-specific methods here if needed
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_THREATS_HAZARDFIELDSYSTEMIMPL_HPP__
