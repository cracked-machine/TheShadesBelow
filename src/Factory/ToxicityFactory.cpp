#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Components/Toxicity/Vertigo.hpp>
#include <Factory/ToxicityFactory.hpp>

#include <spdlog/spdlog.h>

namespace Game::Factory::Toxicity
{

Cmp::Toxicity::Toxidrome create_toxidrome( const std::string &type, int toxicity )
{
  if ( type == "tachycardia" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Tachycardia>( toxicity ).build();
  if ( type == "bradycardia" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Bradycardia>( toxicity ).build();
  if ( type == "halucinogen" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Hallucinogen>( toxicity ).build();
  if ( type == "hypoxia" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Hypoxia>( toxicity ).build();
  if ( type == "phototoxia" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Phototoxia>( toxicity ).build();
  if ( type == "venom" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Venom>( toxicity ).build();
  if ( type == "vertigo" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Vertigo>( toxicity ).build();
  if ( type != "none" ) SPDLOG_WARN( "Unknown toxidrome type: {}", type );
  return ToxidromeBuilder<>{}.build();
}

} // namespace Game::Factory::Toxicity
