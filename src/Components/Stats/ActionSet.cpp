#include <Components/Stats/ActionSet.hpp>
#include <Components/Stats/BaseAction.hpp>

namespace Game::Cmp
{

namespace
{

const ActionMap &empty_map()
{
  static const ActionMap kEmpty;
  return kEmpty;
}

const ActionTimePair &no_action_pair()
{
  static const ActionTimePair kNone{ .action = no_action(), .time = sf::Time::Zero };
  return kNone;
}

} // namespace

ActionSet::ActionSet()
    : m_map( std::make_unique<ActionMap>() )
{
}

ActionSet::~ActionSet() = default;

ActionSet::ActionSet( const ActionSet &other )
    : m_map( std::make_unique<ActionMap>( other.map() ) )
{
}

ActionSet &ActionSet::operator=( const ActionSet &other )
{
  if ( this != &other ) { m_map = std::make_unique<ActionMap>( other.map() ); }
  return *this;
}

ActionSet::ActionSet( ActionSet &&other ) noexcept = default;

ActionSet &ActionSet::operator=( ActionSet &&other ) noexcept = default;

ActionTimePair &ActionSet::at( std::type_index key )
{
  if ( not m_map ) { m_map = std::make_unique<ActionMap>(); }
  // find first: building a zeroed action for try_emplace on every call would copy it even when the key exists
  if ( const auto it = m_map->find( key ); it != m_map->end() ) { return it->second; }
  return m_map->emplace( key, no_action_pair() ).first->second;
}

const ActionTimePair &ActionSet::at( std::type_index key ) const
{
  if ( not m_map ) { return no_action_pair(); }
  const auto it = m_map->find( key );
  return it != m_map->end() ? it->second : no_action_pair();
}

bool ActionSet::contains( std::type_index key ) const { return m_map and m_map->contains( key ); }

void ActionSet::emplace( std::type_index key, const BaseAction &action )
{
  if ( not m_map ) { m_map = std::make_unique<ActionMap>(); }
  m_map->emplace( key, ActionTimePair{ .action = action, .time = sf::Time::Zero } );
}

const ActionMap &ActionSet::map() const { return m_map ? *m_map : empty_map(); }

} // namespace Game::Cmp
