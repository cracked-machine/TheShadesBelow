#ifndef SRC_COMPONENTS_PARTICLE_LAVAFIREBALLPARTICLESPRITE_HPP__
#define SRC_COMPONENTS_PARTICLE_LAVAFIREBALLPARTICLESPRITE_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
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

//! @brief The colours of a LavaFireballParticleSprite's flame layers.
struct FireballPalette
{
  //! @brief The main body of the fireball.
  sf::Color body;
  //! @brief The dark glow behind the body, drawn at half the particle alpha.
  sf::Color glow;
  //! @brief The hot core on top of the body.
  sf::Color core;
};

//! @brief Particle sprite for a fireball spat out of a Cmp::Crypt::RoomLavaPitCell. Each particle travels in an
//!        upwards parabola and is killed when it falls back to the height it was emitted from.
//!        An aimed fireball (see set_land_at_emitter()) instead flies until its lifetime expires or it hits something,
//!        e.g. Azazel's fireballs.
class LavaFireballParticleSprite : public SpriteBase<detail::LavaFireballParticle>
{
public:
  //! @brief Lava pit colours (the default).
  static constexpr FireballPalette kLavaPalette{ .body = sf::Color( 255, 110, 20 ), .glow = sf::Color( 200, 30, 0 ), .core = sf::Color( 255, 230, 120 ) };
  //! @brief Azazel's hellfire colours.
  static constexpr FireballPalette kPurplePalette{ .body = sf::Color( 170, 60, 255 ), .glow = sf::Color( 90, 0, 160 ),
                                                   .core = sf::Color( 230, 190, 255 ) };

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

  //! @brief Every Azazel fireball sprite's tag must end with this. Deliberately doesn't contain kTagSuffix, so Azazel's
  //!        fireballs only hurt the player (see PlayerSystem), not NPCs.
  static constexpr std::string_view kAzazelTagSuffix = "particle.azazelfireball";

  //! @brief Tag of the fireballs shot by Azazel, see NpcSystem::update_azazel().
  static constexpr std::string_view kAzazelTag = "npc.azazel.particle.azazelfireball";

  //! @brief Default downwards acceleration applied to each particle (world px/s^2).
  static const float kGravity;

  //! @brief Set the downwards acceleration. Zero flies straight.
  //! @param gravity World px/s^2.
  void set_gravity( float gravity ) { m_gravity = gravity; }

  //! @brief Whether a falling particle is killed when it gets back down to the height it was emitted from.
  //! @param land true for lava pit fireballs, false for fireballs aimed at a target at a different height.
  void set_land_at_emitter( bool land ) { m_land_at_emitter = land; }

  //! @brief Set the flame colours.
  //! @param palette The colours, e.g. kPurplePalette.
  void set_palette( const FireballPalette &palette ) { m_palette = palette; }

private:
  //! @brief Downwards acceleration applied to each particle (world px/s^2).
  float m_gravity{ kGravity };
  //! @brief See set_land_at_emitter().
  bool m_land_at_emitter{ true };
  //! @brief The flame colours.
  FireballPalette m_palette{ kLavaPalette };
};

} // namespace Game::Cmp::Particle

#endif // SRC_COMPONENTS_PARTICLE_LAVAFIREBALLPARTICLESPRITE_HPP__
