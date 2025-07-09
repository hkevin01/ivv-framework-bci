/**
 * @file qnx_platform.h
 * @brief QNX RTOS Integration Layer for IV&V Framework
 *
 * This file provides QNX-specific implementations for real-time
 * operations, memory management, and safety-critical timing constraints.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 *
 * @copyright Copyright (c) 2025 IV&V Framework for BCI Systems
 *
 * Safety-Critical Notice:
 * QNX integration must maintain real-time guarantees and comply
 * with IEC 61508 functional safety requirements.
 */

#pragma once

#include "../core/verifier.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#ifdef __QNX__
#include <pthread.h>
#include <sys/mman.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <sys/sched.h>
#include <time.h>
#endif

namespace IVVFramework {
namespace QNXIntegration {

/**
 * @brief QNX-specific thread scheduling policies
 */
enum class QNXSchedulingPolicy {
  FIFO = 0,        ///< First-in-first-out (real-time)
  ROUND_ROBIN = 1, ///< Round-robin (real-time)
  OTHER = 2,       ///< Time-sharing (non-real-time)
  SPORADIC = 3     ///< Sporadic scheduling (aperiodic real-time)
};

/**
 * @brief QNX-specific thread priority levels
 */
enum class QNXPriority {
  IDLE = 1,       ///< Idle priority
  NORMAL = 10,    ///< Normal application priority
  HIGH = 50,      ///< High priority
  CRITICAL = 100, ///< Critical system priority
  INTERRUPT = 255 ///< Interrupt service priority
};

/**
 * @brief QNX real-time thread configuration
 */
struct QNXThreadConfig {
  QNXSchedulingPolicy policy = QNXSchedulingPolicy::FIFO;
  QNXPriority priority = QNXPriority::NORMAL;
  size_t stack_size = 8192;           ///< Stack size in bytes
  bool lock_memory = true;            ///< Lock memory to prevent paging
  bool inherit_priority = false;      ///< Inherit priority from parent
  std::chrono::nanoseconds budget{0}; ///< Time budget for sporadic scheduling
  std::chrono::nanoseconds period{0}; ///< Period for sporadic scheduling
};

/**
 * @brief QNX memory management configuration
 */
struct QNXMemoryConfig {
  bool lock_code_pages = true;         ///< Lock code pages in memory
  bool lock_data_pages = true;         ///< Lock data pages in memory
  bool use_typed_memory = false;       ///< Use typed memory objects
  size_t heap_size = 1024 * 1024;      ///< Heap size in bytes
  bool enable_stack_protection = true; ///< Enable stack overflow protection
};

/**
 * @brief QNX timing and synchronization configuration
 */
struct QNXTimingConfig {
  bool use_high_resolution_timer = true; ///< Use high-resolution ClockCycles()
  bool enable_clockselect = true;        ///< Enable clock selection
  int clock_id = CLOCK_MONOTONIC;        ///< Clock ID for timing
  std::chrono::nanoseconds timer_resolution{1000}; ///< Timer resolution
};

/**
 * @brief QNX IPC (Inter-Process Communication) configuration
 */
struct QNXIPCConfig {
  bool use_message_passing = true; ///< Use QNX message passing
  bool use_shared_memory = false;  ///< Use shared memory IPC
  bool use_signals = false;        ///< Use POSIX signals
  size_t max_message_size = 4096;  ///< Maximum message size
  int channel_flags = 0;           ///< Channel creation flags
};

/**
 * @brief QNX platform configuration
 */
struct QNXPlatformConfig {
  QNXThreadConfig thread_config;
  QNXMemoryConfig memory_config;
  QNXTimingConfig timing_config;
  QNXIPCConfig ipc_config;

  std::string node_name;               ///< QNX node name
  std::string network_manager;         ///< Network manager configuration
  bool enable_instrumentation = false; ///< Enable QNX instrumentation
  bool enable_tracelogger = true;      ///< Enable trace logging
};

/**
 * @brief QNX real-time performance metrics
 */
struct QNXPerformanceMetrics {
  std::chrono::nanoseconds max_interrupt_latency{0};
  std::chrono::nanoseconds max_scheduling_latency{0};
  std::chrono::nanoseconds max_message_latency{0};

  uint64_t context_switches = 0;
  uint64_t page_faults = 0;
  uint64_t cache_misses = 0;

  double cpu_utilization = 0.0;
  double memory_utilization = 0.0;
  double network_utilization = 0.0;
};

/**
 * @class QNXPlatform
 * @brief QNX RTOS integration layer for the IV&V Framework
 *
 * Provides QNX-specific implementations for real-time operations,
 * memory management, timing analysis, and inter-process communication
 * required for safety-critical BCI system verification.
 */
class QNXPlatform {
public:
  /**
   * @brief Constructor
   */
  QNXPlatform();

  /**
   * @brief Destructor
   */
  ~QNXPlatform();

  /**
   * @brief Initialize QNX platform integration
   * @param config QNX platform configuration
   * @return true if initialization successful, false otherwise
   * @pre QNX system must be running
   * @post Platform is ready for real-time operations
   */
  bool initialize(const QNXPlatformConfig &config);

  /**
   * @brief Shutdown QNX platform integration
   * @return true if shutdown successful, false otherwise
   * @post All resources are released
   */
  bool shutdown();

  /**
   * @brief Create real-time thread with QNX scheduling
   * @param thread_config Thread configuration
   * @param thread_function Function to execute in thread
   * @param thread_data Data to pass to thread function
   * @return Thread ID on success, 0 on failure
   * @pre Platform must be initialized
   */
  pthread_t create_realtime_thread(const QNXThreadConfig &thread_config,
                                   void *(*thread_function)(void *),
                                   void *thread_data);

  /**
   * @brief Set thread priority and scheduling policy
   * @param thread_id Thread ID
   * @param policy Scheduling policy
   * @param priority Thread priority
   * @return true if successful, false otherwise
   */
  bool set_thread_scheduling(pthread_t thread_id, QNXSchedulingPolicy policy,
                             QNXPriority priority);

  /**
   * @brief Lock memory regions to prevent paging
   * @param address Memory address to lock
   * @param size Size of memory region
   * @return true if successful, false otherwise
   * @note Critical for real-time performance
   */
  bool lock_memory(void *address, size_t size);

  /**
   * @brief Unlock previously locked memory
   * @param address Memory address to unlock
   * @param size Size of memory region
   * @return true if successful, false otherwise
   */
  bool unlock_memory(void *address, size_t size);

  /**
   * @brief Get high-resolution timestamp
   * @return Timestamp in nanoseconds since system boot
   * @note Uses QNX ClockCycles() for maximum precision
   */
  std::chrono::nanoseconds get_high_resolution_time() const;

  /**
   * @brief Sleep with high precision (busy wait)
   * @param duration Sleep duration
   * @note Uses busy waiting for sub-microsecond precision
   */
  void precision_sleep(std::chrono::nanoseconds duration) const;

  /**
   * @brief Create QNX message channel
   * @param channel_name Channel name
   * @param flags Channel creation flags
   * @return Channel ID on success, -1 on failure
   */
  int create_message_channel(const std::string &channel_name, int flags = 0);

  /**
   * @brief Send message through QNX IPC
   * @param channel_id Channel ID
   * @param message Message data
   * @param message_size Message size in bytes
   * @param timeout_ns Timeout in nanoseconds
   * @return Number of bytes sent, -1 on error
   */
  int send_message(
      int channel_id, const void *message, size_t message_size,
      std::chrono::nanoseconds timeout_ns = std::chrono::nanoseconds::zero());

  /**
   * @brief Receive message through QNX IPC
   * @param channel_id Channel ID
   * @param buffer Buffer to receive message
   * @param buffer_size Buffer size in bytes
   * @param timeout_ns Timeout in nanoseconds
   * @return Number of bytes received, -1 on error
   */
  int receive_message(
      int channel_id, void *buffer, size_t buffer_size,
      std::chrono::nanoseconds timeout_ns = std::chrono::nanoseconds::zero());

  /**
   * @brief Get current performance metrics
   * @return Current QNX performance metrics
   */
  QNXPerformanceMetrics get_performance_metrics() const;

  /**
   * @brief Enable/disable QNX instrumentation
   * @param enable Whether to enable instrumentation
   * @return true if successful, false otherwise
   */
  bool set_instrumentation_enabled(bool enable);

  /**
   * @brief Start trace logging for performance analysis
   * @param trace_file Path to trace file
   * @return true if successful, false otherwise
   */
  bool start_trace_logging(const std::string &trace_file);

  /**
   * @brief Stop trace logging
   * @return true if successful, false otherwise
   */
  bool stop_trace_logging();

  /**
   * @brief Check if running on QNX
   * @return true if running on QNX, false otherwise
   */
  static bool is_qnx_platform();

  /**
   * @brief Get QNX version information
   * @return QNX version string
   */
  static std::string get_qnx_version();

  /**
   * @brief Validate real-time constraints
   * @param max_latency_ns Maximum acceptable latency
   * @return true if constraints are met, false otherwise
   */
  bool
  validate_realtime_constraints(std::chrono::nanoseconds max_latency_ns) const;

  /**
   * @brief Set CPU affinity for thread
   * @param thread_id Thread ID
   * @param cpu_mask CPU affinity mask
   * @return true if successful, false otherwise
   */
  bool set_cpu_affinity(pthread_t thread_id, uint32_t cpu_mask);

  /**
   * @brief Disable interrupts (for critical sections)
   * @return Previous interrupt state
   * @warning Use with extreme caution - affects system responsiveness
   */
  uint32_t disable_interrupts();

  /**
   * @brief Restore interrupt state
   * @param previous_state Previous interrupt state from disable_interrupts()
   */
  void restore_interrupts(uint32_t previous_state);

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

/**
 * @brief QNX utility functions
 */
namespace QNXUtils {
/**
 * @brief Convert QNX scheduling policy to POSIX
 * @param policy QNX scheduling policy
 * @return POSIX scheduling policy
 */
int qnx_policy_to_posix(QNXSchedulingPolicy policy);

/**
 * @brief Convert QNX priority to POSIX priority
 * @param priority QNX priority
 * @param policy Scheduling policy
 * @return POSIX priority value
 */
int qnx_priority_to_posix(QNXPriority priority, QNXSchedulingPolicy policy);

/**
 * @brief Calculate optimal thread stack size
 * @param function_stack_usage Estimated function stack usage
 * @param recursion_depth Maximum recursion depth
 * @return Recommended stack size
 */
size_t calculate_stack_size(size_t function_stack_usage,
                            int recursion_depth = 0);

/**
 * @brief Check memory lock capabilities
 * @return true if memory locking is available, false otherwise
 */
bool check_memory_lock_capability();

/**
 * @brief Get available CPU cores
 * @return Number of available CPU cores
 */
int get_cpu_count();

/**
 * @brief Get system page size
 * @return System page size in bytes
 */
size_t get_page_size();

/**
 * @brief Convert nanoseconds to QNX timespec
 * @param ns Nanoseconds
 * @return QNX timespec structure
 */
timespec nanoseconds_to_timespec(std::chrono::nanoseconds ns);

/**
 * @brief Convert QNX timespec to nanoseconds
 * @param ts QNX timespec structure
 * @return Nanoseconds
 */
std::chrono::nanoseconds timespec_to_nanoseconds(const timespec &ts);
} // namespace QNXUtils

} // namespace QNXIntegration
} // namespace IVVFramework
