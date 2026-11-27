#include <Audio/SoundBank.hpp>
#include <Components/AnimData.hpp>
#include <Components/Cart/Barrel.hpp>
#include <Components/Direction.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/StackCount.hpp>
#include <Components/LastDirection.hpp>
#include <Components/Persistent/PlayerStartPosition.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/SceneSettings/CollisionDetection.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <Components/SceneSettings/Footsteps.hpp>
#include <Components/SceneSettings/Paused.hpp>
#include <Components/SceneSettings/Shaders.hpp>
#include <Components/SceneSettings/ShowDebugStats.hpp>
#include <Components/SceneSettings/ShowNavmesh.hpp>
#include <Components/SceneSettings/ShowPathFinding.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/UpdatePlayerPositionEvent.hpp>
#include <Factory/PathfindingFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <Factory/ShaderFactory.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <SceneControl/SceneData.hpp>
#include <SceneControl/Scenes/LevelCompleteScene.hpp>
#include <Systems/AnimSystem.hpp>
#include <Systems/BaseSystem.hpp>
#include <Systems/FootstepSystem.hpp>
#include <Systems/InventorySystem.hpp>
#include <Systems/ItemSystem.hpp>
#include <Systems/PersistSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/PlayerSystem.hpp>
#include <Systems/ProcGen/LevelGenerator.hpp>
#include <Systems/Render/CartOverlaySystem.hpp>
#include <Systems/Render/DebugOverlaySystem.hpp>
#include <Systems/Render/RenderGameSystem.hpp>
#include <Systems/ShaderSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Systems/Threats/NpcSystem.hpp>
#include <Utils/Player.hpp>

#include <SFML/System/Time.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace Game::Scene
{

namespace
{
//! @brief Wealth for each cadaver part in a barrel.
constexpr int kPartWealth = 1;
//! @brief Bonus wealth for each full barrel.
constexpr int kFullBarrelWealth = 2;
//! @brief Wealth for each spare cadaver part in the inventory, as for a part in a barrel.
constexpr int kSparePartWealth = kPartWealth;
//! @brief Wealth for each preserved cat in the inventory.
constexpr int kPreservedCatWealth = 5;
//! @brief The item scored (and consumed) from the inventory.
constexpr const char *kPreservedCat = "item.preservedcat";
//! @brief The preserved cat's sprite, for the row of cats on the victory screen.
constexpr const char *kPreservedCatSprite = "sprite.item.preservedcat";
//! @brief Seconds after the last step before its barrel/item leaves the table and the panels dim.
constexpr float kHandOverDelay = 0.5f;
//! @brief Seconds per flash of the continue prompt: shown for the first half, hidden for the second.
constexpr float kContinueFlashPeriod = 1.f;
} // namespace

void LevelCompleteScene::on_init()
{
  auto &persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  persistent_sys.initialize_component_registry();
  persistent_sys.load_state();

  m_scene_data = std::make_shared<SceneData>( "res/scenes/levelcomplete.json" );

  auto scene_settings_entt = m_reg.create();
  m_reg.emplace_or_replace<Cmp::SceneSettings::CurrentScene>( scene_settings_entt, Cmp::SceneSettings::SceneId::LEVEL_COMPLETE );
  m_reg.emplace_or_replace<Cmp::SceneSettings::CollisionDetection>( scene_settings_entt, true );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowPathFinding>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowNavmesh>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::ShowDebugStats>( scene_settings_entt, false );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Paused>( scene_settings_entt, Cmp::SceneSettings::Paused::Reason::NONE );
  m_reg.emplace_or_replace<Cmp::SceneSettings::Shaders>( scene_settings_entt, true );
  // the player stands still for the whole scene
  m_reg.emplace_or_replace<Cmp::SceneSettings::Footsteps>( scene_settings_entt, false );

  // initialise the persistent player start position from the scene configuration (json) data
  auto [_, player_start_pos_px] = m_scene_data->get_player_start_position();
  Sys::PersistSystem::add<Cmp::Persist::PlayerStartPosition>( m_reg, player_start_pos_px );

  auto [map_size_grid, map_size_pixel] = m_scene_data->map_size();
  m_room_centre = map_size_pixel / 2.f;
  Factory::Shader::add_night_static( m_sys.find<Sys::Store::Type::ShaderSystem>(), map_size_pixel );

  // Positions reserved from procgen/algorithmic changes. Must exist before generation starts.
  m_reserved_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  m_item_sm = std::make_shared<PathFinding::SpatialHashGrid>();
  m_sys.find<Sys::Store::Type::ItemSystem>().init( m_reserved_sm, m_item_sm );

  // walls, the table and Dr Knox
  auto &random_level_sys = m_sys.find<Sys::Store::Type::LevelGenerator>();
  random_level_sys.init( m_reserved_sm, m_item_sm );
  random_level_sys.build_scene_from_data( *m_scene_data );

  m_floormap.create( random_level_sys.get_void_sm(), m_scene_data );
  auto floor_entity = m_reg.create();
  m_reg.emplace<Sprites::Containers::VertexFloor>( floor_entity, m_floormap );
  m_reg.emplace<Cmp::ZOrderValue>( floor_entity, -16.f );

  // create navmeshes for pathfinding
  m_generic_npc_navmesh = Factory::Pathfinding::create_npc_navmesh( m_reg );
  m_open_navmesh = Factory::Pathfinding::create_open_navmesh( m_reg );
  m_loot_sm = Factory::Pathfinding::create_loot_grid( m_reg );
  reinit_navmesh();
}

void LevelCompleteScene::on_enter()
{
  SPDLOG_DEBUG( "Entering {}", get_name() );
  auto &persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  persistent_sys.initialize_component_registry();
  persistent_sys.load_state();
  m_sound_bank.stop_all();

  auto &render_game_sys = m_sys.find<Sys::Store::Type::RenderGameSystem>();
  render_game_sys.init_world_view();
  render_game_sys.lock_camera( m_room_centre );

  // the player is moved through PlayerSystem, which needs this scene's navmeshes to keep them in sync
  reinit_navmesh();

  if ( m_just_spawned )
  {
    Sys::BaseSystem::get_systems_event_queue().trigger(
        Events::UpdatePlayerPositionEvent( sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::PlayerStartPosition>( m_reg ) ) ) );
    m_just_spawned = false;
  }

  // face the table. PlayerSystem never runs here, so set the standing sprite directly.
  Utils::Player::get_last_direction( m_reg ) = Cmp::LastDirection( { 1.f, 0.f } );
  auto &anim_cmp = m_reg.get<Cmp::AnimData>( Utils::Player::get_entity( m_reg ) );
  anim_cmp.m_sprite_type = "sprite.player.walk.east";
  anim_cmp.m_flip_x = false;
  anim_cmp.m_current_frame = 0;
  anim_cmp.m_enabled = false;
  hold_player();

  build_steps();
}

void LevelCompleteScene::on_exit()
{
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();
  m_sys.find<Sys::Store::Type::RenderGameSystem>().unlock_camera();
  // the barrels end with this scene: the next level's graveyard creates its own
  auto barrel_view = m_reg.view<Cmp::Cart::Barrel>();
  m_reg.destroy( barrel_view.begin(), barrel_view.end() );
  m_reg.clear();
}

void LevelCompleteScene::build_steps()
{
  m_barrels.clear();
  for ( auto [barrel_entt, barrel_cmp] : m_reg.view<Cmp::Cart::Barrel>().each() )
  {
    m_barrels.push_back( barrel_cmp );
  }
  // barrels fill from the left (see Sys::CartHorseSystem::transfer_selected_item()), so a barrel always holds at least
  // the parts of the barrel to its right. Restore that order, whatever order the transfer left the entities in.
  // Lambda projection, not &Barrel::filled_count: GCC -O3 flags the member-function-pointer call's virtual-dispatch
  // check as reading past the 6-byte Barrel (-Warray-bounds false positive).
  std::ranges::stable_sort( m_barrels, std::ranges::greater{}, []( const Cmp::Cart::Barrel &barrel ) { return barrel.filled_count(); } );

  m_steps.clear();
  for ( std::size_t barrel_idx = 0; barrel_idx < m_barrels.size(); ++barrel_idx )
  {
    const auto &barrel = m_barrels.at( barrel_idx );
    for ( std::size_t part_idx = 0; part_idx < Cmp::Cart::Barrel::kParts.size(); ++part_idx )
    {
      if ( barrel.*Cmp::Cart::Barrel::kParts.at( part_idx ).second )
      {
        m_steps.push_back( { .type = Step::Type::PART, .barrel = barrel_idx, .part = part_idx } );
      }
    }
    if ( barrel.is_full() ) { m_steps.push_back( { .type = Step::Type::FULL_BARREL, .barrel = barrel_idx } ); }
  }

  // items held in a slot, counting a stack as its size
  auto slot_item_count = [this]( entt::entity slot_entt )
  {
    const auto *stack_cmp = m_reg.try_get<Cmp::Inventory::StackCount>( slot_entt );
    return stack_cmp ? static_cast<std::size_t>( std::max( stack_cmp->m_count, 1 ) ) : std::size_t{ 1 };
  };

  m_cat_sprites.clear();
  for ( auto [slot_entt, slot_cmp] : m_reg.view<Cmp::PlayerInventorySlot>().each() )
  {
    if ( slot_cmp.empty() or slot_cmp.m_item->item_type != kPreservedCat ) continue;
    m_cat_sprites.insert( m_cat_sprites.end(), slot_item_count( slot_entt ), Sys::SpriteKey( kPreservedCatSprite ) );
  }
  for ( std::size_t cat_idx = 0; cat_idx < m_cat_sprites.size(); ++cat_idx )
  {
    m_steps.push_back( { .type = Step::Type::CAT, .cat = cat_idx } );
  }

  // spare cadaver parts: any not put in a barrel. Grouped by part, in the barrel's scoring order.
  m_spare_sprites.clear();
  m_spare_items.clear();
  for ( const auto &[part_sprite, part_member] : Cmp::Cart::Barrel::kParts )
  {
    for ( auto [slot_entt, slot_cmp] : m_reg.view<Cmp::PlayerInventorySlot>().each() )
    {
      if ( slot_cmp.empty() or slot_cmp.m_item->sprite_type.str() != part_sprite ) continue;
      const std::size_t count = slot_item_count( slot_entt );
      m_spare_sprites.insert( m_spare_sprites.end(), count, slot_cmp.m_item->sprite_type );
      m_spare_items.insert( m_spare_items.end(), count, slot_cmp.m_item->item_type );
    }
  }
  for ( std::size_t spare_idx = 0; spare_idx < m_spare_sprites.size(); ++spare_idx )
  {
    m_steps.push_back( { .type = Step::Type::SPARE_PART, .spare = spare_idx } );
  }

  m_next_step = 0;
  m_scorecheck_accumulator = sf::Time::Zero;
  m_score_wealth = 0;
  m_parts_counted = 0;
  m_full_barrels_counted = 0;
  m_cats_counted = 0;
  m_spares_counted = 0;
}

void LevelCompleteScene::run_next_step()
{
  const Step &step = m_steps.at( m_next_step++ );
  int added = 0;
  switch ( step.type )
  {
    case Step::Type::PART:
      added = kPartWealth;
      m_parts_counted++;
      break;
    case Step::Type::FULL_BARREL:
      added = kFullBarrelWealth;
      m_full_barrels_counted++;
      break;
    case Step::Type::CAT:
      added = kPreservedCatWealth;
      m_cats_counted++;
      Factory::Player::destroy_inventory( m_reg, kPreservedCat );
      break;
    case Step::Type::SPARE_PART:
      added = kSparePartWealth;
      m_spares_counted++;
      Factory::Player::destroy_inventory( m_reg, m_spare_items.at( step.spare ) );
      break;
  }
  Utils::Player::get_wealth( m_reg ).wealth += added;
  m_score_wealth += added;
  m_sound_bank.get_effect( "get_loot" ).play();
}

void LevelCompleteScene::do_update( sf::Time dt )
{
  hold_player();
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();

  // the accumulator only restarts when a step runs, so once everything is counted it keeps counting up (see
  // kHandOverDelay) rather than flicking back to the last step every second
  static constexpr float kTallyStepHz = 1.0f;
  m_scorecheck_accumulator += dt;
  if ( m_next_step < m_steps.size() and m_scorecheck_accumulator.asSeconds() >= 1.f / kTallyStepHz )
  {
    run_next_step();
    m_scorecheck_accumulator = sf::Time::Zero;
  }

  m_sys.find<Sys::Store::Type::AnimSystem>().update( dt );
  m_sys.find<Sys::Store::Type::ShaderSystem>().update( dt );
  m_sys.find<Sys::Store::Type::RenderGameSystem>().render_game( dt );

  render( dt );
}

void LevelCompleteScene::hold_player()
{
  // PlayerSystem::update() is never called in this scene, so nothing moves the player; also drop any held direction
  Utils::Player::get_direction( m_reg ) = Cmp::Direction{ { 0.f, 0.f } };
}

std::pair<std::optional<Sys::SpriteKey>, sf::Color> LevelCompleteScene::table_item( bool all_counted ) const
{
  const sf::Color highlight = Sys::CartOverlaySystem::kTallyHighlight;
  // nothing handed over yet, or the last item has gone to Dr Knox
  if ( m_next_step == 0 ) { return { std::nullopt, highlight }; }
  if ( all_counted and m_scorecheck_accumulator.asSeconds() >= kHandOverDelay ) { return { std::nullopt, highlight }; }

  // one item at a time: the one being counted
  const Step &step = m_steps.at( m_next_step - 1 );
  switch ( step.type )
  {
    case Step::Type::PART:
      return { Sys::SpriteKey( std::string( Cmp::Cart::Barrel::kParts.at( step.part ).first ) ), highlight };
    case Step::Type::FULL_BARREL:
      // the bonus has no item of its own: the table is clear while the cart panel highlights the whole barrel
      return { std::nullopt, highlight };
    case Step::Type::CAT:
      return { m_cat_sprites.at( step.cat ), highlight };
    case Step::Type::SPARE_PART:
      return { m_spare_sprites.at( step.spare ), Sys::CartOverlaySystem::kSpareHighlight };
  }
  return { std::nullopt, highlight };
}

void LevelCompleteScene::render( sf::Time dt )
{
  const bool allow_continue = m_next_step >= m_steps.size();

  // highlight the step just run (m_next_step - 1); everything before it is counted
  using BarrelProgress = Sys::CartOverlaySystem::BarrelProgress;
  using IconProgress = Sys::CartOverlaySystem::IconProgress;
  std::optional<BarrelProgress> barrel_progress;
  std::optional<IconProgress> cat_progress;
  std::optional<IconProgress> spare_progress;
  if ( m_next_step > 0 )
  {
    const Step &step = m_steps.at( m_next_step - 1 );
    switch ( step.type )
    {
      case Step::Type::PART:
        barrel_progress = BarrelProgress{ .barrel = step.barrel, .part = step.part };
        break;
      case Step::Type::FULL_BARREL:
        barrel_progress = BarrelProgress{ .barrel = step.barrel, .whole_barrel = true };
        break;
      case Step::Type::CAT:
        barrel_progress = BarrelProgress{ .barrel = m_barrels.size() };
        cat_progress = IconProgress{ .current = step.cat };
        break;
      case Step::Type::SPARE_PART:
        barrel_progress = BarrelProgress{ .barrel = m_barrels.size() };
        cat_progress = IconProgress{ .current = m_cat_sprites.size() };
        spare_progress = IconProgress{ .current = step.spare };
        break;
    }
    // once everything is counted, dim the lot rather than leave the last step highlighted
    if ( allow_continue and m_scorecheck_accumulator.asSeconds() >= kHandOverDelay )
    {
      barrel_progress = BarrelProgress{ .barrel = m_barrels.size() };
      cat_progress = IconProgress{ .current = m_cat_sprites.size() };
      spare_progress = IconProgress{ .current = m_spare_sprites.size() };
    }
  }

  using IconRow = Sys::CartOverlaySystem::IconRow;
  const std::array<IconRow, 2> icon_rows{ {
      { .sprites = m_cat_sprites, .progress = cat_progress },
      { .sprites = m_spare_sprites, .progress = spare_progress, .highlight = Sys::CartOverlaySystem::kSpareHighlight },
  } };

  const auto [item, item_highlight] = table_item( allow_continue );
  // once everything is counted, flash the continue prompt once a second: shown for the first half of each second. The
  // accumulator keeps counting up after the last step, see do_update().
  const bool show_continue = allow_continue and std::fmod( m_scorecheck_accumulator.asSeconds(), kContinueFlashPeriod ) < kContinueFlashPeriod / 2.f;
  m_sys.find<Sys::Store::Type::CartOverlaySystem>().render_level_tally( dt, m_barrels, barrel_progress, icon_rows, show_continue,
                                                                        { .score_wealth = m_score_wealth,
                                                                          .parts = m_parts_counted,
                                                                          .full_barrels = m_full_barrels_counted,
                                                                          .cats = m_cats_counted,
                                                                          .spare_parts = m_spares_counted },
                                                                        item, item_highlight );
}

void LevelCompleteScene::reinit_navmesh()
{
  m_sys.find<Sys::Store::Type::NpcSystem>().init( m_generic_npc_navmesh, m_open_navmesh, nullptr, nullptr, m_loot_sm, m_item_sm );
  m_sys.find<Sys::Store::Type::ItemSystem>().init( m_reserved_sm, m_item_sm, m_generic_npc_navmesh, nullptr, m_loot_sm );
  m_sys.find<Sys::Store::Type::InventorySystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_reserved_sm, m_item_sm, m_loot_sm, m_ghost_navmesh );
  m_sys.find<Sys::Store::Type::PlayerSystem>().init( m_generic_npc_navmesh, m_player_navmesh, m_open_navmesh );
  m_sys.find<Sys::Store::Type::DebugOverlaySystem>().init( m_generic_npc_navmesh, m_reserved_sm, m_item_sm, nullptr );
}

entt::registry &LevelCompleteScene::registry() { return m_reg; }

} // namespace Game::Scene
