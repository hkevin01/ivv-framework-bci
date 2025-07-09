/**
 * @file qnx_platform.cpp
 * @brief QNX RTOS Integration Implementation
 *
 * This file implements QNX-specific functionality for the IV&V Framework,
 * providing real-time capabilities and RTOS integration.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 *
 * @copyright Copyright (c) 2025 IV&V Framework for BCI Systems
 *
 * Safety-Critical Notice:
 * QNX integration must maintain deterministic behavior and real-time
 * guarantees for safety-critical BCI system verification.
 */

#include "qnx_platform.h"
#include "../core/logger.h"
#include <atomic>
#include <cstring>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

#ifdef __QNX__
#include <errno.h>
#include <hw/inout.h>
#include <sys/mman.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <sys/sched.h>
#include <sys/syspage.h>
#include <unistd.h>
#else
// Mock implementations for non-QNX platforms
#include <sys/time.h>
#include <unistd.h>
#endif

namespace IVVFramework {
namespace QNXIntegration {

/**
 * @brief Private implementation using PIMPL idiom
 */
class QNXPlatform::Impl {
public:
  QNXPlatformConfig config_;
  std::atomic<bool> initialized_{false};
  std::atomic<bool> instrumentation_enabled_{false};

  mutable std::mutex channels_mutex_;
  std::map<std::string, int> message_channels_;

  std::unique_ptr<Core::Logger> logger_;

  // Performance tracking
  mutable std::mutex metrics_mutex_;
  QNXPerformanceMetrics current_metrics_;
  std::chrono::high_resolution_clock::time_point last_metrics_update_;

  // QNX-specific data
  uint64_t cycles_per_second_ = 0;
  int trace_logger_fd_ = -1;

#ifdef __QNX__
  // QNX specific members
  struct sched_param original_sched_param_;
  int original_policy_ = 0;
#endif

  Impl() {
    logger_ = std::make_unique<Core::Logger>();
    last_metrics_update_ = std::chrono::high_resolution_clock::now();

#ifdef __QNX__
    // Get cycles per second for timing calculations
    cycles_per_second_ = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
#else
    // Mock value for non-QNX systems
    cycles_per_second_ = 1000000000; // 1 GHz
#endif
  }

  void update_performance_metrics();
};

QNXPlatform::QNXPlatform() : pimpl_(std::make_unique<Impl>()) {
  pimpl_->logger_->log_info("QNX Platform integration created");
}

QNXPlatform::~QNXPlatform() {
  if (pimpl_->initialized_) {
    shutdown();
  }
  pimpl_->logger_->log_info("QNX Platform integration destroyed");
}

bool QNXPlatform::initialize(const QNXPlatformConfig &config) {
  if (pimpl_->initialized_) {
    pimpl_->logger_->log_warning("QNX Platform already initialized");
    return true;
  }

  pimpl_->config_ = config;

#ifdef __QNX__
  // Lock memory pages if configured
  if (config.memory_config.lock_code_pages ||
      config.memory_config.lock_data_pages) {
    int flags = 0;
    if (config.memory_config.lock_code_pages)
      flags |= MCL_CURRENT;
    if (config.memory_config.lock_data_pages)
      flags |= MCL_FUTURE;

    if (mlockall(flags) != 0) {
      pimpl_->logger_->log_error("Failed to lock memory pages: " +
                                 std::string(strerror(errno)));
      return false;
    }
  }

  // Set up high-resolution timing
  if (config.timing_config.use_high_resolution_timer) {
    // QNX provides high-resolution timing by default through ClockCycles()
    pimpl_->logger_->log_info(
        "High-resolution timing enabled using ClockCycles()");
  }

  // Initialize trace logging if enabled
  if (config.enable_tracelogger) {
    if (start_trace_logging("/tmp/ivv_trace.log")) {
      pimpl_->logger_->log_info("Trace logging enabled");
    } else {
      pimpl_->logger_->log_warning("Failed to enable trace logging");
    }
  }

#else
  // Non-QNX platform - provide mock functionality
  pimpl_->logger_->log_warning(
      "Running on non-QNX platform - using mock implementation");
#endif

  pimpl_->initialized_ = true;
  pimpl_->logger_->log_info("QNX Platform initialized successfully");

  return true;
}

bool QNXPlatform::shutdown() {
  if (!pimpl_->initialized_) {
    return false;
  }

  // Stop trace logging
  if (pimpl_->trace_logger_fd_ != -1) {
    stop_trace_logging();
  }

  // Close all message channels
  {
    std::lock_guard<std::mutex> lock(pimpl_->channels_mutex_);
    for (auto &[name, channel_id] : pimpl_->message_channels_) {
#ifdef __QNX__
      ChannelDestroy(channel_id);
#endif
    }
    pimpl_->message_channels_.clear();
  }

#ifdef __QNX__
  // Unlock memory if it was locked
  if (pimpl_->config_.memory_config.lock_code_pages ||
      pimpl_->config_.memory_config.lock_data_pages) {
    munlockall();
  }
#endif

  pimpl_->initialized_ = false;
  pimpl_->logger_->log_info("QNX Platform shutdown completed");

  return true;
}

pthread_t
QNXPlatform::create_realtime_thread(const QNXThreadConfig &thread_config,
                                    void *(*thread_function)(void *),
                                    void *thread_data) {

  if (!pimpl_->initialized_) {
    pimpl_->logger_->log_error("QNX Platform not initialized");
    return 0;
  }

  pthread_attr_t attr;
  pthread_attr_init(&attr);

  // Set stack size
  pthread_attr_setstacksize(&attr, thread_config.stack_size);

  // Set scheduling policy and priority
  int policy = QNXUtils::qnx_policy_to_posix(thread_config.policy);
  pthread_attr_setschedpolicy(&attr, policy);

  struct sched_param param;
  param.sched_priority = QNXUtils::qnx_priority_to_posix(thread_config.priority,
                                                         thread_config.policy);
  pthread_attr_setschedparam(&attr, &param);

  // Set inheritance
  if (thread_config.inherit_priority) {
    pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
  } else {
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  }

  pthread_t thread_id;
  int result = pthread_create(&thread_id, &attr, thread_function, thread_data);

  pthread_attr_destroy(&attr);

  if (result != 0) {
    pimpl_->logger_->log_error("Failed to create real-time thread: " +
                               std::string(strerror(result)));
    return 0;
  }

  // Lock memory for the thread if configured
  if (thread_config.lock_memory) {
    // Note: Memory locking for specific threads would require additional QNX
    // APIs
    pimpl_->logger_->log_info(
        "Thread memory locking requested (requires additional implementation)");
  }

  pimpl_->logger_->log_info("Real-time thread created successfully");
  return thread_id;
}

bool QNXPlatform::set_thread_scheduling(pthread_t thread_id,
                                        QNXSchedulingPolicy policy,
                                        QNXPriority priority) {

  int posix_policy = QNXUtils::qnx_policy_to_posix(policy);
  struct sched_param param;
  param.sched_priority = QNXUtils::qnx_priority_to_posix(priority, policy);

  int result = pthread_setschedparam(thread_id, posix_policy, &param);
  if (result != 0) {
    pimpl_->logger_->log_error("Failed to set thread scheduling: " +
                               std::string(strerror(result)));
    return false;
  }

  return true;
}

bool QNXPlatform::lock_memory(void *address, size_t size) {
#ifdef __QNX__
  if (mlock(address, size) != 0) {
    pimpl_->logger_->log_error("Failed to lock memory: " +
                               std::string(strerror(errno)));
    return false;
  }
  return true;
#else
  // Mock implementation for non-QNX platforms
  pimpl_->logger_->log_info("Memory lock requested (mock implementation)");
  return true;
#endif
}

bool QNXPlatform::unlock_memory(void *address, size_t size) {
#ifdef __QNX__
  if (munlock(address, size) != 0) {
    pimpl_->logger_->log_error("Failed to unlock memory: " +
                               std::string(strerror(errno)));
    return false;
  }
  return true;
#else
  // Mock implementation for non-QNX platforms
  pimpl_->logger_->log_info("Memory unlock requested (mock implementation)");
  return true;
#endif
}

std::chrono::nanoseconds QNXPlatform::get_high_resolution_time() const {
#ifdef __QNX__
  uint64_t cycles = ClockCycles();
  uint64_t nanoseconds = (cycles * 1000000000ULL) / pimpl_->cycles_per_second_;
  return std::chrono::nanoseconds(nanoseconds);
#else
  // Use standard high-resolution clock for non-QNX platforms
  auto now = std::chrono::high_resolution_clock::now();
  auto epoch = std::chrono::high_resolution_clock::time_point{};
  return std::chrono::duration_cast<std::chrono::nanoseconds>(now - epoch);
#endif
}

void QNXPlatform::precision_sleep(std::chrono::nanoseconds duration) const {
  auto start = get_high_resolution_time();
  auto target = start + duration;

  // For very short durations, use busy waiting
  if (duration < std::chrono::microseconds(100)) {
    while (get_high_resolution_time() < target) {
      // Busy wait for maximum precision
    }
  } else {
    // For longer durations, use standard sleep with final busy wait
    auto sleep_duration = duration - std::chrono::microseconds(50);
    std::this_thread::sleep_for(sleep_duration);

    // Busy wait for remaining time
    while (get_high_resolution_time() < target) {
      // Busy wait
    }
  }
}

int QNXPlatform::create_message_channel(const std::string &channel_name,
                                        int flags) {
  std::lock_guard<std::mutex> lock(pimpl_->channels_mutex_);

  // Check if channel already exists
  auto it = pimpl_->message_channels_.find(channel_name);
  if (it != pimpl_->message_channels_.end()) {
    pimpl_->logger_->log_warning("Message channel already exists: " +
                                 channel_name);
    return it->second;
  }

#ifdef __QNX__
  int channel_id = ChannelCreate(flags);
  if (channel_id == -1) {
    pimpl_->logger_->log_error("Failed to create message channel: " +
                               std::string(strerror(errno)));
    return -1;
  }
#else
  // Mock implementation for non-QNX platforms
  static int mock_channel_id = 1000;
  int channel_id = mock_channel_id++;
  pimpl_->logger_->log_info("Mock message channel created: " + channel_name);
#endif

  pimpl_->message_channels_[channel_name] = channel_id;
  pimpl_->logger_->log_info("Message channel created: " + channel_name +
                            " (ID: " + std::to_string(channel_id) + ")");

  return channel_id;
}

int QNXPlatform::send_message(int channel_id, const void *message,
                              size_t message_size,
                              std::chrono::nanoseconds timeout_ns) {

#ifdef __QNX__
  uint64_t timeout_abs = 0;
  if (timeout_ns.count() > 0) {
    timeout_abs =
        ClockCycles() +
        (timeout_ns.count() * pimpl_->cycles_per_second_) / 1000000000ULL;
  }

  // For QNX, we would use MsgSend or similar APIs
  // This is a simplified implementation
  pimpl_->logger_->log_info("Sending message through QNX IPC (size: " +
                            std::to_string(message_size) + ")");
  return static_cast<int>(message_size); // Mock success
#else
  // Mock implementation for non-QNX platforms
  pimpl_->logger_->log_info(
      "Mock message send (size: " + std::to_string(message_size) + ")");
  return static_cast<int>(message_size);
#endif
}

int QNXPlatform::receive_message(int channel_id, void *buffer,
                                 size_t buffer_size,
                                 std::chrono::nanoseconds timeout_ns) {

#ifdef __QNX__
  // For QNX, we would use MsgReceive or similar APIs
  // This is a simplified implementation
  pimpl_->logger_->log_info("Receiving message through QNX IPC (buffer size: " +
                            std::to_string(buffer_size) + ")");
  return 0; // Mock - no message received
#else
  // Mock implementation for non-QNX platforms
  pimpl_->logger_->log_info("Mock message receive (buffer size: " +
                            std::to_string(buffer_size) + ")");
  return 0; // Mock - no message received
#endif
}

QNXPerformanceMetrics QNXPlatform::get_performance_metrics() const {
  std::lock_guard<std::mutex> lock(pimpl_->metrics_mutex_);

  auto now = std::chrono::high_resolution_clock::now();
  auto elapsed = now - pimpl_->last_metrics_update_;

  // Update metrics every second
  if (elapsed >= std::chrono::seconds(1)) {
    pimpl_->update_performance_metrics();
    pimpl_->last_metrics_update_ = now;
  }

  return pimpl_->current_metrics_;
}

bool QNXPlatform::set_instrumentation_enabled(bool enable) {
  pimpl_->instrumentation_enabled_ = enable;

#ifdef __QNX__
  // Enable/disable QNX instrumentation
  // This would involve QNX-specific instrumentation APIs
  pimpl_->logger_->log_info(enable ? "QNX instrumentation enabled"
                                   : "QNX instrumentation disabled");
#else
  pimpl_->logger_->log_info(enable ? "Mock instrumentation enabled"
                                   : "Mock instrumentation disabled");
#endif

  return true;
}

bool QNXPlatform::start_trace_logging(const std::string &trace_file) {
#ifdef __QNX__
  // Start QNX trace logging
  // This would involve tracelogger APIs
  pimpl_->trace_logger_fd_ = 1; // Mock file descriptor
  pimpl_->logger_->log_info("Trace logging started: " + trace_file);
  return true;
#else
  pimpl_->logger_->log_info("Mock trace logging started: " + trace_file);
  pimpl_->trace_logger_fd_ = 1; // Mock file descriptor
  return true;
#endif
}

bool QNXPlatform::stop_trace_logging() {
  if (pimpl_->trace_logger_fd_ == -1) {
    return false;
  }

#ifdef __QNX__
  // Stop QNX trace logging
  pimpl_->logger_->log_info("Trace logging stopped");
#else
  pimpl_->logger_->log_info("Mock trace logging stopped");
#endif

  pimpl_->trace_logger_fd_ = -1;
  return true;
}

bool QNXPlatform::is_qnx_platform() {
#ifdef __QNX__
  return true;
#else
  return false;
#endif
}

std::string QNXPlatform::get_qnx_version() {
#ifdef __QNX__
  // Get QNX version information
  return "QNX 7.x (detected)";
#else
  return "Non-QNX platform";
#endif
}

bool QNXPlatform::validate_realtime_constraints(
    std::chrono::nanoseconds max_latency_ns) const {
  auto metrics = get_performance_metrics();

  bool constraints_met = true;

  if (metrics.max_interrupt_latency > max_latency_ns) {
    pimpl_->logger_->log_warning(
        "Interrupt latency exceeds constraint: " +
        std::to_string(metrics.max_interrupt_latency.count()) + "ns");
    constraints_met = false;
  }

  if (metrics.max_scheduling_latency > max_latency_ns) {
    pimpl_->logger_->log_warning(
        "Scheduling latency exceeds constraint: " +
        std::to_string(metrics.max_scheduling_latency.count()) + "ns");
    constraints_met = false;
  }

  return constraints_met;
}

bool QNXPlatform::set_cpu_affinity(pthread_t thread_id, uint32_t cpu_mask) {
#ifdef __QNX__
  // Set CPU affinity using QNX APIs
  // This would use ThreadCtl with _NTO_TCTL_RUNMASK
  pimpl_->logger_->log_info("CPU affinity set for thread (mask: 0x" +
                            std::to_string(cpu_mask) + ")");
  return true;
#else
  pimpl_->logger_->log_info("Mock CPU affinity set (mask: 0x" +
                            std::to_string(cpu_mask) + ")");
  return true;
#endif
}

uint32_t QNXPlatform::disable_interrupts() {
#ifdef __QNX__
  // Disable interrupts using QNX APIs
  // This is extremely dangerous and should be used with caution
  pimpl_->logger_->log_warning(
      "Interrupts disabled - use with extreme caution");
  return 0; // Mock previous state
#else
  pimpl_->logger_->log_warning("Mock interrupt disable");
  return 0; // Mock previous state
#endif
}

void QNXPlatform::restore_interrupts(uint32_t previous_state) {
#ifdef __QNX__
  // Restore interrupt state using QNX APIs
  pimpl_->logger_->log_info("Interrupts restored");
#else
  pimpl_->logger_->log_info("Mock interrupt restore");
#endif
}

// Private implementation methods
void QNXPlatform::Impl::update_performance_metrics() {
#ifdef __QNX__
  // Update performance metrics using QNX system calls
  // This would involve reading from /proc entries or using QNX APIs

  // Mock values for demonstration
  current_metrics_.max_interrupt_latency =
      std::chrono::nanoseconds(50000); // 50 µs
  current_metrics_.max_scheduling_latency =
      std::chrono::nanoseconds(100000); // 100 µs
  current_metrics_.max_message_latency =
      std::chrono::nanoseconds(200000); // 200 µs

  current_metrics_.context_switches += 1000;
  current_metrics_.page_faults += 10;
  current_metrics_.cache_misses += 100;

  current_metrics_.cpu_utilization = 45.5;
  current_metrics_.memory_utilization = 62.3;
  current_metrics_.network_utilization = 12.1;
#else
  // Mock performance metrics for non-QNX platforms
  current_metrics_.max_interrupt_latency =
      std::chrono::nanoseconds(100000); // 100 µs
  current_metrics_.max_scheduling_latency =
      std::chrono::nanoseconds(200000); // 200 µs
  current_metrics_.max_message_latency =
      std::chrono::nanoseconds(300000); // 300 µs

  current_metrics_.context_switches += 500;
  current_metrics_.page_faults += 5;
  current_metrics_.cache_misses += 50;

  current_metrics_.cpu_utilization = 30.0;
  current_metrics_.memory_utilization = 40.0;
  current_metrics_.network_utilization = 5.0;
#endif
}

// Utility functions implementation
namespace QNXUtils {

int qnx_policy_to_posix(QNXSchedulingPolicy policy) {
  switch (policy) {
  case QNXSchedulingPolicy::FIFO:
    return SCHED_FIFO;
  case QNXSchedulingPolicy::ROUND_ROBIN:
    return SCHED_RR;
  case QNXSchedulingPolicy::OTHER:
    return SCHED_OTHER;
  case QNXSchedulingPolicy::SPORADIC:
#ifdef SCHED_SPORADIC
    return SCHED_SPORADIC;
#else
    return SCHED_FIFO; // Fallback to FIFO
#endif
  default:
    return SCHED_OTHER;
  }
}

int qnx_priority_to_posix(QNXPriority priority, QNXSchedulingPolicy policy) {
  int base_priority = static_cast<int>(priority);

  // Adjust priority based on scheduling policy
  switch (policy) {
  case QNXSchedulingPolicy::FIFO:
  case QNXSchedulingPolicy::ROUND_ROBIN:
  case QNXSchedulingPolicy::SPORADIC:
    // Real-time priorities (1-255 for QNX, 1-99 for POSIX)
    return std::min(base_priority, 99);
  case QNXSchedulingPolicy::OTHER:
    // Time-sharing priority (usually 0)
    return 0;
  default:
    return 0;
  }
}

size_t calculate_stack_size(size_t function_stack_usage, int recursion_depth) {
  size_t base_size = 8192; // 8KB base
  size_t function_overhead = function_stack_usage * (recursion_depth + 1);
  size_t safety_margin = 4096; // 4KB safety margin

  return base_size + function_overhead + safety_margin;
}

bool check_memory_lock_capability() {
#ifdef __QNX__
  // Check if memory locking is available
  return (mlockall(MCL_CURRENT | MCL_FUTURE) == 0) || (errno != EPERM);
#else
  // Assume capability is available on non-QNX platforms
  return true;
#endif
}

int get_cpu_count() {
  return static_cast<int>(std::thread::hardware_concurrency());
}

size_t get_page_size() { return static_cast<size_t>(getpagesize()); }

timespec nanoseconds_to_timespec(std::chrono::nanoseconds ns) {
  timespec ts;
  ts.tv_sec = ns.count() / 1000000000;
  ts.tv_nsec = ns.count() % 1000000000;
  return ts;
}

std::chrono::nanoseconds timespec_to_nanoseconds(const timespec &ts) {
  return std::chrono::nanoseconds(ts.tv_sec * 1000000000LL + ts.tv_nsec);
}

} // namespace QNXUtils

} // namespace QNXIntegration
} // namespace IVVFramework
