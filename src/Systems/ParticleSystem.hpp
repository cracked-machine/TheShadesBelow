#ifndef SRC_SYSTEMS_PARTICLESYSTEM_HPP__
#define SRC_SYSTEMS_PARTICLESYSTEM_HPP__

#include <Components/Particle/SpriteBase.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/ParticleEvents.hpp>
#include <Systems/BaseSystem.hpp>

#include <entt/entity/fwd.hpp>
#include <functional>
#include <spdlog/spdlog.h>
#include <utility>

namespace Game::Sys
{

// 1. Emission Stage:   - IParticle::emit()
// 2. Simulation Stage  - IParticleSprite::simulate()
// 3. Rendering Stage   - via RenderGameSystem using Cmp::ZorderValue

// IParticle                            contract: emit(), vertex, velocity, lifetime
// IParticleSprite                      contract: simulate(), set_emitter(), draw()
//   └── SpriteBase<IParticle>  template: owns vector<T>, implements draw()

// Cmp::Particle::SpriteOwner            entt component: tag + unique_ptr<IParticleSprite>

// ParticleSystem                        add(), update(), find() — knows nothing about concrete types
// RenderGameSystem                      iterates SpriteOwner, calls draw() — knows nothing about concrete types

//! @brief Core system for adding and updating ParticleSprite objects
class ParticleSystem : public BaseSystem
{
public:
  using entt_ps_pair = std::pair<entt::entity, std::reference_wrapper<Cmp::Particle::IParticleSprite>>;

  //! @brief Construct a new Particle System object
  //! @param reg
  //! @param window
  //! @param sound_bank
  ParticleSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank );

  //! @brief Add one or more particle sprites, each wrapped and stored under its own entity.
  //! @tparam PARTICLESPRITES
  //! @param sprites Pack of (particle sprite, z-order) pairs.
  template <typename... PARTICLESPRITES>
  void add( const std::pair<PARTICLESPRITES, Cmp::ZOrderValue> &...sprites )
  {
    auto add_one = [this]<typename PS>( const std::pair<PS, Cmp::ZOrderValue> &ps_pair )
    {
      const auto &[ps, zorder] = ps_pair;
      add_to_registry( Cmp::Particle::SpriteOwner( std::make_unique<PS>( ps ) ), zorder );
    };
    ( add_one( sprites ), ... );
  }

  //! @brief Add a particle sprite tagged with an existing Cmp::UUID, so it can be looked up by that UUID.
  //! @tparam PARTICLESPRITE
  //! @param uuid_cmp
  //! @param ps
  //! @param zorder
  template <typename PARTICLESPRITE>
  void add( Cmp::UUID &uuid_cmp, PARTICLESPRITE ps, Cmp::ZOrderValue zorder )
  {
    add_to_registry( uuid_cmp, Cmp::Particle::SpriteOwner( std::make_unique<PARTICLESPRITE>( ps ) ), zorder );
  }

  //! @brief Calls IParticle::update() function within all added SpriteBase<T>
  //! @param dt
  void update( sf::Time dt );

  //! @brief Check if a particle sprite collides with the given AABB target
  //! @param target The AABB target to collision detect with each particle
  //! @param excl_ps_tag_list List of particle sprite tags to exclude from the collision checks
  void check_collsion( const std::vector<std::string> &excl_ps_tag_list = {} );

  //! @brief Find a SpriteOwner by tag and returns a vector of (entity, sprite reference) pairs, empty if there is no match.
  //! @param reg
  //! @param tag
  //! @return std::vector<ParticleSystem::entt_ps_pair>
  [[nodiscard]] static std::vector<entt_ps_pair> find( entt::registry &reg, const std::string &tag );

  //! @brief event handlers for pausing system clocks
  void on_pause() override {};
  //! @brief event handlers for resuming system clocks
  void on_resume() override {};

private:
  //! @brief Add a wrapped particle sprite and zorder component to the registry
  //! @param owner
  //! @param zorder
  //! @return std::vector<entt::entity>
  std::vector<entt::entity> add_to_registry( Cmp::Particle::SpriteOwner owner, Cmp::ZOrderValue zorder );

  //! @brief Add a wrapped particle sprite and zorder component to the registry, tagged with `uuid_cmp`.
  //! @param uuid_cmp
  //! @param owner
  //! @param zorder
  void add_to_registry( Cmp::UUID &uuid_cmp, Cmp::Particle::SpriteOwner owner, Cmp::ZOrderValue zorder );
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_PARTICLESYSTEM_HPP__
