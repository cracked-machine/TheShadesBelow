#ifndef SRC_LOGGING_BASICLOGCONTROLLER_HPP__
#define SRC_LOGGING_BASICLOGCONTROLLER_HPP__

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <utility>

//! @brief Example usage:
//! @code
//! #include <BasicLogController.hpp>
//! std::unique_ptr<Test::Logging::BasicLogController> logger{
//!      std::make_unique<Test::Logging::BasicLogController>("logger", "log.txt")
//! };
//! @endcode
namespace Game::Logging
{

//! @brief A non-synchronous log controller using SPDLog.
//! @details Follows the Model-View-Controller pattern:
//!          - Internal Model: `spdlog::logger`
//!          - External Views: `spdlog::sinks::stdout_color_sink_mt`, `spdlog::sinks::basic_file_sink_mt`,
//!            `spdlog::sinks::callback_sink_mt`
class BasicLogController
{
public:
  //! @brief Construct a new Basic Log Controller, wiring up console, file, and callback sinks
  //! and installing itself as spdlog's default logger.
  //! @param log_name Name assigned to the underlying spdlog::logger.
  //! @param log_path Filesystem path of the log file sink.
  BasicLogController( std::string log_name, std::string log_path )
      : m_log_name( std::move( log_name ) ),
        m_log_path( std::move( log_path ) )
  {
    m_console_sink->set_level( spdlog::level::trace );
    // m_console_sink->set_pattern("[%c] [%^%l%$] %s:%v");
    m_console_sink->set_pattern( "%s:%# - %v" );

    m_file_sink->set_level( spdlog::level::trace );
    // m_file_sink->set_pattern("[%c] [%^%l%$] %s:%v");
    m_file_sink->set_pattern( "%s:%# - %v" );

    spdlog::set_default_logger( m_logger );
    spdlog::flush_on( spdlog::level::trace );
  }

private:
  //! @brief Name of the underlying spdlog::logger.
  std::string m_log_name;
  //! @brief Filesystem path of the log file sink.
  std::string m_log_path;

  //! @brief Sink for logging to stdout.
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> m_console_sink{ std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };

  //! @brief Sink for logging to file.
  std::shared_ptr<spdlog::sinks::basic_file_sink_mt> m_file_sink{ std::make_shared<spdlog::sinks::basic_file_sink_mt>( m_log_path, true ) };

  //! @brief Sink that forwards log messages to a user-supplied callback (currently a no-op).
  std::shared_ptr<spdlog::sinks::callback_sink_mt> m_callback_sink{
      std::make_shared<spdlog::sinks::callback_sink_mt>( []( [[maybe_unused]] const spdlog::details::log_msg &msg )
  {
    // std::cout << "BasicLogController Callback!!!" << "\n";
  } ) };

  //! @brief The spdlog::logger instance, initialized with the console, file, and callback sinks.
  std::shared_ptr<spdlog::logger> m_logger{ std::make_shared<spdlog::logger>( spdlog::logger( m_log_name, {
                                                                                                              m_file_sink, m_console_sink,
                                                                                                              m_callback_sink // DISABLE CALLBACK HERE
                                                                                                          } ) ) };
};

} // namespace Game::Logging

#endif // SRC_LOGGING_BASICLOGCONTROLLER_HPP__
