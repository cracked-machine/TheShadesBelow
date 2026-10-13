#ifndef SRC_COMPONENTS_PARTICLE_LAVAFIREBALLPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_LAVAFIREBALLPARTICLESPRITE_HPP__

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
//! @brief Individual fireball spat out of a lava pit cell: launched upwards, pulled back down by gravity.
struct LavaFireballParticle : public Cmp::Particle::ParticleBase
{
  //! @brief Number of vertices around this particle's outline (excluding the centre)
  static constexpr int kVertexCount = 12;

  //! @brief Per-vertex radius jitter in the range [-1, 1], re-rolled every flicker interval by
  //!        LavaFireballParticleSprite::simulate() so the outline is ragged and flickers like a flame.
  std::array<float, kVertexCount> m_vertex_jitter{};

  //! @brief Time left until m_vertex_jitter is next re-rolled.
  sf::Time m_flicker_countdown{ sf::Time::Zero };

private:
  //! @brief Launches the particle on (re)emission.
  void emit() override;
};
} // namespace detail

//! @brief Particle sprite for a fireball spat out of a Cmp::Crypt::RoomLavaPitCell. Each particle travels in an
//!        upwards parabola and is killed when it falls back to the height it was emitted from.
class LavaFireballParticleSprite : public SpriteBase<detail::LavaFireballParticle>
{
public:
  //! @brief Construct a new Lava Fireball Particle Sprite object
  //! @param count Number of particles in this sprite
  LavaFireballParticleSprite( size_t count );

  //! @brief Advances the fireball simulation by one frame (gravity, landing check, outline flicker).
  //! @param dt Time elapsed since the last frame.
  void simulate( sf::Time dt ) override;
  //! @brief Draws each particle as a ragged, flickering fireball whose flames stream out behind its direction of travel.
  //! @param target Render target to draw to.
  //! @param states Render states (transform/blend mode) to draw with.
  void draw( sf::RenderTarget &target, sf::RenderStates states ) const override;

  //! @brief Every fireball sprite's tag must end with this: the player/NPC collision checks match on it,
  //!        so they apply to fireballs from any scene.
  static constexpr std::string_view kTagSuffix = "particle.fireball";

  //! @brief Tag of the fireballs spawned by CryptSystem::do_lava_pit_animation().
  static constexpr std::string_view kLavaPitTag = "crypt.lavapit.particle.fireball";

  //! @brief Downwards acceleration applied to each particle (world px/s^2).
  static const float kGravity;
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_LAVAFIREBALLPARTICLESPRITE_HPP__
