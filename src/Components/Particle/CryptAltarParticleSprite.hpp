#ifndef SRC_COMPONENTS_PARTICLE_CRYPTALTARSPARKLES_HPP__
#define SRC_COMPONENTS_PARTICLE_CRYPTALTARSPARKLES_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual particle for the crypt altar's sparkle effect.
struct CryptAltarParticle : public Cmp::Particle::ParticleBase
{

private:
  //! @brief Resets velocity/angle for the sparkle on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for the visual effect played when a crypt altar is activated.
class CryptAltarParticleSprite : public SpriteBase<detail::CryptAltarParticle>
{
public:
  //! @brief Construct a new Crypt Altar Particle Sprite object
  //! @param count Number of particles in this sprite
  CryptAltarParticleSprite( size_t count );

  //! @brief Advances the sparkle simulation by one frame.
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;

  //! @brief Draws each particle as a fan of triangles sized by its own `m_size` (see add_flame()'s `psize` param),
  //! rather than the base class's fixed dot size.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_CRYPTALTARSPARKLES_HPP__
