/**
 * @file timing_analyzer.h
 * @brief Real-time Timing Analysis Engine for BCI Safety-Critical Systems
 *
 * Provides comprehensive timing verification capabilities including deadline
 * analysis, jitter measurement, latency profiling, and worst-case execution
 * time analysis specifically designed for BCI safety-critical applications.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace IVVFramework {
namespace TimingAnalysis {

/**
 * @brief Time unit enumeration for precise timing specifications
 */
enum class TimeUnit {
  NANOSECONDS = 0,
  MICROSECONDS = 1,
  MILLISECONDS = 2,
  SECONDS = 3
};

/**
 * @brief Timing constraint specification
 */
struct TimingConstraint {
  std::string name;                    ///< Constraint identifier
  std::chrono::nanoseconds deadline;   ///< Maximum allowed execution time
  std::chrono::nanoseconds period;     ///< Execution period for periodic tasks
  std::chrono::nanoseconds max_jitter; ///< Maximum allowed timing jitter
  std::chrono::nanoseconds min_separation; ///< Minimum time between executions
  bool is_critical_path = false;           ///< Whether this is safety-critical
  double deadline_miss_threshold =
      0.001; ///< Acceptable deadline miss rate (0.1%)
};

/**
 * @brief Timing measurement result
 */
struct TimingMeasurement {
  std::string task_name;                            ///< Task or component name
  std::chrono::steady_clock::time_point start_time; ///< Measurement start
  std::chrono::steady_clock::time_point end_time;   ///< Measurement end
  std::chrono::nanoseconds execution_time;          ///< Actual execution time
  std::chrono::nanoseconds jitter;                  ///< Observed timing jitter
  bool deadline_met = true; ///< Whether deadline was met
  bool is_outlier = false;  ///< Statistical outlier detection
};

/**
 * @brief Real-time performance statistics
 */
struct PerformanceStatistics {
  std::string component_name;                     ///< Component identifier
  size_t measurement_count = 0;                   ///< Number of measurements
  std::chrono::nanoseconds min_execution_time{0}; ///< Minimum observed time
  std::chrono::nanoseconds max_execution_time{0}; ///< Maximum observed time
  std::chrono::nanoseconds avg_execution_time{0}; ///< Average execution time
  std::chrono::nanoseconds std_deviation{0};      ///< Standard deviation
  std::chrono::nanoseconds wcet_estimate{0};      ///< Worst-case execution time
  double deadline_miss_rate = 0.0; ///< Percentage of deadline misses
  double jitter_coefficient = 0.0; ///< Jitter variability measure
  std::vector<std::chrono::nanoseconds>
      percentiles; ///< 95th, 99th, 99.9th percentiles
};

/**
 * @brief Resource utilization metrics
 */
struct ResourceUtilization {
  std::string resource_name;        ///< Resource identifier (CPU, memory, etc.)
  double average_utilization = 0.0; ///< Average utilization percentage
  double peak_utilization = 0.0;    ///< Peak utilization percentage
  std::chrono::nanoseconds measurement_window{0}; ///< Measurement time window
  std::vector<double> utilization_samples; ///< Time-series utilization data
  bool exceeds_safety_threshold = false;   ///< Whether safety limits exceeded
};

/**
 * @brief Timing analysis report
 */
struct TimingAnalysisReport {
  std::chrono::steady_clock::time_point analysis_timestamp;
  std::string target_system;                     ///< System under analysis
  std::chrono::nanoseconds analysis_duration{0}; ///< Duration of analysis

  std::vector<PerformanceStatistics>
      component_stats;                             ///< Per-component statistics
  std::vector<ResourceUtilization> resource_stats; ///< Resource utilization
  std::vector<std::string> timing_violations;      ///< Detected violations
  std::vector<std::string> safety_concerns;        ///< Safety-critical issues

  bool overall_timing_compliance = true; ///< Overall system compliance
  double system_utilization_score = 0.0; ///< Overall system efficiency
  std::string recommendations;           ///< Performance recommendations
};

/**
 * @brief Timing verification callback for custom validation
 */
using TimingVerificationCallback =
    std::function<bool(const TimingMeasurement &, const TimingConstraint &)>;

/**
 * @brief Resource monitoring callback for external resource tracking
 */
using ResourceMonitoringCallback =
    std::function<ResourceUtilization(const std::string &resource_name)>;

/**
 * @class TimingAnalyzer
 * @brief Main timing analysis engine for real-time BCI systems
 *
 * The TimingAnalyzer provides comprehensive real-time performance analysis
 * capabilities designed specifically for safety-critical BCI systems. It
 * includes deadline analysis, jitter measurement, worst-case execution time
 * analysis, and resource utilization monitoring.
 *
 * Thread Safety: This class is thread-safe for concurrent timing measurements.
 *
 * Real-time Constraints: Measurement overhead is minimized for real-time use.
 */
class TimingAnalyzer {
public:
  /**
   * @brief Factory method to create a timing analyzer instance
   * @return Unique pointer to the created timing analyzer
   * @throws std::runtime_error if initialization fails
   */
  static std::unique_ptr<TimingAnalyzer> create();

  /**
   * @brief Virtual destructor for proper cleanup
   */
  virtual ~TimingAnalyzer() = default;

  /**
   * @brief Initialize the timing analyzer
   * @return true if initialization successful, false otherwise
   */
  virtual bool initialize() = 0;

  /**
   * @brief Configure timing constraints for a component
   * @param component_name Name of the component to monitor
   * @param constraints Timing constraints to enforce
   * @return true if configuration successful, false otherwise
   */
  virtual bool configure_constraints(const std::string &component_name,
                                     const TimingConstraint &constraints) = 0;

  /**
   * @brief Start timing measurement for a component
   * @param component_name Name of the component being measured
   * @return Measurement ID for stopping the measurement
   */
  virtual uint64_t start_measurement(const std::string &component_name) = 0;

  /**
   * @brief Stop timing measurement and record results
   * @param measurement_id ID returned from start_measurement
   * @return Timing measurement result
   */
  virtual TimingMeasurement stop_measurement(uint64_t measurement_id) = 0;

  /**
   * @brief Measure execution time of a callable
   * @param component_name Name of the component being measured
   * @param callable Function or lambda to measure
   * @return Timing measurement result
   */
  template <typename Callable>
  TimingMeasurement measure_execution(const std::string &component_name,
                                      Callable &&callable) {
    auto id = start_measurement(component_name);
    try {
      callable();
    } catch (...) {
      stop_measurement(id); // Ensure measurement is stopped
      throw;
    }
    return stop_measurement(id);
  }

  /**
   * @brief Analyze deadline compliance for a component
   * @param component_name Name of the component to analyze
   * @param analysis_window Time window for analysis
   * @return Performance statistics for the component
   */
  virtual PerformanceStatistics
  analyze_deadline_compliance(const std::string &component_name,
                              std::chrono::nanoseconds analysis_window) = 0;

  /**
   * @brief Measure timing jitter for a component
   * @param component_name Name of the component to analyze
   * @param sample_count Number of samples to collect
   * @return Jitter analysis results
   */
  virtual PerformanceStatistics
  measure_jitter(const std::string &component_name,
                 size_t sample_count = 1000) = 0;

  /**
   * @brief Profile system latency end-to-end
   * @param start_point Starting measurement point
   * @param end_point Ending measurement point
   * @param sample_count Number of samples to collect
   * @return Latency profiling results
   */
  virtual PerformanceStatistics profile_latency(const std::string &start_point,
                                                const std::string &end_point,
                                                size_t sample_count = 100) = 0;

  /**
   * @brief Monitor resource utilization
   * @param resource_name Name of the resource to monitor
   * @param monitoring_duration Duration of monitoring
   * @return Resource utilization statistics
   */
  virtual ResourceUtilization monitor_resource_utilization(
      const std::string &resource_name,
      std::chrono::nanoseconds monitoring_duration) = 0;

  /**
   * @brief Estimate worst-case execution time
   * @param component_name Name of the component to analyze
   * @param confidence_level Statistical confidence level (e.g., 0.999)
   * @return WCET estimate with confidence bounds
   */
  virtual PerformanceStatistics
  estimate_wcet(const std::string &component_name,
                double confidence_level = 0.999) = 0;

  /**
   * @brief Verify timing constraints for all monitored components
   * @return true if all constraints are met, false otherwise
   */
  virtual bool verify_timing_constraints() = 0;

  /**
   * @brief Generate comprehensive timing analysis report
   * @param include_raw_data Whether to include raw measurement data
   * @return Complete timing analysis report
   */
  virtual TimingAnalysisReport
  generate_report(bool include_raw_data = false) = 0;

  /**
   * @brief Set custom timing verification callback
   * @param callback Custom verification function
   */
  virtual void
  set_verification_callback(TimingVerificationCallback callback) = 0;

  /**
   * @brief Set custom resource monitoring callback
   * @param callback Custom resource monitoring function
   */
  virtual void
  set_resource_monitoring_callback(ResourceMonitoringCallback callback) = 0;

  /**
   * @brief Clear all measurement data and statistics
   */
  virtual void clear_measurements() = 0;

  /**
   * @brief Get current system timestamp with high precision
   * @return High-precision timestamp
   */
  virtual std::chrono::steady_clock::time_point get_precise_timestamp() = 0;

  /**
   * @brief Enable or disable real-time priority measurement
   * @param enable Whether to use real-time priority
   * @return true if successfully configured, false otherwise
   */
  virtual bool set_realtime_priority(bool enable) = 0;

  /**
   * @brief Configure measurement sampling rate
   * @param sample_rate Samples per second for continuous monitoring
   * @return true if successfully configured, false otherwise
   */
  virtual bool configure_sampling_rate(double sample_rate) = 0;
};

/**
 * @namespace TimingUtils
 * @brief Utility functions for timing analysis
 */
namespace TimingUtils {

/**
 * @brief Convert time duration to specified unit
 * @param duration Duration to convert
 * @param unit Target time unit
 * @return Converted value as double
 */
double convert_duration(std::chrono::nanoseconds duration,
                        TimeUnit unit) noexcept;

/**
 * @brief Calculate statistical percentile from timing data
 * @param measurements Vector of timing measurements
 * @param percentile Percentile to calculate (0.0 to 1.0)
 * @return Percentile value
 */
std::chrono::nanoseconds
calculate_percentile(const std::vector<std::chrono::nanoseconds> &measurements,
                     double percentile) noexcept;

/**
 * @brief Detect statistical outliers in timing data
 * @param measurements Vector of timing measurements
 * @param threshold Z-score threshold for outlier detection
 * @return Indices of detected outliers
 */
std::vector<size_t>
detect_outliers(const std::vector<std::chrono::nanoseconds> &measurements,
                double threshold = 3.0) noexcept;

/**
 * @brief Validate timing constraint specification
 * @param constraint Timing constraint to validate
 * @return true if constraint is valid, false otherwise
 */
bool validate_timing_constraint(const TimingConstraint &constraint) noexcept;

/**
 * @brief Check if timing measurement violates safety requirements
 * @param measurement Timing measurement to check
 * @param constraint Associated timing constraint
 * @return true if safety requirements are violated, false otherwise
 */
bool is_safety_violation(const TimingMeasurement &measurement,
                         const TimingConstraint &constraint) noexcept;

/**
 * @brief Format timing duration for human-readable output
 * @param duration Duration to format
 * @param unit Preferred time unit for display
 * @return Formatted string representation
 */
std::string format_duration(std::chrono::nanoseconds duration,
                            TimeUnit unit = TimeUnit::MICROSECONDS) noexcept;

} // namespace TimingUtils

} // namespace TimingAnalysis
} // namespace IVVFramework
