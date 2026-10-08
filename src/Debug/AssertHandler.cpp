#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#include <spdlog/spdlog.h>

#include <Debug/AssertHandler.hpp>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stacktrace>
#include <string>

namespace Debug
{

void stack_trace()
{
  SPDLOG_CRITICAL( "=== Stack Trace ===" );

  // skip this function's own frame
  auto trace = std::stacktrace::current( 1 );
  if ( trace.empty() )
  {
    SPDLOG_CRITICAL( "Stack trace unavailable" );
    return;
  }

  size_t i = 0;
  for ( const auto &frame : trace )
  {
    auto name = frame.description();
    if ( name.empty() ) { name = "Unknown function"; }

    // file/line come from the DWARF debug info in the executable, so are only available for unstripped builds
    if ( not frame.source_file().empty() ) { SPDLOG_CRITICAL( "[{:2}] {} in {}:{}", i, name, frame.source_file(), frame.source_line() ); }
    else { SPDLOG_CRITICAL( "[{:2}] {} at {:016X} (no line info)", i, name, static_cast<uintptr_t>( frame.native_handle() ) ); }
    i++;
  }
}

[[noreturn]] void assert_handler( const char *condition, const char *message, const char *file, const int line )
{
  SPDLOG_CRITICAL( "\n" );
  SPDLOG_CRITICAL( "=== Assertion Failed ===" );
  SPDLOG_CRITICAL( "Condition: {}", condition );
  SPDLOG_CRITICAL( "Message: {}", message );
  SPDLOG_CRITICAL( "Location: {}:{}", file, line );
  SPDLOG_CRITICAL( "========================" );

#ifdef _WIN32
  // Break into debugger if attached (Windows)
  if ( IsDebuggerPresent() != 0 ) { DebugBreak(); }
#elif defined( __unix__ )
  // Break into debugger if attached (Unix)
  if ( std::getenv( "UNDER_GDB" ) || std::getenv( "UNDER_LLDB" ) ) { raise( SIGTRAP ); }
#endif

  Debug::stack_trace();

  // already reported above, so don't let the SIGABRT handler report it again
  std::signal( SIGABRT, SIG_DFL );
  std::abort();
}

namespace
{

std::string s_stderr_path;

void abort_handler( int )
{
  SPDLOG_CRITICAL( "\n" );
  SPDLOG_CRITICAL( "=== Abnormal Termination (SIGABRT) ===" );

  // copy whatever was written to stderr (e.g. the libstdc++ assertion text) into the log
  std::fflush( stderr );
  std::ifstream stderr_file( s_stderr_path );
  for ( std::string line; std::getline( stderr_file, line ); )
  {
    if ( not line.empty() ) { SPDLOG_CRITICAL( "stderr: {}", line ); }
  }
  SPDLOG_CRITICAL( "======================================" );

  Debug::stack_trace();
}

} // namespace

void install_crash_logging( const char *stderr_path )
{
  s_stderr_path = stderr_path;

  // unbuffered so the text is on disk before abort() kills the process
  if ( std::freopen( stderr_path, "w", stderr ) ) { std::setvbuf( stderr, nullptr, _IONBF, 0 ); }

  std::signal( SIGABRT, abort_handler );
}

} // namespace Debug
