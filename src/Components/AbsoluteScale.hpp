#ifndef SRC_COMPONENTS_ABSOLUTESCALE_HPP__
#define SRC_COMPONENTS_ABSOLUTESCALE_HPP__

namespace Game::Cmp
{

//! @brief Component to store an absolute uniform scale factor
//! Used by RenderGameSystem to scale sprites about their origin (see Cmp::AbsoluteOffset), this should be set by other systems
class AbsoluteScale
{
public:
  //! @brief Construct with an initial scale factor.
  //! @param scale Uniform scale factor. Defaults to 1 (unscaled).
  AbsoluteScale( float scale = 1.f )
      : m_scale( scale )
  {
  }

  //! @brief Get the stored scale factor.
  //! @return float Uniform scale factor.
  float getScale() const { return m_scale; }

  //! @brief Set the stored scale factor.
  //! @param scale Uniform scale factor.
  void setScale( float scale ) { m_scale = scale; }

private:
  //! @brief Uniform scale factor.
  float m_scale;
};

} // namespace Game::Cmp

#endif // SRC_COMPONENTS_ABSOLUTESCALE_HPP__
