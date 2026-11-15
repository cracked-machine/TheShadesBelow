#ifndef SRC_COMPONENTS_PERSISTENT_RETAINANIMABETWEENLEVELS_HPP__
#define SRC_COMPONENTS_PERSISTENT_RETAINANIMABETWEENLEVELS_HPP__

#include <Components/Persistent/BasePersistent.hpp>
namespace Game::Cmp::Persist
{

//! @brief Whether unspent anima carries over to the next level (true) or is sold for wealth on the level complete
//! screen (false).
//! @note Read by Scene::LevelCompleteScene and Scene::GraveyardScene.
class RetainAnimaBetweenLevels : public BasePersistent<bool>
{
public:
  //! @brief Construct the retain anima between levels setting.
  //! @param value true to carry unspent anima over to the next level, false to sell it for wealth.
  RetainAnimaBetweenLevels( bool value = true )
      : BasePersistent<bool>( value )
  {
  }
  //! @brief Returns the unique registry identifier for this persistent setting.
  [[nodiscard]] std::string class_name() const override { return "RetainAnimaBetweenLevels"; }
  //! @brief Returns the display description for this setting (unset; see persist.json).
  [[nodiscard]] const std::string get_detail() const override { return ""; }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_RETAINANIMABETWEENLEVELS_HPP__
