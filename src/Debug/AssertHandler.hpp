#ifndef SRC_DEBUG_ASSERTHANDLER_HPP__
#define SRC_DEBUG_ASSERTHANDLER_HPP__

#include <csignal>
#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace Debug
{

//! @brief Prints a stack trace of the current call stack to the log (SPDLOG_CRITICAL).
//! @note Uses std::stacktrace; function names and file:line need an unstripped build (-g).
void stack_trace();

//! @brief Custom assertion-failure handler: logs the failed condition, message, and location,
//! optionally breaks into an attached debugger, prints a stack trace, then aborts.
//! @param condition Stringified expression that failed.
//! @param message Human-readable message describing the assertion.
//! @param file Source file where the assertion fired.
//! @param line Source line where the assertion fired.
//! @note Does not return; terminates the process via std::abort().
[[noreturn]] void assert_handler( const char *condition, const char *message, const char *file, int line );

//! @brief Redirects stderr to a file and installs a SIGABRT handler that copies its contents,
//! plus a stack trace, to the log (SPDLOG_CRITICAL).
//! @details Covers failures that bypass assert_handler() and exceptions: libstdc++ assertions
//! (_GLIBCXX_ASSERTIONS), plain assert() and std::terminate().
//! @param stderr_path Filesystem path of the file that receives stderr output.
void install_crash_logging( const char *stderr_path );

} // namespace Debug

//! @def ENTT_ASSERT
//! @brief Replaces the default EnTT assert macro with one that routes failures through
//! Debug::assert_handler() for richer diagnostic output (stack trace, message, location).
#ifdef ENTT_ASSERT
#undef ENTT_ASSERT
#endif
#define ENTT_ASSERT( condition, msg )                                                                                                                \
  do                                                                                                                                                 \
  {                                                                                                                                                  \
    if ( !( condition ) ) { ::Debug::assert_handler( #condition, msg, __FILE__, __LINE__ ); }                                                        \
  } while ( 0 )

#endif // SRC_DEBUG_ASSERTHANDLER_HPP__
