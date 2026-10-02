#ifndef SRC_SYSTEMS_STORES_NPCSTORE_HPP__
#define SRC_SYSTEMS_STORES_NPCSTORE_HPP__

#include <Components/Npc/Npc.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Systems/Stores/StoreSingleton.hpp>

namespace Game::Sys
{

//! @brief Singleton store of NPC metadata (Cmp::Npc::NPC, keyed by NPC id) loaded from res/json/npc.json.
class NpcStore : public StoreSingleton<NpcStore, Cmp::Npc::NPC>
{
public:
  //! @brief Construct a new Npc Store object
  //! @param json_file_path Path of the JSON file to load; override to load a fixture (e.g. in tests).
  explicit NpcStore( std::filesystem::path json_file_path = "res/json/npc.json" );

  //! @brief Destroy the Npc Store object
  ~NpcStore() {}

private:
  //! @brief Populates m_store with Cmp::Npc::NPC components
  void init_store();
};

} // namespace Game::Sys

#endif // SRC_SYSTEMS_STORES_NPCSTORE_HPP__
