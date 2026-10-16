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
  if ( type == "all" )
  {
    // a cure for every toxidrome: negative deltas only reduce already active toxidromes, so
    // exclusions don't matter here and the builder's compile-time check can be bypassed
    Cmp::Toxicity::Toxidrome t;
    if ( toxicity > 0 )
    {
      SPDLOG_WARN( "Toxidrome type 'all' only supports negative toxicity (cures), got {}", toxicity );
      return t;
    }
    t.set<Cmp::Toxicity::Bradycardia>( toxicity );
    t.set<Cmp::Toxicity::Tachycardia>( toxicity );
    t.set<Cmp::Toxicity::Hypoxia>( toxicity );
    t.set<Cmp::Toxicity::Hallucinogen>( toxicity );
    t.set<Cmp::Toxicity::Phototoxia>( toxicity );
    t.set<Cmp::Toxicity::Venom>( toxicity );
    t.set<Cmp::Toxicity::Vertigo>( toxicity );
    return t;
  }
  if ( type != "none" ) SPDLOG_WARN( "Unknown toxidrome type: {}", type );
  return ToxidromeBuilder<>{}.build();
}

} // namespace Game::Factory::Toxicity
