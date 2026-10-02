#ifndef SRC_SCENECONTROL_SCENESTACK_HPP__
#define SRC_SCENECONTROL_SCENESTACK_HPP__

#include <SceneControl/IScene.hpp>

#include <memory>
#include <stdexcept>
#include <vector>

namespace Game::Scene
{

//! @brief Owns the ordered stack of active IScene instances that SceneManager pushes/pops/replaces.
//!        The back of the stack is the current scene; the entry below it (if any) is the "previous"
//!        scene, used for overlay scenes (e.g. a paused menu drawn over the gameplay scene beneath it).
class SceneStack
{
public:
  //! @brief Construct a new, empty Scene Stack object
  SceneStack() = default;
  //! @brief Destroy the Scene Stack object
  ~SceneStack() = default;

  //! @brief Deleted copy constructor; SceneStack owns its scenes and is not copyable.
  SceneStack( const SceneStack & ) = delete;
  //! @brief Deleted copy assignment; SceneStack owns its scenes and is not copyable.
  SceneStack &operator=( const SceneStack & ) = delete;
  //! @brief Default move constructor.
  SceneStack( SceneStack && ) = default;
  //! @brief Default move assignment operator.
  SceneStack &operator=( SceneStack && ) = default;

  //! @brief Push a new scene onto the top of the stack.
  //! @param scene The scene to push; ownership is transferred to the stack.
  void push( std::unique_ptr<IScene> scene ) { m_stack.push_back( std::move( scene ) ); }

  //! @brief Pop the current (top) scene off the stack.
  //! @note Throws std::runtime_error if the stack is empty.
  void pop()
  {
    if ( m_stack.empty() ) { throw std::runtime_error( "SceneStack::pop: Cannot pop from empty stack" ); }
    m_stack.pop_back();
  }

  //! @brief Safe access to current scene
  //! @note Throws std::runtime_error if the stack is empty.
  //! @return IScene&
  IScene &current()
  {
    if ( m_stack.empty() ) { throw std::runtime_error( "SceneStack::current: No current scene available" ); }
    return *m_stack.back();
  }

  //! @brief Safe access to current scene
  //! @note Throws std::runtime_error if the stack is empty.
  //! @return const IScene&
  [[nodiscard]] const IScene &current() const
  {
    if ( m_stack.empty() ) { throw std::runtime_error( "SceneStack::current: No current scene available" ); }
    return *m_stack.back();
  }

  //! @brief Check whether the stack has no scenes.
  //! @return bool
  [[nodiscard]] bool empty() const noexcept { return m_stack.empty(); }

  //! @brief Get the number of scenes on the stack.
  //! @return std::size_t
  [[nodiscard]] std::size_t size() const noexcept { return m_stack.size(); }

  //! @brief Safe access to previous scene (for overlays)
  //! @return IScene* The scene beneath the current one, or nullptr if there is no such scene.
  IScene *previous() { return ( m_stack.size() >= 2 ) ? m_stack[m_stack.size() - 2].get() : nullptr; }

  //! @brief Safe access to previous scene (for overlays)
  //! @return const IScene* The scene beneath the current one, or nullptr if there is no such scene.
  [[nodiscard]] const IScene *previous() const { return ( m_stack.size() >= 2 ) ? m_stack[m_stack.size() - 2].get() : nullptr; }

  //! @brief Iterator access for debugging/logging (read-only)
  //! @return Iterator to the bottom of the stack.
  [[nodiscard]] auto begin() const { return m_stack.begin(); }
  //! @brief Iterator access for debugging/logging (read-only)
  //! @return Iterator past the top of the stack.
  [[nodiscard]] auto end() const { return m_stack.end(); }

  //! @brief Log the current stack contents (index and scene name), top-most last, via SPDLOG_INFO.
  void print_stack()
  {
    std::stringstream ss;
    for ( std::size_t i = 0; i < m_stack.size(); ++i )
    {
      auto *s = m_stack[i].get();
      ss << "[" << i << "] " << s->get_name() << " ";
    }
    ss << "(top)";
    SPDLOG_INFO( "{}", ss.str() );
  }

private:
  //! @brief Ordered stack of owned scenes; the back is the current scene.
  std::vector<std::unique_ptr<IScene>> m_stack;
};

} // namespace Game::Scene

#endif // SRC_SCENECONTROL_SCENESTACK_HPP__
