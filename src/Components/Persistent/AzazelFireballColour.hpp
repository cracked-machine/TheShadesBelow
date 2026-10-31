#ifndef SRC_COMPONENTS_PERSISTENT_AZAZELFIREBALLCOLOUR_HPP__
#define SRC_COMPONENTS_PERSISTENT_AZAZELFIREBALLCOLOUR_HPP__

#include <Components/Persistent/BasePersistentColour.hpp>

namespace Game::Cmp::Persist
{

//! @brief Main (body) colour of Azazel's fireballs. The glow and core keep Cmp::Particle::LavaFireballParticleSprite::kPurplePalette.
class AzazelFireballColour : public BasePersistentColour
{
public:
  //! @brief Construct with the default sf::Color (transparent black), set from res/json/persist.json.
  AzazelFireballColour()
      : BasePersistentColour( "AzazelFireballColour" )
  {
  }
};

} // namespace Game::Cmp::Persist

#endif // SRC_COMPONENTS_PERSISTENT_AZAZELFIREBALLCOLOUR_HPP__
