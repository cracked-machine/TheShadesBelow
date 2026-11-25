#ifndef SRC_SCENECONTROL_SCENES_LEVELCOMPLETESCENE_HPP__
#define SRC_SCENECONTROL_SCENES_LEVELCOMPLETESCENE_HPP__

#include <Components/Cart/Barrel.hpp>
#include <SceneControl/Events/ProcessLevelCompleteSceneInputEvent.hpp>
#include <SceneControl/Scene.hpp>
#include <Systems/Stores/StoreKey.hpp>

#include <cstddef>
#include <vector>

// clang-format off
namespace Game::Sys { class Store; }
namespace Game::Audio { class SoundBank; }
// clang-format on

namespace Game::Scene
{

//! @brief The victory screen shown when the player leaves the graveyard by the horse. It scores the cart barrels carried
//! over from the graveyard (+1 per cadaver part, +2 per full barrel), then the inventory: preserved cats (+5 each) and spare
//! cadaver parts not put in a barrel (+1 each), both consumed. One step per second, highlighting each part, barrel, cat
//! and spare part on screen as it is counted.
//! The player can only continue once everything has been counted. The barrels end with this scene.
class LevelCompleteScene : public Scene<Events::ProcessLevelCompleteSceneInputEvent>
{
public:
  //! @brief Construct a new LevelCompleteScene object
  //! @param sound_bank Shared sound bank used to play/stop audio for this scene
  //! @param system_store Store of game systems the scene drives each update
  //! @param nav_event_dispatcher Used to notify the SceneInputRouter when new input events are available
  LevelCompleteScene( Audio::SoundBank &sound_bank, Sys::Store &system_store, entt::dispatcher &nav_event_dispatcher )
      : Scene( nav_event_dispatcher ),
        m_sound_bank( sound_bank ),
        m_sys( system_store )
  {
  }

  //! @brief No-op: no world entities need to be created for this scene
  void on_init() override;

  //! @brief Reloads persistent state, stops all audio and builds the tally steps from the barrels and inventory
  void on_enter() override;

  //! @brief Destroys the cart barrels, clears the registry and stops footstep audio
  void on_exit() override;

  //! @brief Get the name of the scene
  //! @return std::string "LevelCompleteScene"
  [[nodiscard]] std::string get_name() const override { return "LevelCompleteScene"; }

  //! @brief Get the registry object owned by the scene
  //! @return entt::registry&
  entt::registry &registry() override;

protected:
  //! @brief Every second, runs the next tally step (adding its wealth and playing a loot sound), then renders the
  //! victory screen
  //! @param dt The time elapsed since the last update
  void do_update( [[maybe_unused]] sf::Time dt ) override;

private:
  //! @brief Shared sound bank used to play/stop audio for this scene
  Audio::SoundBank &m_sound_bank;

  //! @brief Store of game systems the scene drives each update
  Sys::Store &m_sys;

  //! @brief One scored element of the tally.
  struct Step
  {
    enum class Type
    {
      //! @brief One cadaver part in a barrel.
      PART,
      //! @brief The bonus for a full barrel, after its last part.
      FULL_BARREL,
      //! @brief One preserved cat in the inventory.
      CAT,
      //! @brief One spare cadaver part in the inventory.
      SPARE_PART
    };
    Type type;
    //! @brief PART/FULL_BARREL: index into m_barrels.
    std::size_t barrel{ 0 };
    //! @brief PART: index into Cmp::Cart::Barrel::kParts.
    std::size_t part{ 0 };
    //! @brief CAT: index of the cat in the row.
    std::size_t cat{ 0 };
    //! @brief SPARE_PART: index into m_spare_sprites / m_spare_items.
    std::size_t spare{ 0 };
  };

  //! @brief Build m_barrels, m_cat_sprites, m_spare_sprites/m_spare_items and m_steps from the transferred barrels and the
  //! inventory.
  void build_steps();

  //! @brief Run m_steps[m_next_step], adding its wealth to the player.
  void run_next_step();

  //! @brief Render the victory screen text and the tally rows, highlighting the step that has just been run.
  //! @param dt The time elapsed since the last update
  void render( sf::Time dt );

  //! @brief Accumulates elapsed time between tally steps
  sf::Time m_scorecheck_accumulator;

  //! @brief The barrels carried over from the graveyard, left to right
  std::vector<Cmp::Cart::Barrel> m_barrels;

  //! @brief One sprite per preserved cat in the inventory on arrival; they stay drawn after they are consumed
  std::vector<Sys::SpriteKey> m_cat_sprites;

  //! @brief One sprite per spare cadaver part in the inventory on arrival, grouped in Cmp::Cart::Barrel::kParts order;
  //! they stay drawn after they are consumed
  std::vector<Sys::SpriteKey> m_spare_sprites;

  //! @brief The item of each m_spare_sprites entry, consumed from the inventory as it is counted
  std::vector<Sys::ItemKey> m_spare_items;

  //! @brief Every scored element, in counting order
  std::vector<Step> m_steps;

  //! @brief Index of the next step to run; m_steps.size() once everything is counted
  std::size_t m_next_step{ 0 };

  //! @brief Wealth added so far by the tally, shown on the victory screen
  int m_score_wealth{ 0 };

  //! @brief Cadaver parts counted so far
  int m_parts_counted{ 0 };

  //! @brief Full barrels counted so far
  int m_full_barrels_counted{ 0 };

  //! @brief Preserved cats counted so far
  int m_cats_counted{ 0 };

  //! @brief Spare cadaver parts counted so far
  int m_spares_counted{ 0 };
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENES_LEVELCOMPLETESCENE_HPP__
