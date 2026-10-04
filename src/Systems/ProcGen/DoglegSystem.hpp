#ifndef SRC_SYSTEMS_PROCGEN_DOGLEGSYSTEM_HPP__
#define SRC_SYSTEMS_PROCGEN_DOGLEGSYSTEM_HPP__

#include <Components/Player/Doglegs.hpp>
#include <Systems/BaseSystem.hpp>

#include <SFML/Graphics/Rect.hpp>

// clang-format off
namespace Game::Cmp::Inventory { struct DowsingTarget; }
// clang-format on

namespace Game::Sys::ProcGen
{

//! @brief Builds the dowsing rod guide: a train of chevrons travelling from the player towards each landmark
//! matching the rod target. All paths share one pulse, which restarts once it has reached the landmark or left the
//! view on every path. Results are stored in Cmp::Player::Doglegs for RenderGameSystem to draw.
class DoglegSystem : public BaseSystem
{
public:
  //! @brief Construct a new DoglegSystem object
  DoglegSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
      : BaseSystem( reg, window, sound_bank )
  {
  }

  //! @brief Rebuild the doglegs and markers for this frame. Leaves both empty if the player holds no dowsing rod.
  //! @param dt Frame delta, advances the pulse
  void update( sf::Time dt );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Add single dogleg to the component
  //! @param source_pos
  //! @param target_pos
  //! @param color
  void add_dogleg( sf::Vector2f source_pos, sf::Vector2f target_pos, sf::Color color );

  //! @brief Add a dogleg from the player to each landmark matching the dowsing rod target
  void update_target( const Cmp::Inventory::DowsingTarget &dowsing_cmp );

  //! @brief Place the chevron markers along each dogleg for the current pulse time
  void update_markers( Cmp::Inventory::DowsingTarget &dowsing_cmp );

  //! @brief Clear doglegs and markers from the component
  void clear_doglegs();

  //! @brief Distance from the player to where the path leaves the view, or the whole path if the target is in view
  static float visible_length( const Cmp::Player::Doglegs::Dogleg &dogleg, const sf::FloatRect &view_bounds );
};

} // namespace Game::Sys::ProcGen

#endif // SRC_SYSTEMS_PROCGEN_DOGLEGSYSTEM_HPP__