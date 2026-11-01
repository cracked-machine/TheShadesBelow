#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Components/Toxicity/Vertigo.hpp>
#include <Factory/ToxicityFactory.hpp>

#include <stdexcept>
#include <utility>
#include <vector>

namespace Game::Factory::Toxicity
{

namespace
{

// Stores toxicity verbatim for the named toxidrome, bypassing exclusion checks. Callers must either
// only pass cures (negative deltas), which only reduce already active toxidromes so exclusions don't
// matter, or have already checked the named toxidromes with mutually_exclusive() below.
bool set_by_name( Cmp::Toxicity::Toxidrome &t, const std::string &type, int toxicity )
{
  if ( type == "tachycardia" ) t.set<Cmp::Toxicity::Tachycardia>( toxicity );
  else if ( type == "bradycardia" ) t.set<Cmp::Toxicity::Bradycardia>( toxicity );
  else if ( type == "halucinogen" ) t.set<Cmp::Toxicity::Hallucinogen>( toxicity );
  else if ( type == "hypoxia" ) t.set<Cmp::Toxicity::Hypoxia>( toxicity );
  else if ( type == "phototoxia" ) t.set<Cmp::Toxicity::Phototoxia>( toxicity );
  else if ( type == "venom" ) t.set<Cmp::Toxicity::Venom>( toxicity );
  else if ( type == "vertigo" ) t.set<Cmp::Toxicity::Vertigo>( toxicity );
  else return false;
  return true;
}

// Runtime counterpart of Cmp::Toxicity::are_mutually_exclusive_v over every known toxidrome, for
// type names that aren't known until runtime (e.g. a JSON "type" array).
template <typename... All>
struct ExclusionTable
{
  template <typename A>
  static bool excludes( entt::id_type b )
  {
    return ( ( entt::type_hash<All>::value() == b and Cmp::Toxicity::are_mutually_exclusive_v<A, All> ) or ... );
  }

  static bool mutually_exclusive( entt::id_type a, entt::id_type b )
  {
    return ( ( entt::type_hash<All>::value() == a and excludes<All>( b ) ) or ... );
  }
};

bool mutually_exclusive( entt::id_type a, entt::id_type b )
{
  using namespace Cmp::Toxicity;
  return ExclusionTable<Bradycardia, Tachycardia, Hypoxia, Hallucinogen, Phototoxia, Venom, Vertigo>::mutually_exclusive( a, b );
}

} // namespace

std::optional<entt::id_type> toxidrome_id( const std::string &type )
{
  if ( type == "tachycardia" ) return entt::type_hash<Cmp::Toxicity::Tachycardia>::value();
  if ( type == "bradycardia" ) return entt::type_hash<Cmp::Toxicity::Bradycardia>::value();
  if ( type == "halucinogen" ) return entt::type_hash<Cmp::Toxicity::Hallucinogen>::value();
  if ( type == "hypoxia" ) return entt::type_hash<Cmp::Toxicity::Hypoxia>::value();
  if ( type == "phototoxia" ) return entt::type_hash<Cmp::Toxicity::Phototoxia>::value();
  if ( type == "venom" ) return entt::type_hash<Cmp::Toxicity::Venom>::value();
  if ( type == "vertigo" ) return entt::type_hash<Cmp::Toxicity::Vertigo>::value();
  return std::nullopt;
}

Cmp::Toxicity::Toxidrome create_toxidrome( const std::string &type, int toxicity )
{
  if ( type == "none" )
  {
    if ( toxicity != 0 ) throw std::runtime_error( "Toxidrome type 'none' must have zero toxicity, got " + std::to_string( toxicity ) );
    return ToxidromeBuilder<>{}.build();
  }
  if ( type == "all" )
  {
    // a cure for every toxidrome: negative deltas only reduce already active toxidromes, so
    // exclusions don't matter here and the builder's compile-time check can be bypassed
    if ( toxicity >= 0 )
      throw std::runtime_error( "Toxidrome type 'all' only supports negative toxicity (cures), got " + std::to_string( toxicity ) );
    Cmp::Toxicity::Toxidrome t;
    for ( const auto *name : { "bradycardia", "tachycardia", "hypoxia", "halucinogen", "phototoxia", "venom", "vertigo" } )
      set_by_name( t, name, toxicity );
    return t;
  }
  if ( toxicity == 0 ) throw std::runtime_error( "Toxidrome type '" + type + "' has zero toxicity, use type 'none' instead" );
  if ( type == "tachycardia" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Tachycardia>( toxicity ).build();
  if ( type == "bradycardia" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Bradycardia>( toxicity ).build();
  if ( type == "halucinogen" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Hallucinogen>( toxicity ).build();
  if ( type == "hypoxia" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Hypoxia>( toxicity ).build();
  if ( type == "phototoxia" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Phototoxia>( toxicity ).build();
  if ( type == "venom" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Venom>( toxicity ).build();
  if ( type == "vertigo" ) return ToxidromeBuilder<>{}.add<Cmp::Toxicity::Vertigo>( toxicity ).build();
  throw std::runtime_error( "Unknown toxidrome type: " + type );
}

Cmp::Toxicity::Toxidrome create_toxidrome( const std::vector<std::string> &types, int toxicity )
{
  if ( types.empty() ) throw std::runtime_error( "Toxidrome type list is empty, use type 'none' instead" );
  if ( types.size() == 1 ) return create_toxidrome( types.front(), toxicity );

  // a cure for several toxidromes: same reasoning as "all", the list may name mutually exclusive
  // toxidromes (e.g. tachycardia and bradycardia) since negative deltas only reduce active ones.
  // A positive list must be conflict-free, checked at runtime as the names aren't known until now.
  if ( toxicity == 0 ) throw std::runtime_error( "Multi-type toxidrome has zero toxicity, use type 'none' instead" );
  Cmp::Toxicity::Toxidrome t;
  std::vector<std::pair<std::string, entt::id_type>> seen;
  for ( const auto &type : types )
  {
    if ( type == "none" or type == "all" ) throw std::runtime_error( "Toxidrome type '" + type + "' isn't allowed in a multi-type list" );
    const auto id = toxidrome_id( type );
    if ( not id ) throw std::runtime_error( "Unknown toxidrome type: " + type );
    for ( const auto &[other, other_id] : seen )
    {
      if ( other_id == *id ) throw std::runtime_error( "Toxidrome type '" + type + "' is listed more than once" );
      if ( toxicity > 0 and mutually_exclusive( *id, other_id ) )
        throw std::runtime_error( "Toxidrome types '" + other + "' and '" + type + "' are mutually exclusive, so can't both be added" );
    }
    seen.emplace_back( type, *id );
    set_by_name( t, type, toxicity );
  }
  return t;
}

} // namespace Game::Factory::Toxicity
