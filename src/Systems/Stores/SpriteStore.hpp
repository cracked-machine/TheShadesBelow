#ifndef SRC_SYSTEMS_STORES_SPRITESTORE_HPP__
#define SRC_SYSTEMS_STORES_SPRITESTORE_HPP__

#include <Sprites/SpriteSheet.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Systems/Stores/StoreSingleton.hpp>

namespace Game::Sys
{

//! @brief Singleton store of Sprites::SpriteSheet objects (keyed by Sys::SpriteKey) loaded from res/json/spritesheets.json.
class SpriteStore : public StoreSingleton<SpriteStore, Sprites::SpriteSheet>
{
public:
  //! @brief Construct a new Sprite Store object
  //! @param json_file_path Path of the JSON file to load; override to load a fixture (e.g. in tests).
  explicit SpriteStore( std::filesystem::path json_file_path = "res/json/spritesheets.json" );

  //! @brief Destroy the Sprite Store object
  ~SpriteStore() {}

private:
  //! @brief Populates m_store with Sprites::SpriteSheet objects
  void init_store();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_SPRITESTORE_HPP__
