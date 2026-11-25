#ifndef SRC_COMPONENTS_PARTICLE_PLANTLEAVESPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_PLANTLEAVESPARTICLESPRITE_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>

#include <array>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual leaf-debris particle for a plant destroyed by the shovel, drawn as a jittered irregular polygon.
struct PlantLeavesParticle : public Cmp::Particle::ParticleBase
{
  //! @brief Add some variation to colour of each particle
  int m_color_variation = 0;

  //! @brief This particle's own randomly-assigned lifetime at spawn, captured so speed falloff can
  //!        ease out relative to it rather than the sprite-wide max lifetime
  sf::Time m_initial_lifetime;

  //! @brief Number of vertices in this particle's polygon (excluding the centre)
  static constexpr int kVertexCount = 5;

  //! @brief Per-particle jittered vertex angles, randomized on emit() to give each particle an irregular polygon shape
  std::array<float, kVertexCount> m_vertex_angles{};

  //! @brief Running phase (radians) for the vertical bounce oscillation. Advanced each frame by the
  //!        current bounce frequency rather than derived from elapsed time directly, so the frequency
  //!        can ramp up smoothly over the particle's lifetime without a discontinuity in the wave
  float m_bounce_phase = 0.f;

private:
  //! @brief Rolls new vertex-angle jitter and launches the particle on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for the leaf debris effect played when a plant is destroyed by the shovel.
class PlantLeavesParticleSprite : public SpriteBase<detail::PlantLeavesParticle>
{
public:
  //! @brief Construct a new Plant Leaves Particle Sprite object
  //! @param count Number of particles in this sprite
  PlantLeavesParticleSprite( size_t count );

  //! @brief Advances the leaf debris simulation by one frame (bounce, speed falloff).
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle as a jittered irregular polygon rather than the base class's dot.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_PLANTLEAVESPARTICLESPRITE_HPP__
