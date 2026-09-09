#ifndef SRC_SYSTEMS_ITEMSYSTEM_HPP__
#define SRC_SYSTEMS_ITEMSYSTEM_HPP__

#include <Events/CreateItemEvent.hpp>
#include <PathFinding/SmartPointers.hpp>
#include <Systems/BaseSystem.hpp>

namespace Game::Cmp
{
class Position;
} // namespace Game::Cmp

namespace Game::Sys
{

//! @brief Spawns world item entities (dropped/placed items, seeing stones, explosives) in response to
//! Events::CreateItemEvent.
class ItemSystem : public BaseSystem
{
public:
  //! @brief Construct a new Item System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  ItemSystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief Destroy the Item System object
  ~ItemSystem() {}

  //! @brief init the weak pointer for the reserved-positions grid.
  //! @param reserved_sm
  void init( const PathFinding::SpatialHashGridSharedPtr &reserved_sm ) { m_reserved_sm = reserved_sm; }

  //! @brief Event handler that forwards to create_world_item().
  //! @param ev
  void on_create_item_event( Game::Events::CreateItemEvent ev );

  //! @brief Create a generic world item entity, dispatching to create_seeing_stone()/create_explosive()
  //! for those special item types.
  //! @param pos
  //! @param item Item identifier, e.g. "item.pickaxe". See res/json/sprite_metadata.json.
  //! @param sfx Sound effect to play on placement, if not empty.
  //! @param zorder
  void create_world_item( Cmp::Position pos, const std::string &item, std::string sfx, float zorder = 0.f );

  //! @brief Create a seeing stone (scrying ball) world item, assigning it a unique unused Cmp::SeeingStone::Target.
  //! @param pos
  //! @param item
  //! @param zorder
  void create_seeing_stone( Cmp::Position pos, const std::string &item, float zorder );

  //! @brief Create an explosive (bomb) world item.
  //! @param pos
  //! @param item
  //! @param zorder
  void create_explosive( Cmp::Position pos, const std::string &item, float zorder );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {}
  //! @brief event handlers for resuming system clocks
  void on_resume() override {}

private:
  //! @brief Positions occupied by entities that procgen/algorithmic code must not modify.
  PathFinding::SpatialHashGridWeakPtr m_reserved_sm;
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_ITEMSYSTEM_HPP__
