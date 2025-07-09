/**
 * @file logger.h
 * @brief Safety-Critical Logging System for IV&V Framework
 *
 * Provides comprehensive logging capabilities with safety-critical
 * features including audit trails, real-time logging, and compliance
 * with medical device logging standards.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 *
 * @copyright Copyright (c) 2025 IV&V Framework for BCI Systems
 *
 * Safety-Critical Notice:
 * All safety-critical events must be logged with appropriate
 * severity levels and tamper-evident audit trails.
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace IVVFramework {
namespace Core {

/**
 * @brief Log severity levels for safety-critical systems
 */
enum class LogLevel {
  TRACE = 0,       ///< Detailed tracing information
  DEBUG_LEVEL = 1, ///< Debug information
  INFO = 2,        ///< General information
  WARNING = 3,     ///< Warning conditions
  ERROR_LEVEL = 4, ///< Error conditions
  CRITICAL = 5,    ///< Critical safety violations
  FATAL = 6        ///< Fatal system errors
};

/**
 * @brief Log entry structure
 */
struct LogEntry {
  std::chrono::system_clock::time_point timestamp;
  LogLevel level;
  std::string category;
  std::string message;
  std::string thread_id;
  std::string file_name;
  int line_number;
  std::string function_name;

  // Safety-critical fields
  bool is_safety_critical = false;
  std::string safety_context;
  uint64_t sequence_number = 0;
  std::string checksum;
};

/**
 * @brief Log output destination
 */
enum class LogDestination {
  FILE = 0,        ///< Log to file
  CONSOLE = 1,     ///< Log to console
  SYSLOG = 2,      ///< Log to system log
  AUDIT_TRAIL = 3, ///< Log to tamper-evident audit trail
  REMOTE = 4       ///< Log to remote server
};

/**
 * @brief Log configuration
 */
struct LogConfig {
  LogLevel min_level = LogLevel::INFO;
  std::vector<LogDestination> destinations;
  std::string log_file_path;
  std::string audit_trail_path;
  size_t max_file_size_mb = 100;
  size_t max_file_count = 10;
  bool enable_compression = true;
  bool enable_encryption = false;
  bool real_time_flush = true;

  // Safety-critical settings
  bool enable_audit_trail = true;
  bool enable_integrity_checking = true;
  std::chrono::milliseconds flush_interval{100};
};

/**
 * @brief Log filter callback
 */
using LogFilterCallback = std::function<bool(const LogEntry &)>;

/**
 * @brief Safety event callback
 */
using SafetyEventCallback = std::function<void(const LogEntry &)>;

/**
 * @class Logger
 * @brief Safety-critical logging system
 *
 * The Logger provides comprehensive logging capabilities designed
 * for safety-critical BCI systems with audit trails, integrity
 * checking, and real-time performance.
 *
 * Thread Safety: This class is thread-safe for concurrent logging.
 * Real-time Safety: Critical log methods have bounded execution time.
 */
class Logger {
public:
  /**
   * @brief Constructor
   */
  Logger();

  /**
   * @brief Destructor
   */
  ~Logger();

  /**
   * @brief Initialize logger
   * @param component_name Name of the component using the logger
   * @param config Logger configuration
   * @return true if initialization successful, false otherwise
   * @pre component_name must not be empty
   * @post Logger is ready for use
   */
  bool initialize(const std::string &component_name,
                  const LogConfig &config = LogConfig{});

  /**
   * @brief Log trace message
   * @param message Message to log
   * @param file_name Source file name (optional)
   * @param line_number Source line number (optional)
   * @param function_name Source function name (optional)
   * @note This method is real-time safe
   */
  void log_trace(const std::string &message, const char *file_name = nullptr,
                 int line_number = 0, const char *function_name = nullptr);

  /**
   * @brief Log debug message
   * @param message Message to log
   * @param file_name Source file name (optional)
   * @param line_number Source line number (optional)
   * @param function_name Source function name (optional)
   * @note This method is real-time safe
   */
  void log_debug(const std::string &message, const char *file_name = nullptr,
                 int line_number = 0, const char *function_name = nullptr);

  /**
   * @brief Log info message
   * @param message Message to log
   * @param file_name Source file name (optional)
   * @param line_number Source line number (optional)
   * @param function_name Source function name (optional)
   * @note This method is real-time safe
   */
  void log_info(const std::string &message, const char *file_name = nullptr,
                int line_number = 0, const char *function_name = nullptr);

  /**
   * @brief Log warning message
   * @param message Message to log
   * @param file_name Source file name (optional)
   * @param line_number Source line number (optional)
   * @param function_name Source function name (optional)
   * @note This method is real-time safe
   */
  void log_warning(const std::string &message, const char *file_name = nullptr,
                   int line_number = 0, const char *function_name = nullptr);

  /**
   * @brief Log error message
   * @param message Message to log
   * @param file_name Source file name (optional)
   * @param line_number Source line number (optional)
   * @param function_name Source function name (optional)
   * @note This method is real-time safe
   */
  void log_error(const std::string &message, const char *file_name = nullptr,
                 int line_number = 0, const char *function_name = nullptr);

  /**
   * @brief Log critical safety violation
   * @param message Message to log
   * @param safety_context Safety context information
   * @param file_name Source file name (optional)
   * @param line_number Source line number (optional)
   * @param function_name Source function name (optional)
   * @note This method is real-time safe and triggers safety callbacks
   */
  void log_critical(const std::string &message,
                    const std::string &safety_context = "",
                    const char *file_name = nullptr, int line_number = 0,
                    const char *function_name = nullptr);

  /**
   * @brief Log fatal system error
   * @param message Message to log
   * @param safety_context Safety context information
   * @param file_name Source file name (optional)
   * @param line_number Source line number (optional)
   * @param function_name Source function name (optional)
   * @note This method is real-time safe and may trigger emergency procedures
   */
  void log_fatal(const std::string &message,
                 const std::string &safety_context = "",
                 const char *file_name = nullptr, int line_number = 0,
                 const char *function_name = nullptr);

  /**
   * @brief Set minimum log level
   * @param level Minimum level to log
   * @note This method is real-time safe
   */
  void set_log_level(LogLevel level);

  /**
   * @brief Get current minimum log level
   * @return Current minimum log level
   * @note This method is real-time safe
   */
  LogLevel get_log_level() const;

  /**
   * @brief Register log filter callback
   * @param callback Filter callback function
   * @pre callback must be valid
   */
  void register_filter_callback(LogFilterCallback callback);

  /**
   * @brief Register safety event callback
   * @param callback Safety event callback function
   * @pre callback must be valid
   */
  void register_safety_callback(SafetyEventCallback callback);

  /**
   * @brief Flush all pending log entries
   * @return true if flushed successfully, false otherwise
   */
  bool flush();

  /**
   * @brief Get log statistics
   * @return Log statistics structure
   */
  struct LogStatistics {
    uint64_t total_entries = 0;
    uint64_t safety_critical_entries = 0;
    uint64_t error_entries = 0;
    uint64_t dropped_entries = 0;
    std::chrono::system_clock::time_point first_entry_time;
    std::chrono::system_clock::time_point last_entry_time;
  };

  LogStatistics get_statistics() const;

  /**
   * @brief Verify audit trail integrity
   * @return true if integrity is intact, false otherwise
   */
  bool verify_audit_trail_integrity() const;

  /**
   * @brief Archive old log files
   * @return true if archived successfully, false otherwise
   */
  bool archive_old_logs();

  /**
   * @brief Emergency log function for critical situations
   * @param message Emergency message
   * @return true if logged successfully, false otherwise
   * @note This method must complete within 10ms
   */
  bool emergency_log(const std::string &message) noexcept;

private:
  /**
   * @brief Internal log message function
   */
  void log_message(LogLevel level, const std::string &message,
                   const std::string &safety_context = "",
                   const char *file_name = nullptr, int line_number = 0,
                   const char *function_name = nullptr,
                   bool is_safety_critical = false);

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

/**
 * @brief Logging utility functions
 */
namespace LogUtils {
/**
 * @brief Convert log level to string
 * @param level Log level to convert
 * @return String representation
 */
std::string log_level_to_string(LogLevel level);

/**
 * @brief Parse log level from string
 * @param level_str String to parse
 * @return Parsed log level
 * @throws std::invalid_argument if string is invalid
 */
LogLevel string_to_log_level(const std::string &level_str);

/**
 * @brief Format timestamp for logging
 * @param timestamp Timestamp to format
 * @return Formatted timestamp string
 */
std::string format_timestamp(std::chrono::system_clock::time_point timestamp);

/**
 * @brief Calculate log entry checksum
 * @param entry Log entry to checksum
 * @return Checksum string
 */
std::string calculate_checksum(const LogEntry &entry);
} // namespace LogUtils

} // namespace Core
} // namespace IVVFramework

// Convenience macros for logging with file/line information
#define IVV_LOG_TRACE(logger, message)                                         \
  (logger)->log_trace((message), __FILE__, __LINE__, __FUNCTION__)

#define IVV_LOG_DEBUG(logger, message)                                         \
  (logger)->log_debug((message), __FILE__, __LINE__, __FUNCTION__)

#define IVV_LOG_INFO(logger, message)                                          \
  (logger)->log_info((message), __FILE__, __LINE__, __FUNCTION__)

#define IVV_LOG_WARNING(logger, message)                                       \
  (logger)->log_warning((message), __FILE__, __LINE__, __FUNCTION__)

#define IVV_LOG_ERROR(logger, message)                                         \
  (logger)->log_error((message), __FILE__, __LINE__, __FUNCTION__)

#define IVV_LOG_CRITICAL(logger, message, context)                             \
  (logger)->log_critical((message), (context), __FILE__, __LINE__, __FUNCTION__)

#define IVV_LOG_FATAL(logger, message, context)                                \
  (logger)->log_fatal((message), (context), __FILE__, __LINE__, __FUNCTION__)
