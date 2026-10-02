#ifndef SRC_COMPONENTS_RUIN_OBJECTIVETYPE_HPP__
#define SRC_COMPONENTS_RUIN_OBJECTIVETYPE_HPP__

#include <Systems/Stores/StoreKey.hpp>

namespace Game::Cmp::Ruin
{

//! @brief Tags an entity with the sprite/type identifier of the ruin's quest objective.
//! @note Persisted across scene transfers via SceneControl::RegistryTransfer so the objective's identity
//! survives moving between the ruin's floors and other scenes.
class ObjectiveType
{
public:
  //! @brief Construct a new ObjectiveType.
  //! @param type Sprite/meta type identifying the objective.
  ObjectiveType( Sys::SpriteKey type )
      : m_type( type )
  {
  }
  //! @brief Sprite/meta type identifying the objective.
  Sys::SpriteKey m_type;
};

} // namespace Game::Cmp::Ruin

#endif // SRC_COMPONENTS_RUIN_OBJECTIVETYPE_HPP__
