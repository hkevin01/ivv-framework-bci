/**
 * @file logger.cpp
 * @brief Safety-Critical Logging System Implementation
 *
 * Implementation of the logging system with safety-critical features
 * for the IV&V Framework.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 */

#include "logger.h"
#include <atomic>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

namespace IVVFramework {
namespace Core {

/**
 * @brief Private implementation class
 */
class Logger::Impl {
public:
  mutable std::mutex log_mutex_;
  std::string component_name_;
  LogConfig config_;
  std::atomic<LogLevel> current_level_{LogLevel::INFO};
  std::atomic<bool> initialized_{false};

  std::vector<LogFilterCallback> filter_callbacks_;
  std::vector<SafetyEventCallback> safety_callbacks_;

  Logger::LogStatistics statistics_;
  std::atomic<uint64_t> sequence_counter_{0};

  std::queue<LogEntry> pending_entries_;
  std::unique_ptr<std::thread> flush_thread_;
  std::atomic<bool> should_stop_flush_thread_{false};

  void write_log_entry(const LogEntry &entry) {
    // Write to console if configured
    for (auto dest : config_.destinations) {
      if (dest == LogDestination::CONSOLE) {
        write_to_console(entry);
      } else if (dest == LogDestination::FILE) {
        write_to_file(entry);
      }
      // Other destinations would be implemented here
    }

    // Update statistics
    statistics_.total_entries++;
    if (entry.is_safety_critical) {
      statistics_.safety_critical_entries++;
    }
    if (entry.level >= LogLevel::ERROR) {
      statistics_.error_entries++;
    }

    if (statistics_.total_entries == 1) {
      statistics_.first_entry_time = entry.timestamp;
    }
    statistics_.last_entry_time = entry.timestamp;
  }

  void write_to_console(const LogEntry &entry) {
    std::cout << format_log_entry(entry) << std::endl;
    if (config_.real_time_flush) {
      std::cout.flush();
    }
  }

  void write_to_file(const LogEntry &entry) {
    std::ofstream file(config_.log_file_path, std::ios::app);
    if (file.is_open()) {
      file << format_log_entry(entry) << std::endl;
      if (config_.real_time_flush) {
        file.flush();
      }
    }
  }

  std::string format_log_entry(const LogEntry &entry) {
    std::ostringstream oss;
    oss << LogUtils::format_timestamp(entry.timestamp) << " ";
    oss << "[" << LogUtils::log_level_to_string(entry.level) << "] ";
    oss << "[" << component_name_ << "] ";
    if (!entry.category.empty()) {
      oss << "[" << entry.category << "] ";
    }
    oss << entry.message;

    if (entry.is_safety_critical) {
      oss << " [SAFETY_CRITICAL]";
    }

    return oss.str();
  }

  void flush_loop() {
    while (!should_stop_flush_thread_.load()) {
      {
        std::lock_guard<std::mutex> lock(log_mutex_);
        while (!pending_entries_.empty()) {
          auto entry = pending_entries_.front();
          pending_entries_.pop();
          write_log_entry(entry);
        }
      }

      std::this_thread::sleep_for(config_.flush_interval);
    }
  }
};

Logger::Logger() : pimpl_(std::make_unique<Impl>()) {}

Logger::~Logger() {
  if (pimpl_->flush_thread_) {
    pimpl_->should_stop_flush_thread_.store(true);
    if (pimpl_->flush_thread_->joinable()) {
      pimpl_->flush_thread_->join();
    }
  }
  flush(); // Final flush
}

bool Logger::initialize(const std::string &component_name,
                        const LogConfig &config) {
  if (component_name.empty()) {
    return false;
  }

  std::lock_guard<std::mutex> lock(pimpl_->log_mutex_);

  pimpl_->component_name_ = component_name;
  pimpl_->config_ = config;
  pimpl_->current_level_.store(config.min_level);

  // Set default destinations if none specified
  if (pimpl_->config_.destinations.empty()) {
    pimpl_->config_.destinations.push_back(LogDestination::CONSOLE);
    if (!pimpl_->config_.log_file_path.empty()) {
      pimpl_->config_.destinations.push_back(LogDestination::FILE);
    }
  }

  // Start flush thread if needed
  if (!pimpl_->config_.real_time_flush) {
    pimpl_->flush_thread_ =
        std::make_unique<std::thread>(&Impl::flush_loop, pimpl_.get());
  }

  pimpl_->initialized_.store(true);
  return true;
}

void Logger::log_trace(const std::string &message, const char *file_name,
                       int line_number, const char *function_name) {
  log_message(LogLevel::TRACE, message, "", file_name, line_number,
              function_name);
}

void Logger::log_debug(const std::string &message, const char *file_name,
                       int line_number, const char *function_name) {
  log_message(LogLevel::DEBUG, message, "", file_name, line_number,
              function_name);
}

void Logger::log_info(const std::string &message, const char *file_name,
                      int line_number, const char *function_name) {
  log_message(LogLevel::INFO, message, "", file_name, line_number,
              function_name);
}

void Logger::log_warning(const std::string &message, const char *file_name,
                         int line_number, const char *function_name) {
  log_message(LogLevel::WARNING, message, "", file_name, line_number,
              function_name);
}

void Logger::log_error(const std::string &message, const char *file_name,
                       int line_number, const char *function_name) {
  log_message(LogLevel::ERROR, message, "", file_name, line_number,
              function_name);
}

void Logger::log_critical(const std::string &message,
                          const std::string &safety_context,
                          const char *file_name, int line_number,
                          const char *function_name) {
  log_message(LogLevel::CRITICAL, message, safety_context, file_name,
              line_number, function_name, true);
}

void Logger::log_fatal(const std::string &message,
                       const std::string &safety_context, const char *file_name,
                       int line_number, const char *function_name) {
  log_message(LogLevel::FATAL, message, safety_context, file_name, line_number,
              function_name, true);
}

void Logger::log_message(LogLevel level, const std::string &message,
                         const std::string &safety_context,
                         const char *file_name, int line_number,
                         const char *function_name, bool is_safety_critical) {
  if (!pimpl_->initialized_.load() || level < pimpl_->current_level_.load()) {
    return;
  }

  LogEntry entry;
  entry.timestamp = std::chrono::system_clock::now();
  entry.level = level;
  entry.message = message;
  entry.safety_context = safety_context;
  entry.is_safety_critical = is_safety_critical;
  entry.sequence_number = pimpl_->sequence_counter_.fetch_add(1);

  if (file_name) {
    entry.file_name = file_name;
  }
  entry.line_number = line_number;
  if (function_name) {
    entry.function_name = function_name;
  }

  // Apply filters
  bool should_log = true;
  for (const auto &filter : pimpl_->filter_callbacks_) {
    if (!filter(entry)) {
      should_log = false;
      break;
    }
  }

  if (!should_log) {
    return;
  }

  // Calculate checksum for integrity
  entry.checksum = LogUtils::calculate_checksum(entry);

  {
    std::lock_guard<std::mutex> lock(pimpl_->log_mutex_);

    if (pimpl_->config_.real_time_flush) {
      pimpl_->write_log_entry(entry);
    } else {
      pimpl_->pending_entries_.push(entry);
    }
  }

  // Trigger safety callbacks for critical events
  if (is_safety_critical) {
    for (const auto &callback : pimpl_->safety_callbacks_) {
      try {
        callback(entry);
      } catch (...) {
        // Safety callbacks must not throw
      }
    }
  }
}

void Logger::set_log_level(LogLevel level) {
  pimpl_->current_level_.store(level);
}

LogLevel Logger::get_log_level() const { return pimpl_->current_level_.load(); }

void Logger::register_filter_callback(LogFilterCallback callback) {
  if (callback) {
    std::lock_guard<std::mutex> lock(pimpl_->log_mutex_);
    pimpl_->filter_callbacks_.push_back(callback);
  }
}

void Logger::register_safety_callback(SafetyEventCallback callback) {
  if (callback) {
    std::lock_guard<std::mutex> lock(pimpl_->log_mutex_);
    pimpl_->safety_callbacks_.push_back(callback);
  }
}

bool Logger::flush() {
  std::lock_guard<std::mutex> lock(pimpl_->log_mutex_);

  while (!pimpl_->pending_entries_.empty()) {
    auto entry = pimpl_->pending_entries_.front();
    pimpl_->pending_entries_.pop();
    pimpl_->write_log_entry(entry);
  }

  return true;
}

Logger::LogStatistics Logger::get_statistics() const {
  std::lock_guard<std::mutex> lock(pimpl_->log_mutex_);
  return pimpl_->statistics_;
}

bool Logger::verify_audit_trail_integrity() const {
  // Implementation would verify checksums and detect tampering
  return true;
}

bool Logger::archive_old_logs() {
  // Implementation would archive old log files
  return true;
}

bool Logger::emergency_log(const std::string &message) noexcept {
  try {
    // Emergency logging bypasses normal queuing
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = LogLevel::FATAL;
    entry.message = "[EMERGENCY] " + message;
    entry.is_safety_critical = true;
    entry.sequence_number = pimpl_->sequence_counter_.fetch_add(1);

    std::cout << "[EMERGENCY] " << message << std::endl;
    std::cout.flush();

    return true;
  } catch (...) {
    return false;
  }
}

// Utility functions implementation
namespace LogUtils {

std::string log_level_to_string(LogLevel level) {
  switch (level) {
  case LogLevel::TRACE:
    return "TRACE";
  case LogLevel::DEBUG:
    return "DEBUG";
  case LogLevel::INFO:
    return "INFO";
  case LogLevel::WARNING:
    return "WARN";
  case LogLevel::ERROR:
    return "ERROR";
  case LogLevel::CRITICAL:
    return "CRITICAL";
  case LogLevel::FATAL:
    return "FATAL";
  default:
    return "UNKNOWN";
  }
}

LogLevel string_to_log_level(const std::string &level_str) {
  if (level_str == "TRACE")
    return LogLevel::TRACE;
  if (level_str == "DEBUG")
    return LogLevel::DEBUG;
  if (level_str == "INFO")
    return LogLevel::INFO;
  if (level_str == "WARN" || level_str == "WARNING")
    return LogLevel::WARNING;
  if (level_str == "ERROR")
    return LogLevel::ERROR;
  if (level_str == "CRITICAL")
    return LogLevel::CRITICAL;
  if (level_str == "FATAL")
    return LogLevel::FATAL;

  throw std::invalid_argument("Invalid log level: " + level_str);
}

std::string format_timestamp(std::chrono::system_clock::time_point timestamp) {
  auto time_t = std::chrono::system_clock::to_time_t(timestamp);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                timestamp.time_since_epoch()) %
            1000;

  std::ostringstream oss;
  oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
  oss << "." << std::setfill('0') << std::setw(3) << ms.count();

  return oss.str();
}

std::string calculate_checksum(const LogEntry &entry) {
  // Simple checksum calculation for integrity checking
  std::ostringstream oss;
  oss << entry.sequence_number << static_cast<int>(entry.level)
      << entry.message;

  std::hash<std::string> hasher;
  auto hash = hasher(oss.str());

  std::ostringstream checksum_oss;
  checksum_oss << std::hex << hash;

  return checksum_oss.str();
}

} // namespace LogUtils

} // namespace Core
} // namespace IVVFramework
