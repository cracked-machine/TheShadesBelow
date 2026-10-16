#ifndef SRC_COMPONENTS_PARTICLE_LightningRodParticleSprite_HPP__
#define SRC_COMPONENTS_PARTICLE_LightningRodParticleSprite_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>
#include <Systems/ParticleSystem.hpp>

#include <array>
#include <string_view>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual copper debris particle from a lightning rod worn out by lightning, drawn as a jittered irregular polygon.
struct LightningRodParticle : public Cmp::Particle::ParticleBase
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

//! @brief Particle sprite for the copper debris effect played when a lightning rod's wear level reaches zero.
//!        The debris damages the player on contact, see Sys::PlayerSystem::check_burning_particle_collision().
class LightningRodParticleSprite : public SpriteBase<detail::LightningRodParticle>
{
public:
  //! @brief Construct a new Lightning Rod Particle Sprite object
  //! @param count Number of particles in this sprite
  LightningRodParticleSprite( size_t count );

  //! @brief Advances the debris simulation by one frame (bounce, speed falloff).
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle as a jittered irregular polygon rather than the base class's dot.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;

  //! @brief Every lightning rod debris sprite's tag must end with this: the player collision check matches on it.
  static constexpr std::string_view kTagSuffix = "particle.lightningrod";

  //! @brief Tag of the debris spawned by LightningSystem when a lightning rod wears out.
  static constexpr std::string_view kTag = "graveyard.lightningrod.particle.lightningrod";
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_LightningRodParticleSprite_HPP__
