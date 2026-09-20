#include <Components/Inventory/WorldItem.hpp>
#include <Components/Random.hpp>
#include <Components/Stats/BaseAction.hpp>
#include <Components/Stats/BurnAction.hpp>
#include <Components/Stats/CarryAction.hpp>
#include <Components/Stats/ConsumeAction.hpp>
#include <Components/Stats/DestroyAction.hpp>
#include <Components/Stats/SacrificeAction.hpp>
#include <Components/Stats/SpawnAction.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Factory/ToxicityFactory.hpp>
#include <Systems/Stores/BaseStore.hpp>

#include <fstream>
#include <nlohmann/json.hpp>

namespace Game::Sys
{

BaseStore::BaseStore( entt::registry &reg, sf::RenderWindow &window, Sprites::SpriteFactory &sprite_factory, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sprite_factory, sound_bank )
{
  SPDLOG_DEBUG( "ItemSystem initialized" );
}

nlohmann::json BaseStore::load_json_file( const std::filesystem::path &json_file )
{
  if ( not std::filesystem::exists( json_file ) )
  {
    SPDLOG_ERROR( "JSON file does not exist: {}", json_file.string() );
    throw std::runtime_error( "JSON file not found: " + json_file.string() );
  }

  std::ifstream fs( json_file );
  if ( not fs.is_open() )
  {
    SPDLOG_ERROR( "Unable to open JSON file: {}", json_file.string() );
    throw std::runtime_error( "Cannot open JSON file: " + json_file.string() );
  }

  nlohmann::json json;
  fs >> json;
  return json;
}

int BaseStore::health( const nlohmann::json &item ) { return item.at( "health" ).get<int>(); }
int BaseStore::fear( const nlohmann::json &item ) { return item.at( "fear" ).get<int>(); }
int BaseStore::despair( const nlohmann::json &item ) { return item.at( "despair" ).get<int>(); }
int BaseStore::infamy( const nlohmann::json &item ) { return item.at( "infamy" ).get<int>(); }
int BaseStore::luck( const nlohmann::json &item ) { return item.at( "luck" ).get<int>(); }
float BaseStore::tick( const nlohmann::json &item ) { return item.at( "tick" ).get<float>(); }

Cmp::Toxicity::Toxidrome BaseStore::toxidrome( const nlohmann::json &item )
{
  const auto &toxidrome = item.contains( "toxidrome" ) ? item.at( "toxidrome" ) : item;

  std::string type = toxidrome.at( "type" ).get<std::string>();
  int toxicity = toxidrome.at( "toxicity" ).get<int>();

  if ( type == "tachycardia" ) return Factory::Toxicity::ToxidromeBuilder<>{}.add<Cmp::Toxicity::Tachycardia>( toxicity ).build();
  if ( type == "bradycardia" ) return Factory::Toxicity::ToxidromeBuilder<>{}.add<Cmp::Toxicity::Bradycardia>( toxicity ).build();
  if ( type == "halucinogen" ) return Factory::Toxicity::ToxidromeBuilder<>{}.add<Cmp::Toxicity::Hallucinogen>( toxicity ).build();
  if ( type == "hypoxia" ) return Factory::Toxicity::ToxidromeBuilder<>{}.add<Cmp::Toxicity::Hypoxia>( toxicity ).build();
  if ( type == "phototoxia" ) return Factory::Toxicity::ToxidromeBuilder<>{}.add<Cmp::Toxicity::Phototoxia>( toxicity ).build();
  if ( type == "venom" ) return Factory::Toxicity::ToxidromeBuilder<>{}.add<Cmp::Toxicity::Venom>( toxicity ).build();
  if ( type != "none" ) SPDLOG_WARN( "Unknown toxidrome type: {}", type );
  return Factory::Toxicity::ToxidromeBuilder<>{}.build();
}

} // namespace Game::Sys