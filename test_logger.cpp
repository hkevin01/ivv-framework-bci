#include "src/core/logger.h"
#include <iostream>

int main() {
  using namespace IVVFramework::Core;

  Logger logger;
  LogConfig config;
  config.min_level = LogLevel::DEBUG_LEVEL;
  config.destinations.push_back(LogDestination::CONSOLE);

  if (logger.initialize("TestLogger", config)) {
    std::cout << "Logger initialized successfully!" << std::endl;

    logger.log_info("This is a test info message");
    logger.log_debug("This is a test debug message");
    logger.log_warning("This is a test warning message");
    logger.log_error("This is a test error message");
    logger.log_critical("This is a test critical message", "TEST_CONTEXT");

    std::cout << "All log messages sent successfully!" << std::endl;
    return 0;
  } else {
    std::cout << "Failed to initialize logger!" << std::endl;
    return 1;
  }
}
