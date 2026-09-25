#ifndef SRC_COMPONENTS_NPC_NPC_HPP__
#define SRC_COMPONENTS_NPC_NPC_HPP__

#include <Components/Stats/BaseAction.hpp>
#include <Sprites/SpriteMetaType.hpp>

#include <typeindex>

namespace Game::Cmp::Npc
{

//! @brief ECS component holding an NPC's sprite animation set, lerp/frame-rate parameters,
//! and the currently active gameplay-stat action modifiers with their elapsed times.
class NPC
{
public:
  //! @brief Default constructor. Leaves sprite_type_list empty and the lerp/frame-rate at 0.
  NPC() = default;

  //! @brief Construct an NPC with a single sprite type.
  //! @param sprite_type The sprite to use, wrapped into a single-element sprite_type_list.
  //! @param lerp_speed Interpolation speed used when smoothing this NPC's movement.
  //! @param frame_rate Animation frame rate for the sprite.
  NPC( Sprites::SpriteMetaType sprite_type, float lerp_speed, float frame_rate )
      : sprite_type_list( std::vector<Sprites::SpriteMetaType>{ std::move( sprite_type ) } ),
        m_lerp_speed( lerp_speed ),
        m_frame_rate( frame_rate )
  {
  }

  //! @brief Construct an NPC with multiple sprite types (e.g. for animation).
  //! @param sprite_type_list The list of sprites, see res/json/npc.json.
  //! @param lerp_speed Interpolation speed used when smoothing this NPC's movement.
  //! @param frame_rate Animation frame rate for the sprites.
  NPC( std::vector<Sprites::SpriteMetaType> sprite_type_list, float lerp_speed, float frame_rate )
      : sprite_type_list( std::move( sprite_type_list ) ),
        m_lerp_speed( lerp_speed ),
        m_frame_rate( frame_rate )
  {
  }
  // bool active = true;

  //! @brief The associated sprite. Supports multiple sprites for animation. See res/json/npc.json.
  std::vector<Sprites::SpriteMetaType> sprite_type_list;

  //! @brief Interpolation speed used when smoothing this NPC's movement.
  float m_lerp_speed{ 0 };
  //! @brief Animation frame rate for the sprite(s) in sprite_type_list.
  float m_frame_rate{ 0 };

  //! @brief Object holding a list of action modifiers and time object
  struct ActionTimePair
  {
    //! @brief Record of player stat modifiers for a specific action.
    BaseAction action;
    //! @brief Time elapsed for a specific action. This can be used for applying modifiers from a specific NPC instance.
    sf::Time time;
  };

  template <typename ActionT>
  auto &at()
  {
    return actions.at( std::type_index( typeid( ActionT ) ) );
  }
  template <typename ActionT>
  void emplace( ActionT action )
  {
    actions.emplace( std::type_index( typeid( ActionT ) ), ActionTimePair{ action, sf::Time::Zero } );
  }
  auto begin() { return actions.begin(); }
  auto end() { return actions.end(); }

private:
  //! @brief The list of action modifiers and their current elapsed time
  std::unordered_map<std::type_index, ActionTimePair> actions;
};
} // namespace Game::Cmp::Npc

#endif // SRC_COMPONENTS_NPC_NPC_HPP__
