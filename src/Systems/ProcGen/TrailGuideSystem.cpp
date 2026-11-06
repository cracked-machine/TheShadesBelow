#include <Components/Altar/Activated.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/TrailGuide.hpp>
#include <Components/Grave/TrailStart.hpp>
#include <Components/Persistent/GraveTrailGuideColour.hpp>
#include <Components/Position.hpp>
#include <Components/ZOrderValue.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/ProcGen/TrailGuideSystem.hpp>

namespace Game::Sys::ProcGen
{

namespace
{

constexpr float kTrailGuideRepeat = 160.f; // px between consecutive trains
constexpr float kTrailGuideZOrder = -12.f; // above the floor, below sprites (same as the dowsing rod guide)

} // namespace

void TrailGuideSystem::update( sf::Time dt )
{
  auto guide_view = reg().view<Cmp::Altar::TrailGuide>();
  auto destroy_guide = [&]()
  {
    for ( auto guide_entt : guide_view )
      reg().destroy( guide_entt );
  };

  auto altar_view = reg().view<Cmp::Altar::MultiBlock, Cmp::Altar::Activated>();
  auto start_view = reg().view<Cmp::Grave::TrailStart, Cmp::Position>();
  if ( altar_view.begin() == altar_view.end() or start_view.begin() == start_view.end() )
  {
    destroy_guide();
    return;
  }

  entt::entity guide_entt = guide_view.empty() ? entt::null : guide_view.front();
  if ( guide_entt == entt::null )
  {
    guide_entt = reg().create();
    reg().emplace<Cmp::Altar::TrailGuide>( guide_entt );
    reg().emplace<Cmp::ZOrderValue>( guide_entt, kTrailGuideZOrder );
  }
  auto &guide_cmp = reg().get<Cmp::Altar::TrailGuide>( guide_entt );

  clear( guide_cmp.paths );
  const sf::Color colour = Sys::PersistSystem::get<Cmp::Persist::GraveTrailGuideColour>( reg() );
  for ( auto [altar_entt, altar_cmp] : altar_view.each() )
  {
    for ( auto [grave_entt, grave_pos_cmp] : start_view.each() )
    {
      add_dogleg( guide_cmp.paths, altar_cmp.getCenter(), grave_pos_cmp.getCenter(), colour );
    }
  }

  guide_cmp.pulse_time += dt;
  update_markers( guide_cmp.paths, guide_cmp.pulse_time, kTrailGuideRepeat );
}

} // namespace Game::Sys::ProcGen
