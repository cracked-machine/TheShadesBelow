#ifndef SRC_SYS_THREATS_TOXICITYSYSTEM_HPP__
#define SRC_SYS_THREATS_TOXICITYSYSTEM_HPP__

#include <Systems/BaseSystem.hpp>

namespace Game::Sys
{

class ToxicitySystem : public BaseSystem
{
public:
  //! @brief Construct a new ToxicitySystem System object
  //! @param reg
  //! @param window
  //! @param sprite_factory
  //! @param sound_bank
  ToxicitySystem( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank );

  //! @brief Update Priest NPC shockwave
  //! @param dt
  void update( sf::Time dt );
};

} // namespace Game::Sys

#endif // SRC_SYS_THREATS_TOXICITYSYSTEM_HPP__