#ifndef SRC_COMPONENTS_PARTICLE_EMBERPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_EMBERPARTICLESPRITE_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>
#include <Systems/ParticleSystem.hpp>

#include <string_view>

namespace Game::Cmp::Particle
{

//! @brief Implementation detail — do not use externally
namespace detail
{
//! @brief Individual ember thrown off a burning tree: launched upwards, pulled back down by gravity,
//!        cooling as it flies.
struct EmberParticle : public Cmp::Particle::ParticleBase
{
  //! @brief Expected duration (seconds) of this particle's flight, from launch to landing back at the emitter height.
  float m_flight_time{ 0.f };

  //! @brief Time (seconds) this particle has been in flight, used to cool its colour.
  float m_flight_elapsed{ 0.f };

private:
  //! @brief Launches the particle on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for the embers thrown off a burning tree. Each particle travels in an upwards
//!        parabola, cooling from white-hot to dark red, and is killed when it falls back to the height it
//!        was emitted from.
class EmberParticleSprite : public SpriteBase<detail::EmberParticle>
{
public:
  //! @brief Construct a new Ember Particle Sprite object
  //! @param count Number of particles in this sprite
  EmberParticleSprite( size_t count );

  //! @brief Advances the ember simulation by one frame (gravity, landing check, colour cooling).
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle as a small glowing speck.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;

  //! @brief Every ember sprite's tag must end with this: the player/NPC collision checks match on it,
  //!        so they apply to embers from any scene.
  static constexpr std::string_view kTagSuffix = "particle.ember";

  //! @brief Tag of the embers spawned by ActionSystem::update_burning_worlditems() for burning yew trees.
  static constexpr std::string_view kYewTag = "graveyard.plant.particle.ember";

  //! @brief Downwards acceleration applied to each particle (world px/s^2).
  static const float kGravity;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_EMBERPARTICLESPRITE_HPP__
