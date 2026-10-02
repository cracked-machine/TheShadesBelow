#include <Systems/Stores/StoreSingleton.hpp>

#include <regex>
#include <spdlog/spdlog.h>

namespace Game::Sys::Detail
{

std::function<bool( const std::string & )> make_pattern_matcher( const std::string &pattern )
{
  try
  {
    // Try to use as regex first
    return [pattern_regex = std::regex( pattern )]( const std::string &s ) { return std::regex_search( s, pattern_regex ); };
  } catch ( const std::regex_error &e )
  {
    // If regex fails, fallback to substring matching
    SPDLOG_DEBUG( "Pattern '{}' is not valid regex, using substring matching", pattern );
    return [pattern]( const std::string &s ) { return s.find( pattern ) != std::string::npos; };
  }
}

} // namespace Game::Sys::Detail
