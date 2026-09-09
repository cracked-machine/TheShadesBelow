#ifndef SRC_SCENECONTROL_ISCENE_HPP__
#define SRC_SCENECONTROL_ISCENE_HPP__

#include <PathFinding/SmartPointers.hpp>

#include <SFML/System/Time.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/signal/dispatcher.hpp>
#include <string>

namespace Game::Scene
{

//! @brief Abstract interface for a scene
class IScene
{
public:
  //! @brief Destroy the IScene object
  virtual ~IScene() = default;

  //! @brief Initialize the scene
  virtual void on_init() = 0;
  //! @brief Called when the scene is entered
  virtual void on_enter() = 0;
  //! @brief Called when the scene is exited
  virtual void on_exit() = 0;
  //! @brief Update the scene
  virtual void update( sf::Time dt ) = 0;
  //! @brief Get the name of the scene
  virtual std::string get_name() const = 0;

  //! @brief Get the registry object owned by the scene
  //! @return entt::registry&
  virtual entt::registry &registry() = 0;

  //! @brief Get the scene's grid of positions reserved from procgen/algorithmic changes.
  //! @return PathFinding::SpatialHashGridSharedPtr
  virtual PathFinding::SpatialHashGridSharedPtr get_reserved_sm() = 0;

protected:
  //! @brief User-overridable update function
  virtual void do_update( sf::Time dt ) = 0;
  //! @brief The entity-component registry for the scene
  entt::registry m_reg;
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_ISCENE_HPP__
