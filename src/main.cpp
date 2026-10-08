#include "Debug/AssertHandler.hpp" // Include this first for custom assertions

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#include <Engine.hpp>
#include <Logging/BasicLogController.hpp>

int main()
{
  // Logging: make sure errors - exceptions and failed asserts - go to log file
  Debug::install_crash_logging( "stderr.txt" );

  using Logger = Game::Logging::BasicLogController;
  std::unique_ptr<Logger> logger{ std::make_unique<Logger>( "logger", "log.txt" ) };
  spdlog::set_level( spdlog::level::trace );

  SPDLOG_DEBUG( "Entering Engine" );

  Game::Engine engine;
  engine.run();
}