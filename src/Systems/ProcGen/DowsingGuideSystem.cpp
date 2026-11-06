#include <Components/Altar/MultiBlock.hpp>
#include <Components/Crypt/Chest.hpp>
#include <Components/Crypt/Entrance.hpp>
#include <Components/Crypt/Lever.hpp>
#include <Components/Exit.hpp>
#include <Components/Inventory/DowsingTarget.hpp>
#include <Components/Inventory/WearLevel.hpp>
#include <Components/Player/Doglegs.hpp>
#include <Components/Position.hpp>
#include <PathFinding/BreadthFirst.hpp>
#include <PathFinding/SpatialHashGrid.hpp>
#include <Systems/ProcGen/DowsingGuideSystem.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <vector>

namespace Game::Sys::ProcGen
{

namespace
{

constexpr sf::Color kAltarColour{ 255, 255, 0, 255 };
constexpr sf::Color kCryptColour{ 255, 0, 0, 255 };
constexpr sf::Color kExitColour{ 0, 255, 0, 255 };
const sf::Time kFieldRefresh = sf::seconds( 0.25f ); // how stale the crypt routes may get while the player stands still

} // namespace

void DowsingGuideSystem::init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh )
{
  m_npc_navmesh = npc_navmesh;
  // a field belongs to the navmesh it was made from
  m_field.clear();
}

void DowsingGuideSystem::update( sf::Time dt )
{
  // always clear first, so nothing stale is left to render once the rod leaves the inventory
  auto &doglegs = Utils::Player::get_doglegs( reg() );
  clear( doglegs );

  m_location = Utils::scene_setting<Cmp::SceneSettings::CurrentScene>( reg() ).id;

  auto [inventory_entt, _, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_entt == entt::null ) return;
  auto *dowsing_cmp = reg().try_get<Cmp::Inventory::DowsingTarget>( inventory_entt );
  if ( not dowsing_cmp ) return;

  if ( Utils::Player::get_inventory_wear_level( reg() ) <= 0 ) return;

  dowsing_cmp->m_dowsing_pulse_time += dt;
  if ( m_location == Cmp::SceneSettings::SceneId::CRYPT ) update_field( dt );
  update_target( *dowsing_cmp );
  update_markers( doglegs, dowsing_cmp->m_dowsing_pulse_time );
}

void DowsingGuideSystem::add_route( sf::Vector2f source_pos, sf::Vector2f target_pos, sf::Color color )
{
  const std::vector<sf::Vector2f> route = PathFinding::route_to( m_field, target_pos );
  if ( route.empty() ) return; // no way there on foot

  // The route runs between cell centres, but the player moves sub-grid: leave from the player and step sideways
  // onto the line of the first leg, rather than cutting a diagonal that could clip a passage corner.
  std::vector<sf::Vector2f> points{ source_pos };
  if ( route.size() == 1 ) { points.push_back( target_pos ); }
  else
  {
    const bool first_leg_horizontal = route[0].y == route[1].y;
    const sf::Vector2f join = first_leg_horizontal ? sf::Vector2f{ source_pos.x, route[0].y } : sf::Vector2f{ route[0].x, source_pos.y };
    if ( join != source_pos ) points.push_back( join );
    points.insert( points.end(), route.begin() + 1, route.end() );
  }

  Utils::Player::get_doglegs( reg() ).m_doglegs.push_back( { std::move( points ), color } );
}

void DowsingGuideSystem::update_field( sf::Time dt )
{
  auto navmesh = m_npc_navmesh.lock();
  if ( not navmesh )
  {
    m_field.clear();
    return;
  }

  // Searching the whole crypt every frame is wasteful: redo it when the player changes cell, and every so often
  // besides, since rooms and passages open and close around a player who is standing still.
  const sf::Vector2f player_centre = Utils::Player::get_position( reg() ).getCenter();
  const sf::Vector2f player_cell = Utils::snap_to_grid( player_centre, Utils::Rounding::TOWARDS_ZERO );
  m_field_age += dt;
  if ( not m_field.empty() and player_cell == m_field_cell and m_field_age < kFieldRefresh ) return;

  m_field = PathFinding::breadth_first_field( reg(), *navmesh, player_centre );
  m_field_cell = player_cell;
  m_field_age = sf::Time::Zero;
}

void DowsingGuideSystem::update_target( const Cmp::Inventory::DowsingTarget &dowsing_cmp )
{

  using Target = Cmp::Inventory::DowsingTarget::Target;

  auto pos_cmp = Utils::Player::get_position( reg() );
  switch ( dowsing_cmp.target )
  {
    case Target::YELLOW: {
      switch ( m_location )
      {
        case Cmp::SceneSettings::SceneId::GRAVEYARD: {
          auto altar_view = reg().view<Cmp::Altar::MultiBlock>();
          for ( auto [altar_entt, altar_cmp] : altar_view.each() )
          {
            add_dogleg( Utils::Player::get_doglegs( reg() ), pos_cmp.getCenter(), altar_cmp.getCenter(), kAltarColour );
          }
          break;
        }
        case Cmp::SceneSettings::SceneId::CRYPT: {
          auto crypt_chest_view = reg().view<Cmp::Crypt::Chest, Cmp::Position>();
          for ( auto [chest_entt, chest_cmp, chest_pos] : crypt_chest_view.each() )
          {
            if ( chest_cmp.open ) continue;
            add_route( pos_cmp.getCenter(), chest_pos.getCenter(), kAltarColour );
          }
          break;
        }
        default:
          break;
      }
      break;
    }
    case Target::RED: {
      switch ( m_location )
      {
        case Cmp::SceneSettings::SceneId::GRAVEYARD: {
          auto crypt_entrance_view = reg().view<Cmp::Crypt::Entrance, Cmp::Position>();
          for ( auto [crypt_entt, crypt_cmp, crypt_pos_cmp] : crypt_entrance_view.each() )
          {
            add_dogleg( Utils::Player::get_doglegs( reg() ), pos_cmp.getCenter(), crypt_pos_cmp.getCenter(), kCryptColour );
          }
          break;
        }
        case Cmp::SceneSettings::SceneId::CRYPT: {
          auto crypt_lever_view = reg().view<Cmp::Crypt::Lever, Cmp::Position>();
          for ( auto [lever_entt, lever_cmp, lever_pos_cmp] : crypt_lever_view.each() )
          {
            if ( lever_cmp.isEnabled() ) continue;
            add_route( pos_cmp.getCenter(), lever_pos_cmp.getCenter(), kCryptColour );
          }
          break;
        }
        default:
          break;
      }
      break;
    }
    case Target::GREEN: {
      switch ( m_location )
      {
        case Cmp::SceneSettings::SceneId::GRAVEYARD: {
          auto graveyard_exit_view = reg().view<Cmp::Exit, Cmp::Position>();
          for ( auto [exit_entt, exit_cmp, exit_pos_cmp] : graveyard_exit_view.each() )
          {
            add_dogleg( Utils::Player::get_doglegs( reg() ), pos_cmp.getCenter(), exit_pos_cmp.getCenter(), kExitColour );
          }
          break;
        }
        case Cmp::SceneSettings::SceneId::CRYPT: {
          auto crypt_exit_view = reg().view<Cmp::Exit, Cmp::Position>();
          for ( auto [exit_entt, exit_cmp, exit_pos_cmp] : crypt_exit_view.each() )
          {
            add_route( pos_cmp.getCenter(), exit_pos_cmp.getCenter(), kExitColour );
          }
          break;
        }
        default:
          break;
      }
      break;
    }
    case Target::NONE: {
      break;
    }
  }
}

} // namespace Game::Sys::ProcGen