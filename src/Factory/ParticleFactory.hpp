#ifndef SRC_FACTORY_PARTICLEFACTORY_HPP__
#define SRC_FACTORY_PARTICLEFACTORY_HPP__

#include <SFML/System/Vector2.hpp>
#include <entt/entity/fwd.hpp>

// clang-format off
namespace Game::Cmp { class UUID; class ZOrderValue; }
// clang-format on

namespace Game::Factory::Particle
{

//! @brief Spawn the particle effect for an activated crypt altar.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param lifetime_seconds Maximum particle lifetime, in seconds.
//! @param speed Maximum particle speed.
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter.
//! @param zorder Draw order relative to other entities.
void add_crypt_altar_ps( entt::registry &reg, const std::string &tag, float lifetime_seconds, float speed, Cmp::UUID &uuid_cmp, sf::Vector2f pos,
                         float zorder );

//! @brief Spawn the particle effect that plays when the player is healed.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param lifetime_seconds Maximum particle lifetime, in seconds.
//! @param speed Maximum particle speed.
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter.
//! @param zorder Draw order relative to other entities.
void add_player_healing_ps( entt::registry &reg, const std::string &tag, float lifetime_seconds, float speed, Cmp::UUID &uuid_cmp, sf::Vector2f pos,
                            float zorder );

//! @brief Spawn a gunfire particle burst from a watchman, aimed at the player's current position with a small random spread.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param lifetime_seconds Particle lifetime, in seconds (fixed, not randomised).
//! @param speed Particle speed (fixed, not randomised).
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter (the firing watchman).
//! @param zorder Draw order relative to other entities.
void add_watchman_gunfire_ps( entt::registry &reg, const std::string &tag, float lifetime_seconds, float speed, Cmp::UUID &uuid_cmp, sf::Vector2f pos,
                              float zorder );

//! @brief Spawn a rune particle effect.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param lifetime_seconds Maximum particle lifetime, in seconds.
//! @param speed Maximum particle speed.
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter.
//! @param zorder Draw order relative to other entities.
void add_rune_ps( entt::registry &reg, const std::string &tag, float lifetime_seconds, float speed, Cmp::UUID &uuid_cmp, sf::Vector2f pos,
                  float zorder );

//! @brief Spawn a wormhole particle effect.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param lifetime_seconds Maximum particle lifetime, in seconds.
//! @param speed Maximum particle speed.
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter.
//! @param zorder Draw order relative to other entities.
void add_wormhole_ps( entt::registry &reg, const std::string &tag, float lifetime_seconds, float speed, Cmp::UUID &uuid_cmp, sf::Vector2f pos,
                      float zorder );

//! @brief Spawn the debris particle effect for an obstacle being dug up.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param particle_count Number of particles in the sprite.
//! @param lifetime_seconds Maximum particle lifetime, in seconds.
//! @param speed Particle speed (fixed, not randomised).
//! @param size Per-particle dot size.
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter (jittered slightly per call).
//! @param zorder Draw order relative to other entities.
void add_obstacledig_ps( entt::registry &reg, const std::string &tag, int particle_count, float lifetime_seconds, float speed, float size,
                         Cmp::UUID &uuid_cmp, sf::Vector2f pos, float zorder );

//! @brief Spawn the debris particle effect for an player eating.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param particle_count Number of particles in the sprite.
//! @param lifetime_seconds Maximum particle lifetime, in seconds.
//! @param speed Particle speed (fixed, not randomised).
//! @param size Per-particle dot size.
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter (jittered slightly per call).
//! @param facing_direction Direction the player is currently facing; the crumbs are launched around this angle with a small spread.
//! @param zorder Draw order relative to other entities.
void add_eatingcrumbs_ps( entt::registry &reg, const std::string &tag, int particle_count, float lifetime_seconds, float speed, float size,
                          Cmp::UUID &uuid_cmp, sf::Vector2f pos, sf::Vector2f facing_direction, float zorder );

//! @brief Spawn the leaf debris particle effect for a plant destroyed by the axe.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param particle_count Number of particles in the sprite.
//! @param lifetime_seconds Maximum particle lifetime, in seconds.
//! @param speed Particle speed (fixed, not randomised).
//! @param size Per-particle dot size.
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter.
//! @param zorder Draw order relative to other entities.
void add_plantleaves_ps( entt::registry &reg, const std::string &tag, int particle_count, float lifetime_seconds, float speed, float size,
                         Cmp::UUID &uuid_cmp, sf::Vector2f pos, float zorder );

//! @brief Spawn the twig debris particle effect for a plant destroyed by the axe.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param particle_count Number of particles in the sprite.
//! @param lifetime_seconds Maximum particle lifetime, in seconds.
//! @param speed Particle speed (fixed, not randomised).
//! @param size Per-particle dot size.
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter.
//! @param zorder Draw order relative to other entities.
void add_planttwigs_ps( entt::registry &reg, const std::string &tag, int particle_count, float lifetime_seconds, float speed, float size,
                        Cmp::UUID &uuid_cmp, sf::Vector2f pos, float zorder );

//! @brief Spawn the bone debris particle effect for a skeleton killed by the axe.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param particle_count Number of particles in the sprite.
//! @param lifetime_seconds Maximum particle lifetime, in seconds.
//! @param speed Particle speed (fixed, not randomised).
//! @param size Per-particle dot size.
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter.
//! @param zorder Draw order relative to other entities.
void add_skelebones_ps( entt::registry &reg, const std::string &tag, int particle_count, float lifetime_seconds, float speed, float size,
                        Cmp::UUID &uuid_cmp, sf::Vector2f pos, float zorder );

//! @brief Spawn a flame particle sprite (candle/altar fire).
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param initial_pos Initial position of the emitter. Note this is changed per-frame by ParticleSystem::update()
//! @param zorder Draw order relative to other entities.
//! @param scale Multiplies the flame's sway/rise trajectory (not per-particle dot size). The world view's camera
//! transform magnifies that trajectory relative to the untransformed inventory UI view, so world-space
//! flames currently need a smaller scale than the inventory UI flame to read as the same visual size.
void add_flame( entt::registry &reg, const std::string &tag, Cmp::UUID &uuid_cmp, sf::Vector2f initial_pos, float zorder, float scale,
                float psize = 3.f, float speed = 40.f, float plifetime = 0.5, size_t pcount = 220 );

//! @brief Create (or move) a flame particle sprite for a candle currently equipped in the player's inventory,
//! switching it to render in the UI/inventory view instead of the game world.
//! @param reg
void add_flame_for_player_inventory_slot( entt::registry &reg );

//! @brief Spawn a smoke particle effect.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter.
//! @param zorder Draw order relative to other entities.
void add_smoke( entt::registry &reg, const std::string &tag, Cmp::UUID &uuid_cmp, sf::Vector2f pos, float zorder );

//! @brief Spawn a small pyramid-shaped pile of ash. Particles fall from the apex, staggered over
//! `buildup_seconds` so the pile fills in gradually rather than dropping all at once, and settle
//! permanently inside a triangular cross-section (see AshPileParticleSprite::kFallDistance/kBaseWidth),
//! centred on `emitter_pos`'s x position.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via
//! update_position()). Give this its own UUID rather than reusing another effect's (e.g. a burning
//! plant's), otherwise cleanup code that destroys particle sprites by UUID will delete this pile too.
//! @param emitter_pos World position the ash pile settles around (the pyramid's base); the apex/emitter
//! is placed AshPileParticleSprite::kFallDistance (8px) above this.
//! @param zorder Draw order relative to other entities.
//! @param scale Multiplies the fall/settle trajectory, scaling the overall size of the pyramid.
//! @param psize Per-particle dot size.
//! @param speed Particle fall speed (px/s).
//! @param pcount Number of particles in the sprite.
//! @param buildup_seconds Time window over which particles' fall start times are randomly spread, so
//! the pile builds up over time instead of every particle falling in the same frame.
void add_ashpile( entt::registry &reg, const std::string &tag, Cmp::UUID &uuid_cmp, sf::Vector2f emitter_pos, float zorder, float scale,
                  float psize = 3.f, float speed = 20.f, float buildup_seconds = 4.f );

//! @brief Spawn a shockwave particle effect radiating outward from `pos`.
//! @param reg
//! @param tag <location>.<item>.particle.<sprite>
//! @param uuid_cmp UUID assigned to the created entity, so it can be tracked/updated later (e.g. via update_position()).
//! @param pos World position of the emitter.
//! @param zorder Draw order relative to other entities.
void add_shockwave( entt::registry &reg, const std::string &tag, Cmp::UUID &uuid_cmp, sf::Vector2f pos, float zorder );

//! @brief Destroy the entities of any particle sprites whose tag contains `search_pattern` and which are no longer active.
//! @param reg
//! @param search_pattern Substring matched against each particle sprite's tag.
void delete_expired_particle_sprites( entt::registry &reg, const std::string &search_pattern );

//! @brief Update the emitter position of every particle sprite whose tag contains `search_pattern`.
//! @param reg
//! @param search_pattern Substring matched against each particle sprite's tag.
//! @param pos
void update_position( entt::registry &reg, const std::string &search_pattern, sf::Vector2f pos );

//! @brief Update the particle sprite position so that it follows `pos`
//! @param reg
//! @param uuid_cmp
//! @param pos
void update_position( entt::registry &reg, Cmp::UUID uuid_cmp, sf::Vector2f pos );

} // namespace Game::Factory::Particle

#endif // SRC_FACTORY_PARTICLEFACTORY_HPP__
