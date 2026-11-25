#include <Audio/SoundBank.hpp>
#include <Components/Cart/Barrel.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/Inventory/StackCount.hpp>
#include <Components/Persistent/DisplayResolution.hpp>
#include <Components/Player/Wealth.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SceneControl/Events/ProcessLevelCompleteSceneInputEvent.hpp>
#include <SceneControl/Scenes/LevelCompleteScene.hpp>
#include <Systems/FootstepSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/PlayerSystem.hpp>
#include <Systems/Render/CartOverlaySystem.hpp>
#include <Systems/Render/RenderMenuSystem.hpp>
#include <Systems/Stores/SystemStore.hpp>
#include <Utils/Player.hpp>

#include <SFML/System/Time.hpp>

#include <algorithm>
#include <array>
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
} // namespace

void LevelCompleteScene::on_init() { /* init entities */ }
void LevelCompleteScene::on_enter()
{
  SPDLOG_DEBUG( "Entering {}", get_name() );
  auto &persistent_sys = m_sys.find<Sys::Store::Type::PersistSystem>();
  persistent_sys.initialize_component_registry();
  persistent_sys.load_state();
  m_sound_bank.stop_all();
  build_steps();
}

void LevelCompleteScene::on_exit()
{
  SPDLOG_DEBUG( "Exiting {}", get_name() );
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();
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
  std::ranges::stable_sort( m_barrels, std::ranges::greater{}, &Cmp::Cart::Barrel::filled_count );

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
  m_sys.find<Sys::Store::Type::FootstepSystem>().stop_footsteps_sound();

  static constexpr float kTallyStepHz = 1.0f;
  m_scorecheck_accumulator += dt;
  if ( m_scorecheck_accumulator.asSeconds() >= 1.f / kTallyStepHz )
  {
    if ( m_next_step < m_steps.size() ) { run_next_step(); }
    m_scorecheck_accumulator = sf::Time::Zero;
  }

  render( dt );
}

void LevelCompleteScene::render( sf::Time dt )
{
  const bool allow_continue = m_next_step >= m_steps.size();
  auto &render_menu_sys = m_sys.find<Sys::Store::Type::RenderMenuSystem>();
  render_menu_sys.render_victory_screen( allow_continue, { .score_wealth = m_score_wealth,
                                                           .parts = m_parts_counted,
                                                           .full_barrels = m_full_barrels_counted,
                                                           .cats = m_cats_counted,
                                                           .spare_parts = m_spares_counted } );

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
    if ( allow_continue and m_scorecheck_accumulator.asSeconds() >= 0.5f )
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

  const auto display_size = sf::Vector2f( Sys::PersistSystem::get<Cmp::Persist::DisplayResolution>( m_reg ) );
  m_sys.find<Sys::Store::Type::CartOverlaySystem>().render_level_tally( dt, m_barrels, barrel_progress, icon_rows,
                                                                        { display_size.x / 2.f, display_size.y * 0.3f } );
}

entt::registry &LevelCompleteScene::registry() { return m_reg; }

} // namespace Game::Scene
