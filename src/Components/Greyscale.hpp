#ifndef SRC_COMPONENTS_GREYSCALE_HPP__
#define SRC_COMPONENTS_GREYSCALE_HPP__

namespace Game::Cmp
{

//! @brief Add this component to a renderable entity to draw its sprite desaturated (res/shaders/Greyscale.frag).
//! Any Cmp::AbsoluteTint is applied to the grey result.
struct Greyscale
{
public:
  //! @brief Always true; presence of the component is what enables the greyscale draw.
  [[maybe_unused]] bool m_active{ true };
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_GREYSCALE_HPP__
