#include <Audio/SoundBank.hpp>
#include <Components/Direction.hpp>
#include <Components/Exit.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/ShadowHand.hpp>
#include <Components/Particle/BlockParticle.hpp>
#include <Components/Persistent/RuinMaxSpiders.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/Curse.hpp>
#include <Components/Player/Mortality.hpp>
#include <Components/Player/NoPath.hpp>
#include <Components/Player/RuinLocation.hpp>
#include <Components/Player/SpeedPenalty.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Ruin/Bookcase.hpp>
#include <Components/Ruin/BuildingMultiBlock.hpp>
#include <Components/Ruin/BuildingSegment.hpp>
#include <Components/Ruin/Cobweb.hpp>
#include <Components/Ruin/Entrance.hpp>
#include <Components/Ruin/FloorAccess.hpp>
#include <Components/Ruin/GateSegment.hpp>
#include <Components/Ruin/RuneMarking.hpp>
#include <Components/Ruin/StairsBalustradeMultiBlock.hpp>
#include <Components/Ruin/StairsGateMultiBlock.hpp>
#include <Components/Ruin/StairsLowerMultiBlock.hpp>
#include <Components/Ruin/StairsSegment.hpp>
#include <Components/Ruin/StairsUpperMultiBlock.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/CollisionAction.hpp>

#include <Components/Wall.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/DropInventoryEvent.hpp>
#include <Events/PlayerMortalityEvent.hpp>
#include <Factory/MultiblockFactory.hpp>
#include <Factory/NpcFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/RuinFactory.hpp>
#include <SceneControl/Events/SceneManagerEvent.hpp>
#include <Sprites/SpriteSheet.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/RuinSystem.hpp>
#include <Systems/Stores/NpcStore.hpp>
#include <Utils/Collision.hpp>
#include <Utils/Constants.hpp>
#include <Utils/Player.hpp>
#include <Utils/Random.hpp>
#include <Utils/Utils.hpp>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>
#include <entt/entity/fwd.hpp>
#include <numbers>
#include <set>
#include <typeindex>

namespace Game::Sys
{

RuinSystem::RuinSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank,
                        entt::dispatcher &scenemanager_event_dispatcher )
    : BaseSystem( reg, window, sprite_factory, sound_bank ),
      m_scenemanager_event_dispatcher( scenemanager_event_dispatcher )
{
  SPDLOG_DEBUG( "PlayerSystem initialized" );
}

void RuinSystem::check_entrance_collision()
{
  auto player_pos = Utils::Player::get_position( reg() );
  auto door_view = reg().view<Cmp::Ruin::Entrance, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );
  for ( auto [door_entity, door_cmp, door_pos_cmp] : door_view.each() )
  {
    // optimize: skip if not visible
    if ( !Utils::is_visible_in_view( view_bounds, door_pos_cmp ) ) continue;

    // Player can't intersect with a closed crypt door so expand their hitbox to facilitate collision detection
    auto player_hitbox = Cmp::RectBounds::scaled( player_pos.position, player_pos.size, 0.5f );
    if ( not player_hitbox.findIntersection( door_pos_cmp ) ) continue;
    m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::ENTER_RUIN_LOWER );

    get_systems_event_queue().trigger( Events::DropInventoryEvent() );

    Factory::Player::remove_player_last_graveyard_pos( reg() );
    Cmp::Position last_known_pos(
        {
            door_pos_cmp.position.x,
            door_pos_cmp.position.y + Constants::kGridSizePxF.y,
        },
        Constants::kGridSizePxF );
    SPDLOG_INFO( "Last known graveyard position {}, {}", last_known_pos.position.x, last_known_pos.position.y );
    Factory::Player::add_player_last_graveyard_pos( reg(), last_known_pos );
    break;
  }
}

void RuinSystem::update_exit_zorder()
{
  auto player_pos = Utils::Player::get_position( reg() );

  for ( auto [mb_entt, mb_cmp, mb_z_cmp] : reg().view<Cmp::Ruin::BuildingMultiBlock, Cmp::ZOrderValue>().each() )
  {
    if ( not player_pos.findIntersection( mb_cmp ) ) continue;
    auto segment_view = reg().view<Cmp::Ruin::BuildingSegment, Cmp::Position, Cmp::ZOrderValue>();
    for ( auto [segment_entt, segment_cmp, segment_pos_cmp, segment_z_cmp] : segment_view.each() )
    {
      if ( not player_pos.findIntersection( segment_pos_cmp ) ) continue;
      mb_z_cmp.setZOrder( segment_z_cmp.getZOrder() );
      SPDLOG_DEBUG( "Updated zorder to {}", segment_z_cmp.getZOrder() );
    }
  }
}

void RuinSystem::check_exit_collision()
{
  auto player_pos = Utils::Player::get_position( reg() );
  auto door_view = reg().view<Cmp::Exit, Cmp::Position>();
  const auto view_bounds = Utils::calculate_view_bounds( RenderSystem::get_world_view() );

  for ( auto [door_entity, door_cmp, door_pos_cmp] : door_view.each() )
  {
    // optimize: skip if not visible
    if ( !Utils::is_visible_in_view( view_bounds, door_pos_cmp ) ) continue;

    auto decreased_entrance_bounds = Cmp::RectBounds::scaled( door_pos_cmp.position, door_pos_cmp.size, 0.1f,
                                                              Cmp::RectBounds::ScaleAxis::XY ); // shrink entrance bounds slightly for better UX

    if ( not player_pos.findIntersection( decreased_entrance_bounds.getBounds() ) ) continue;

    m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::EXIT_RUIN );
    reset_player_curse();
  }
}

void RuinSystem::check_puzzle_status()
{
  size_t active_run_count = 0;
  auto rune_view = reg().view<Cmp::RuneMarking>();
  auto rune_view_size = rune_view->size();
  for ( auto [rune_entt, rune_cmp] : rune_view.each() )
  {
    if ( rune_cmp.active ) active_run_count++;
  }

  if ( active_run_count == rune_view_size )
  {
    // disable the gate
    for ( auto [gate_entt, gate_cmp, gate_zorder_cmp] : reg().view<Cmp::Ruin::StairsGateMultiBlock, Cmp::ZOrderValue>().each() )
    {
      gate_zorder_cmp.setZOrder( -1000.f );
      reg().remove<Cmp::Npc::NoPathFinding>( gate_entt );
      reg().remove<Cmp::Player::NoPath>( gate_entt );
      reg().remove<Cmp::Particle::BlockParticle>( gate_entt );
    }
    for ( auto [gate_entt, gate_cmp] : reg().view<Cmp::Ruin::GateSegment>().each() )
    {
      reg().remove<Cmp::Npc::NoPathFinding>( gate_entt );
      reg().remove<Cmp::Player::NoPath>( gate_entt );
      reg().remove<Cmp::Particle::BlockParticle>( gate_entt );
    }

    auto &sfx = m_sound_bank.get_effect( "secret" );
    if ( not m_puzzle_solved ) { sfx.play(); }
    m_puzzle_solved = true;
  }
  else
  {
    // enable the gate
    for ( auto [gate_entt, gate_cmp, gate_zorder_cmp] : reg().view<Cmp::Ruin::StairsGateMultiBlock, Cmp::ZOrderValue>().each() )
    {
      const auto &gate_sprite_sheet = m_sprite_factory.get_spritesheet_by_type( "sprite.ruin.stairs.gate" );
      gate_zorder_cmp.setZOrder( gate_sprite_sheet.get_zorder( 0 ) );
      reg().emplace_or_replace<Cmp::Npc::NoPathFinding>( gate_entt );
      reg().emplace_or_replace<Cmp::Player::NoPath>( gate_entt );
      reg().emplace_or_replace<Cmp::Particle::BlockParticle>( gate_entt );
    }
    for ( auto [gate_entt, gate_cmp] : reg().view<Cmp::Ruin::GateSegment>().each() )
    {
      reg().emplace_or_replace<Cmp::Npc::NoPathFinding>( gate_entt );
      reg().emplace_or_replace<Cmp::Player::NoPath>( gate_entt );
      reg().emplace_or_replace<Cmp::Particle::BlockParticle>( gate_entt );
    }
    m_puzzle_solved = false;
  }
}

void RuinSystem::spawn_floor_access( sf::Vector2f spawn_position, sf::Vector2f size, Cmp::Ruin::FloorAccess::Direction dir )
{
  auto floor_access_entt = reg().create();
  reg().emplace_or_replace<Cmp::Ruin::FloorAccess>( floor_access_entt, spawn_position, size, dir );
  SPDLOG_DEBUG( "Spawning floor access at {},{}", spawn_position.x, spawn_position.y );
}

void RuinSystem::check_floor_access_collision( Cmp::Ruin::FloorAccess::Direction direction )
{
  if ( m_floor_access_cooldown.getElapsedTime().asSeconds() < kFloorAccessCooldownSeconds ) { return; }

  auto player_pos = Utils::Player::get_position( reg() );
  bool currently_on_floor_access = false;

  for ( auto [access_entt, access_cmp] : reg().view<Cmp::Ruin::FloorAccess>().each() )
  {
    if ( player_pos.findIntersection( access_cmp ) )
    {
      currently_on_floor_access = true;

      // Only trigger if player wasn't already on floor access (must leave and re-enter)
      if ( m_was_on_floor_access ) { continue; }

      // Check if THIS floor access entity leads in the direction we're looking for
      if ( access_cmp.m_direction != direction ) { continue; }

      switch ( access_cmp.m_direction )
      {
        case Cmp::Ruin::FloorAccess::Direction::TO_UPPER:
          m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::ENTER_RUIN_UPPER );
          break;
        case Cmp::Ruin::FloorAccess::Direction::TO_LOWER:
          m_scenemanager_event_dispatcher.enqueue<Events::SceneManagerEvent>( Events::SceneManagerEvent::Type::EXIT_RUIN_UPPER );
          break;
      }
    }
  }

  // Update tracking - player must leave floor access area before it can trigger again
  m_was_on_floor_access = currently_on_floor_access;
}

void RuinSystem::check_movement_slowdowns()
{
  auto player_pos = Utils::Player::get_position( reg() );
  auto player_entt = Utils::Player::get_entity( reg() );

  float slowdown_penalty = 0.0f;

  // Check staircase collision
  if ( Utils::Collision::check_pos<Cmp::Ruin::StairsLowerMultiBlock>( reg(), Cmp::RectBounds::scaled( player_pos, 1.f ) ) )
  {
    slowdown_penalty = std::max( slowdown_penalty, 0.7f );
  }
  if ( Utils::Collision::check_pos<Cmp::Ruin::StairsUpperMultiBlock>( reg(), Cmp::RectBounds::scaled( player_pos, 1.f ) ) )
  {
    slowdown_penalty = std::max( slowdown_penalty, 0.7f );
  }

  // Check cobweb collision
  if ( Utils::Collision::check_cmp<Cmp::Ruin::Cobweb>( reg(), Cmp::RectBounds::scaled( player_pos, 1 ),
                                                       []( const Cmp::Ruin::Cobweb &cobweb ) { return cobweb.integrity > 0; } ) )
  {
    slowdown_penalty = std::max( slowdown_penalty, 0.5f );
  }

  // Apply or remove penalty
  auto &sfx = m_sound_bank.get_effect( "cobweb_movement" );
  bool player_moving = ( Utils::Player::get_direction( reg() ) != sf::Vector2f( 0.f, 0.f ) );

  if ( slowdown_penalty > 0.0f )
  {
    reg().emplace_or_replace<Cmp::Player::SpeedPenalty>( player_entt, slowdown_penalty );
    if ( player_moving )
    {
      if ( sfx.getStatus() != sf::Sound::Status::Playing ) sfx.play();
    }
    else { sfx.stop(); }
  }
  else
  {
    if ( reg().any_of<Cmp::Player::SpeedPenalty>( player_entt ) ) { reg().remove<Cmp::Player::SpeedPenalty>( player_entt ); }
    sfx.stop();
  }
}

void RuinSystem::gen_lowerfloor_bookcases( sf::FloatRect scene_dimensions )
{
  SPDLOG_INFO( "gen_lowerfloor_obstacles" );
  using namespace std::views;

  auto reserved_sm = m_reserved_sm.lock();
  auto has_collision = [&]( Cmp::RectBounds pos )
  {
    if ( Utils::Collision::check_cmp<Cmp::Ruin::Bookcase>( reg(), pos ) ) { return true; }
    if ( Utils::Collision::check_cmp<Cmp::Npc::NoPathFinding>( reg(), pos ) ) { return true; }
    if ( reserved_sm && not reserved_sm->query_rect( pos.getBounds() ).empty() ) { return true; }

    // ensure bookcase is inside scene
    if ( not Cmp::RectBounds::scaled( pos.position(), pos.size(), 1.5f ).findIntersection( scene_dimensions ) ) { return true; }
    return false;
  };

  constexpr auto gridsize = Constants::kGridSizePxF;
  constexpr auto ScaleCardinality_VERTICAL = Cmp::RectBounds::ScaleAxis::Y;

  int max_rows = scene_dimensions.size.y / gridsize.y;
  int max_cols = scene_dimensions.size.x / gridsize.x;

  // odd number rows should be long broken lines of bookcases
  for ( int row : iota( 1, max_rows ) | std::views::stride( 2 ) )
  {
    for ( int col = 1; col < max_cols; /* incremented in loop body */ )
    {
      std::vector<std::pair<Cmp::RectBounds, Sprites::SpriteMetaType>> bookshelf_row_candidate{};
      bool valid_candidate = true;

      // column picker as we traverse row left->right: 1 in N chance of starting a new bookcase
      Cmp::RandomInt column_pick( 0, 1 );
      if ( column_pick.gen() != 0 )
      {
        SPDLOG_DEBUG( "Skipping col #{}", col );
        ++col;
        continue;
      }

      // bookcase left edge
      auto bc_left_coord = Cmp::RectBounds::scaled( { col * gridsize.x, row * gridsize.y }, gridsize, 1 );
      SPDLOG_DEBUG( "Row #{} candidate left edge is {},{}", row, bc_left_coord.position().x, bc_left_coord.position().y );
      if ( has_collision( Cmp::RectBounds::scaled( bc_left_coord.position(), bc_left_coord.size(), 1.f ) ) )
      {
        SPDLOG_DEBUG( "Left edge rejected at {},{}", bc_left_coord.position().x, bc_left_coord.position().y );
        ++col;
        continue;
      }
      bookshelf_row_candidate.emplace_back( bc_left_coord, "sprite.ruin.bookcase.left" );

      // bookcase middle pieces - advance N times from the left edge section coord
      Cmp::RectBounds bc_mid_coord = bc_left_coord;
      Cmp::RandomInt bookcase_length( 2, 3 );

      for ( auto bc_mid_idx : iota( 1, bookcase_length.gen() ) )
      {
        bc_mid_coord = Cmp::RectBounds::scaled( { bc_left_coord.position().x + gridsize.x * bc_mid_idx, bc_left_coord.position().y }, gridsize, 1 );

        if ( has_collision( Cmp::RectBounds::scaled( bc_mid_coord.position(), bc_mid_coord.size(), 1, ScaleCardinality_VERTICAL ) ) )
        {
          SPDLOG_DEBUG( "Mid section rejected at {},{}", bc_mid_coord.position().x, bc_mid_coord.position().y );
          valid_candidate = false;
          break;
        }
        bookshelf_row_candidate.emplace_back( bc_mid_coord, "sprite.ruin.bookcase.mid" );
      }

      if ( not valid_candidate )
      {
        ++col;
        continue;
      }

      // bookcase right edge
      auto bc_right_coord = Cmp::RectBounds::scaled( { bc_mid_coord.position().x + gridsize.x, bc_mid_coord.position().y }, gridsize, 1 );
      if ( has_collision( bc_right_coord ) )
      {
        SPDLOG_DEBUG( "Right edge rejected at {},{}", bc_right_coord.position().x, bc_right_coord.position().y );
        ++col;
        continue;
      }
      bookshelf_row_candidate.emplace_back( bc_right_coord, "sprite.ruin.bookcase.right" );

      // add the line of bookshelf sprites
      SPDLOG_DEBUG( "bookcase candidate of length: {}", bookshelf_row_candidate.size() );
      for ( auto [point, type] : bookshelf_row_candidate )
      {
        auto [ms, idx] = m_sprite_factory.get_random_type_and_texture_index( { type } );
        Factory::Ruin::create_bookcase( reg(), point.position(), m_sprite_factory.get_spritesheet_by_type( ms ), idx );
      }

      // ensure we have one horizontal gap inbetween bookcases
      col += bookshelf_row_candidate.size() + 1;
    }
  }

  // even number rows are empty so lets add a single bookcase to block the player
  std::set<int> used_cols{};
  constexpr int max_attempts = 60;
  for ( int row : iota( 0, max_rows - 1 ) | std::views::stride( 2 ) )
  {
    for ( int attempt = 0; attempt < max_attempts; ++attempt )
    {
      Cmp::RandomInt column_pick( 0, max_cols );
      int colpick = column_pick.gen();
      auto point = Cmp::RectBounds::scaled( { colpick * gridsize.x, row * gridsize.y }, gridsize, 1 );
      if ( has_collision( point ) or used_cols.contains( colpick ) ) continue;
      auto [ms, idx] = m_sprite_factory.get_random_type_and_texture_index( { "sprite.ruin.bookcase.mid" } );
      Factory::Ruin::create_bookcase( reg(), point.position(), m_sprite_factory.get_spritesheet_by_type( ms ), idx );
      used_cols.insert( colpick );
      break;
    }
  }

  // always block this col on the bottom row
  int colpick = 8;
  auto point = Cmp::RectBounds::scaled( { colpick * gridsize.x, scene_dimensions.size.y - gridsize.y }, gridsize, 1 );
  auto [ms, idx] = m_sprite_factory.get_random_type_and_texture_index( { "sprite.ruin.bookcase.mid" } );
  Factory::Ruin::create_bookcase( reg(), point.position(), m_sprite_factory.get_spritesheet_by_type( ms ), idx );
  used_cols.insert( colpick );
}

void RuinSystem::remove_rune_markings_neighbouring_cobwebs( PathFinding::SpatialHashGrid &non_obstacle_spatialgrid )
{
  std::vector<std::pair<entt::entity, Cmp::Position>> kill_list;
  for ( auto [rune_entt, rune_cmp, rune_pos_cmp] : reg().view<Cmp::RuneMarking, Cmp::Position>().each() )
  {
    int neighbour_count = 0;
    auto neighbours_list = non_obstacle_spatialgrid.neighbours( rune_pos_cmp );
    for ( auto &neighbour_entt : neighbours_list )
    {
      if ( not reg().any_of<Cmp::Ruin::Cobweb>( neighbour_entt ) ) continue;
      if ( not reg().all_of<Cmp::Wall, Cmp::Player::NoPath>( neighbour_entt ) ) continue;
      neighbour_count++;

      // don't surround on four sides
      if ( neighbour_count > 3 ) { kill_list.emplace_back( neighbour_entt, rune_pos_cmp ); }
    }
  }

  for ( auto [entt, pos_cmp] : kill_list )
  {
    non_obstacle_spatialgrid.remove( entt, pos_cmp );
    reg().destroy( entt );
  }
}

void RuinSystem::creaking_rope_update()
{
  static float m_creaking_rope_swing_freq{ 0.075f }; // oscillations per second
  auto t = m_creaking_rope_swing_timer.getElapsedTime().asSeconds();

  float stereo_pan_value = std::sin( t * m_creaking_rope_swing_freq * std::numbers::pi );

  // SPDLOG_INFO( "creaking_rope_update: {}", stereo_pan_value );
  m_sound_bank.get_music( "ruin_creaking_rope" ).setPan( stereo_pan_value );
}

bool RuinSystem::check_activate_player_curse( sf::Vector2f scene_dimensions )
{
  Cmp::Player::Curse &player_curse = Utils::Player::get_curse( reg() );

  auto npc_shadowhand_cmp = Sys::NpcStore::instance().get_item( "npc.shadowhand" );
  const auto &hand_ms = m_sprite_factory.get_spritesheet_by_type( npc_shadowhand_cmp.sprite_type_list.front() );

  auto [_, inventory_type, _] = Utils::Player::get_inventory( m_reg );
  if ( not player_curse.active && inventory_type == "item.witchesjar" )
  {
    if ( not m_curse_activation_future.valid() )
    {
      m_sound_bank.get_music( "ruin_creaking_rope" ).stop();
      m_sound_bank.get_music( "ruin_music" ).stop();

      if ( m_sound_bank.get_effect( "witch_scream" ).getStatus() != sf::Sound::Status::Playing ) { m_sound_bank.get_effect( "witch_scream" ).play(); }
      if ( m_sound_bank.get_effect( "banging_smashing_sounds" ).getStatus() != sf::Sound::Status::Playing )
      {
        m_sound_bank.get_effect( "banging_smashing_sounds" ).play();
      }
      player_curse.active = true; // prevent the active curse from being re-activated

      Factory::Ruin::create_shadow_hand( m_reg, scene_dimensions, hand_ms );
      m_curse_activation_future = std::async( std::launch::async, []() { std::this_thread::sleep_for( std::chrono::seconds( 5 ) ); } );
    }

    // Check each frame if the async delay has completed
    if ( m_curse_activation_future.valid() && m_curse_activation_future.wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready )
    {
      // do something at the end of the curse activation
    }
  }

  return player_curse.active;
}

void RuinSystem::update_shadow_hand_pos( sf::Vector2f scene_dimensions )
{
  if ( not Utils::Player::get_curse( reg() ).active ) return;
  auto npc_shadowhand_cmp = Sys::NpcStore::instance().get_item( "npc.shadowhand" );

  const auto &hand_ms = m_sprite_factory.get_spritesheet_by_type( npc_shadowhand_cmp.sprite_type_list.front() );
  const auto hand_ms_size = hand_ms.get_sprite_size();
  float max_shadow_hand_xpos = scene_dimensions.x - hand_ms_size.x;

  float shadow_hand_speed = 0.45f;

  for ( auto [hand_entt, hand_cmp, hand_pos] : reg().view<Cmp::Npc::ShadowHand, Cmp::Position>().each() )
  {
    if ( hand_pos.position.x + shadow_hand_speed < max_shadow_hand_xpos ) { hand_pos.position.x += shadow_hand_speed; }
  }
}

void RuinSystem::check_player_shadow_hand_collision( sf::Time dt )
{
  if ( not Utils::scene_setting<Cmp::SceneSettings::CollisionDetection>( reg() ).enabled ) return;

  static constexpr float kActionEffectInterval = 0.05f;
  m_shadowhand_action_effects_time += dt;
  if ( m_shadowhand_action_effects_time.asSeconds() < kActionEffectInterval ) { return; }

  auto npc_shadowhand_cmp = Sys::NpcStore::instance().get_item( "npc.shadowhand" );
  auto npc_collision_action = npc_shadowhand_cmp.actions.at( std::type_index( typeid( Cmp::CollisionAction ) ) );

  // only trigger PlayerMortalityEvents if player is alive
  if ( Utils::Player::get_mortality( reg() ).state == Cmp::Player::Mortality::State::DEAD ) { return; }

  const auto player_pos = Utils::Player::get_position( reg() );
  if ( Utils::Collision::check_cmp<Cmp::Npc::NPC>( reg(), Cmp::RectBounds::scaled( player_pos.position, Constants::kGridSizePxF, 1.f ) ) )
  {
    // damage player
    Utils::Player::get_player_stats( reg() ).apply_modifiers( npc_collision_action.action );
  }
  if ( Utils::Player::get_player_stats( reg() ).health() <= 0 )
  {
    get_systems_event_queue().enqueue( Events::PlayerMortalityEvent( Cmp::Player::Mortality::State::SHADOWCURSED, player_pos ) );
  }

  m_shadowhand_action_effects_time = sf::Time::Zero;
}

void RuinSystem::reset_player_curse()
{
  Utils::Player::reset_curse( reg() );
  m_curse_activation_future = std::future<void>{}; // allow re-entrant scene to trigger the future again
}

void RuinSystem::create_spiders( sf::FloatRect scene_boundary )
{
  auto reserved_sm = m_reserved_sm.lock();
  auto has_collision = [&]( const Cmp::RectBounds &pos )
  {
    if ( Utils::Collision::check_cmp<Cmp::Npc::NoPathFinding>( reg(), pos ) ) { return true; }
    if ( Utils::Collision::check_cmp<Cmp::Ruin::StairsSegment>( reg(), pos ) ) { return true; }
    if ( reserved_sm && not reserved_sm->query_rect( pos.getBounds() ).empty() ) { return true; }

    // ensure spider is inside scene
    if ( not Cmp::RectBounds::scaled( pos.position(), pos.size(), 1.5f ).findIntersection( scene_boundary ) ) { return true; }
    return false;
  };

  const auto kMaxSpiderCount = Sys::PersistSystem::get<Cmp::Persist::RuinMaxSpiders>( m_reg ).get_value();

  // keep tryng to add spiders until we hit our target or exceed max attempts.
  int max_attempts = 100;
  for ( auto _ : std::views::iota( 0, max_attempts ) )
  {
    size_t spiders_count = 0;
    for ( auto [npc_entt, npc_cmp, npc_sprite_cmp] : reg().view<Cmp::Npc::NPC, Cmp::AnimData>().each() )
    {
      if ( npc_sprite_cmp.m_sprite_type == "sprite.spider" ) { spiders_count++; }
    }
    if ( spiders_count >= kMaxSpiderCount ) break;

    for ( auto _ : std::views::iota( uint16_t{ 0 }, kMaxSpiderCount ) )
    {
      auto [rnd_entt, rnd_pos_cmp] = Utils::Rnd::get_random_position( reg(), {}, {} );
      if ( has_collision( Cmp::RectBounds::scaled( rnd_pos_cmp.position, rnd_pos_cmp.size, 1 ) ) ) continue;

      auto new_spider_entity = reg().create();
      Cmp::Position position_cmp = reg().emplace<Cmp::Position>( new_spider_entity, rnd_pos_cmp.position, rnd_pos_cmp.size );
      [[maybe_unused]] Cmp::ZOrderValue zorder_cmp = reg().emplace<Cmp::ZOrderValue>( new_spider_entity, position_cmp.position.y );
      Factory::Npc::create_npc( reg(), new_spider_entity, "npc.spider" );
    }
  }
}

void RuinSystem::check_create_witch( sf::FloatRect scene_boundary )
{
  auto has_collision = [&]( const Cmp::RectBounds &pos )
  {
    if ( Utils::Collision::check_cmp<Cmp::Ruin::Bookcase>( reg(), pos ) ) { return true; }
    if ( Utils::Collision::check_cmp<Cmp::Npc::NoPathFinding>( reg(), pos ) ) { return true; }
    if ( Utils::Collision::check_cmp<Cmp::Ruin::StairsSegment>( reg(), pos ) ) { return true; }

    // ensure spider is inside scene
    if ( not Cmp::RectBounds::scaled( pos.position(), pos.size(), 1.5f ).findIntersection( scene_boundary ) ) { return true; }
    return false;
  };

  bool witch_exists = false;
  for ( auto [npc_entt, npc_cmp, npc_sprite_cmp] : reg().view<Cmp::Npc::NPC, Cmp::AnimData>().each() )
  {
    if ( npc_sprite_cmp.m_sprite_type == "sprite.witch" ) { witch_exists = true; }
  }

  if ( not witch_exists )
  {
    for ( auto _ : std::views::iota( 0, 100 ) )
    {
      auto [rnd_entt, rnd_pos_cmp] = Utils::Rnd::get_random_position( reg(), {}, {} );
      if ( has_collision( Cmp::RectBounds::scaled( rnd_pos_cmp.position, rnd_pos_cmp.size, 1 ) ) ) continue;

      auto new_witch_entity = reg().create();
      Cmp::Position position_cmp = reg().emplace<Cmp::Position>( new_witch_entity, rnd_pos_cmp.position, rnd_pos_cmp.size );
      [[maybe_unused]] Cmp::ZOrderValue zorder_cmp = reg().emplace<Cmp::ZOrderValue>( new_witch_entity, position_cmp.position.y );
      Factory::Npc::create_npc( reg(), new_witch_entity, "npc.witch" );
      break;
    }
  }
}

} // namespace Game::Sys