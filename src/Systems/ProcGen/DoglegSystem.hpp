#ifndef SRC_SYSTEMS_PROCGEN_DOGLEGSYSTEM_HPP__
#define SRC_SYSTEMS_PROCGEN_DOGLEGSYSTEM_HPP__

#include <Components/Player/Doglegs.hpp>
#include <Components/SceneSettings/CurrentScene.hpp>
#include <PathFinding/BreadthFirst.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Time.hpp>

// clang-format off
namespace Game::Cmp::Inventory { struct DowsingTarget; }
// clang-format on

namespace Game::Sys::ProcGen
{

//! @brief Builds the dowsing rod guide: a train of chevrons travelling from the player towards each landmark
//! matching the rod target. In the graveyard each path is an L-shaped "crow flies" dogleg; in the crypt it is the
//! walkable route along the passages, and a landmark that cannot be reached on foot gets no path. All paths share
//! one pulse, which restarts once it has reached the landmark or left the view on every path. Results are stored
//! in Cmp::Player::Doglegs for RenderGameSystem to draw.
class DoglegSystem : public BaseSystem
{
public:
  //! @brief Construct a new DoglegSystem object
  DoglegSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
      : BaseSystem( reg, window, sound_bank )
  {
  }

  //! @brief Set the navmesh used to route along the passages in the crypt
  //! @param npc_navmesh The scene's NPC navmesh, which tracks rooms and passages opening and closing
  void init( const PathFinding::SpatialHashGridSharedPtr &npc_navmesh );

  //! @brief Rebuild the doglegs and markers for this frame. Leaves both empty if the player holds no dowsing rod.
  //! @param dt Frame delta, advances the pulse
  void update( sf::Time dt );

private:
  //! @brief Add single L-shaped dogleg to the component
  //! @param source_pos
  //! @param target_pos
  //! @param color
  void add_dogleg( sf::Vector2f source_pos, sf::Vector2f target_pos, sf::Color color );

  //! @brief Add the walkable route from the player to a landmark, or nothing if there is no way there on foot
  //! @param source_pos Player centre
  //! @param target_pos
  //! @param color
  void add_route( sf::Vector2f source_pos, sf::Vector2f target_pos, sf::Color color );

  //! @brief Refresh `m_field` if the player has changed cell or it has gone stale
  //! @param dt Frame delta, ages the field
  void update_field( sf::Time dt );

  //! @brief Add a path from the player to each landmark matching the dowsing rod target
  void update_target( const Cmp::Inventory::DowsingTarget &dowsing_cmp );

  //! @brief Place the chevron markers along each dogleg for the current pulse time
  void update_markers( Cmp::Inventory::DowsingTarget &dowsing_cmp );

  //! @brief Clear doglegs and markers from the component
  void clear();

  //! @brief Distance from the player to where the path leaves the view, or the whole path if the target is in view
  static float visible_length( const Cmp::Player::Doglegs::Dogleg &dogleg, const sf::FloatRect &view_bounds );

  //! @brief Which scene is updating, selects the landmarks and the kind of path
  Cmp::SceneSettings::SceneId m_location{ Cmp::SceneSettings::SceneId::GRAVEYARD };

  //! @brief Navmesh searched to find the crypt routes
  PathFinding::SpatialHashGridWeakPtr m_npc_navmesh;
  //! @brief Every cell the player can walk to in the crypt, cached between frames. See update_field()
  PathFinding::DistanceField m_field;
  //! @brief Top-left of the player's cell when `m_field` was made
  sf::Vector2f m_field_cell;
  //! @brief Time since `m_field` was made
  sf::Time m_field_age{ sf::Time::Zero };
};

} // namespace Game::Sys::ProcGen

#endif // SRC_SYSTEMS_PROCGEN_DOGLEGSYSTEM_HPP__