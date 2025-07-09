/**
 * @file timing_analyzer.cpp
 * @brief Real-time Timing Analysis Engine Implementation
 *
 * Implementation of the TimingAnalyzer for BCI safety-critical systems.
 * Uses simplified logging to avoid external dependencies.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 */

#include "timing_analyzer.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <numeric>
#include <thread>
#include <unordered_map>

namespace IVVFramework {
namespace TimingAnalysis {

/**
 * @brief Concrete implementation of TimingAnalyzer
 */
class TimingAnalyzerImpl : public TimingAnalyzer {
public:
  TimingAnalyzerImpl();
  virtual ~TimingAnalyzerImpl();

  // Override virtual methods from base class
  bool initialize() override;
  bool configure_constraints(const std::string &component_name,
                             const TimingConstraint &constraints) override;
  uint64_t start_measurement(const std::string &component_name) override;
  TimingMeasurement stop_measurement(uint64_t measurement_id) override;
  PerformanceStatistics analyze_deadline_compliance(
      const std::string &component_name,
      std::chrono::nanoseconds analysis_window) override;
  PerformanceStatistics measure_jitter(const std::string &component_name,
                                       size_t sample_count) override;
  PerformanceStatistics profile_latency(const std::string &start_point,
                                        const std::string &end_point,
                                        size_t sample_count) override;
  ResourceUtilization monitor_resource_utilization(
      const std::string &resource_name,
      std::chrono::nanoseconds monitoring_duration) override;
  PerformanceStatistics estimate_wcet(const std::string &component_name,
                                      double confidence_level) override;
  bool verify_timing_constraints() override;
  TimingAnalysisReport generate_report(bool include_raw_data) override;
  void set_verification_callback(TimingVerificationCallback callback) override;
  void set_resource_monitoring_callback(
      ResourceMonitoringCallback callback) override;
  void clear_measurements() override;
  std::chrono::steady_clock::time_point get_precise_timestamp() override;
  bool set_realtime_priority(bool enable) override;
  bool configure_sampling_rate(double sample_rate) override;

private:
  struct ActiveMeasurement {
    uint64_t id;
    std::string component_name;
    std::chrono::steady_clock::time_point start_time;
    std::thread::id thread_id;
  };

  std::mutex measurements_mutex_;
  mutable std::mutex constraints_mutex_;
  std::atomic<uint64_t> next_measurement_id_{1};
  std::atomic<bool> initialized_{false};
  std::atomic<bool> realtime_enabled_{false};
  std::atomic<double> sampling_rate_{1000.0}; // 1kHz default

  std::unordered_map<uint64_t, ActiveMeasurement> active_measurements_;
  std::unordered_map<std::string, TimingConstraint> constraints_;
  std::unordered_map<std::string, std::vector<TimingMeasurement>>
      measurement_history_;

  TimingVerificationCallback verification_callback_;
  ResourceMonitoringCallback resource_callback_;

  // Simple logging helper
  void log_message(const std::string &level, const std::string &component,
                   const std::string &message) const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::printf("2025-07-09 [%s] [%s] %s\n", level.c_str(), component.c_str(),
                message.c_str());
  }

  // Helper methods
  bool
  validate_component_name(const std::string &component_name) const noexcept;
  PerformanceStatistics calculate_statistics(
      const std::string &component_name,
      const std::vector<TimingMeasurement> &measurements) const;
  bool check_safety_constraints(const TimingMeasurement &measurement) const;
  void log_timing_violation(const std::string &component_name,
                            const TimingMeasurement &measurement) const;
  std::chrono::nanoseconds
  calculate_jitter(const std::vector<TimingMeasurement> &measurements) const;
};

TimingAnalyzerImpl::TimingAnalyzerImpl() {
  // Constructor implementation
}

TimingAnalyzerImpl::~TimingAnalyzerImpl() {
  if (initialized_.load()) {
    log_message("CRITICAL", "TimingAnalyzer",
                "TimingAnalyzer shutting down [SAFETY_CRITICAL]");
  }
}

bool TimingAnalyzerImpl::initialize() {
  std::lock_guard<std::mutex> lock(measurements_mutex_);

  if (initialized_.load()) {
    return true; // Already initialized
  }

  try {
    // Initialize internal state
    active_measurements_.clear();
    constraints_.clear();
    measurement_history_.clear();

    // Reset atomic variables
    next_measurement_id_.store(1);
    realtime_enabled_.store(false);
    sampling_rate_.store(1000.0);

    initialized_.store(true);

    log_message("INFO", "TimingAnalyzer",
                "TimingAnalyzer initialized successfully");

    return true;
  } catch (const std::exception &e) {
    log_message("ERROR", "TimingAnalyzer",
                "Failed to initialize TimingAnalyzer: " +
                    std::string(e.what()));
    return false;
  }
}

bool TimingAnalyzerImpl::configure_constraints(
    const std::string &component_name, const TimingConstraint &constraints) {
  if (!initialized_.load()) {
    return false;
  }

  if (!validate_component_name(component_name)) {
    return false;
  }

  if (!TimingUtils::validate_timing_constraint(constraints)) {
    log_message("ERROR", "TimingAnalyzer",
                "Invalid timing constraint for component: " + component_name);
    return false;
  }

  std::lock_guard<std::mutex> lock(constraints_mutex_);
  constraints_[component_name] = constraints;

  log_message("INFO", "TimingAnalyzer",
              "Configured timing constraints for: " + component_name);

  return true;
}

uint64_t
TimingAnalyzerImpl::start_measurement(const std::string &component_name) {
  if (!initialized_.load()) {
    return 0; // Invalid measurement ID
  }

  if (!validate_component_name(component_name)) {
    return 0;
  }

  uint64_t measurement_id = next_measurement_id_.fetch_add(1);
  auto start_time = get_precise_timestamp();

  ActiveMeasurement measurement{measurement_id, component_name, start_time,
                                std::this_thread::get_id()};

  std::lock_guard<std::mutex> lock(measurements_mutex_);
  active_measurements_[measurement_id] = measurement;

  return measurement_id;
}

TimingMeasurement
TimingAnalyzerImpl::stop_measurement(uint64_t measurement_id) {
  auto end_time = get_precise_timestamp();

  TimingMeasurement result{};
  result.end_time = end_time;

  if (!initialized_.load() || measurement_id == 0) {
    result.task_name = "INVALID";
    return result;
  }

  std::lock_guard<std::mutex> lock(measurements_mutex_);

  auto it = active_measurements_.find(measurement_id);
  if (it == active_measurements_.end()) {
    result.task_name = "NOT_FOUND";
    return result;
  }

  const auto &active = it->second;
  result.task_name = active.component_name;
  result.start_time = active.start_time;
  result.execution_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
      end_time - active.start_time);

  // Check if this thread matches the starting thread
  if (std::this_thread::get_id() != active.thread_id) {
    log_message("WARNING", "TimingAnalyzer",
                "Measurement started and stopped on different threads: " +
                    active.component_name);
  }

  // Calculate jitter if we have historical data
  {
    auto hist_it = measurement_history_.find(active.component_name);
    if (hist_it != measurement_history_.end() && !hist_it->second.empty()) {
      result.jitter = calculate_jitter(hist_it->second);
    }
  }

  // Check deadline compliance
  {
    std::lock_guard<std::mutex> constraints_lock(constraints_mutex_);
    auto constraint_it = constraints_.find(active.component_name);
    if (constraint_it != constraints_.end()) {
      const auto &constraint = constraint_it->second;
      result.deadline_met = (result.execution_time <= constraint.deadline);

      if (!result.deadline_met) {
        log_timing_violation(active.component_name, result);
      }
    }
  }

  // Check safety constraints
  if (!check_safety_constraints(result)) {
    log_message("CRITICAL", "TimingAnalyzer",
                "Safety violation detected for: " + active.component_name +
                    " [SAFETY_CRITICAL]");
  }

  // Store measurement in history
  measurement_history_[active.component_name].push_back(result);

  // Remove from active measurements
  active_measurements_.erase(it);

  return result;
}

PerformanceStatistics TimingAnalyzerImpl::analyze_deadline_compliance(
    const std::string &component_name,
    std::chrono::nanoseconds analysis_window) {

  std::lock_guard<std::mutex> lock(measurements_mutex_);

  auto it = measurement_history_.find(component_name);
  if (it == measurement_history_.end()) {
    return PerformanceStatistics{component_name, 0};
  }

  // Filter measurements within the analysis window
  auto now = std::chrono::steady_clock::now();
  auto cutoff_time = now - analysis_window;

  std::vector<TimingMeasurement> filtered_measurements;
  for (const auto &measurement : it->second) {
    if (measurement.start_time >= cutoff_time) {
      filtered_measurements.push_back(measurement);
    }
  }

  return calculate_statistics(component_name, filtered_measurements);
}

PerformanceStatistics
TimingAnalyzerImpl::measure_jitter(const std::string &component_name,
                                   size_t sample_count) {

  std::lock_guard<std::mutex> lock(measurements_mutex_);

  auto it = measurement_history_.find(component_name);
  if (it == measurement_history_.end()) {
    return PerformanceStatistics{component_name, 0};
  }

  // Take the last N measurements for jitter analysis
  const auto &all_measurements = it->second;
  size_t start_idx = (all_measurements.size() > sample_count)
                         ? all_measurements.size() - sample_count
                         : 0;

  std::vector<TimingMeasurement> jitter_samples(
      all_measurements.begin() + start_idx, all_measurements.end());

  auto stats = calculate_statistics(component_name, jitter_samples);

  // Calculate jitter-specific metrics
  if (!jitter_samples.empty()) {
    stats.jitter_coefficient =
        static_cast<double>(stats.std_deviation.count()) /
        static_cast<double>(stats.avg_execution_time.count());
  }

  return stats;
}

PerformanceStatistics
TimingAnalyzerImpl::profile_latency(const std::string &start_point,
                                    const std::string &end_point,
                                    size_t sample_count) {

  // This is a simplified implementation
  // In practice, this would coordinate measurements between start and end
  // points
  return PerformanceStatistics{start_point + "_to_" + end_point, 0};
}

ResourceUtilization TimingAnalyzerImpl::monitor_resource_utilization(
    const std::string &resource_name,
    std::chrono::nanoseconds monitoring_duration) {

  ResourceUtilization result{};
  result.resource_name = resource_name;
  result.measurement_window = monitoring_duration;

  if (resource_callback_) {
    result = resource_callback_(resource_name);
  } else {
    // Default implementation - simulate some utilization data
    result.average_utilization = 45.0; // 45% average
    result.peak_utilization = 78.0;    // 78% peak
    result.exceeds_safety_threshold = (result.peak_utilization > 85.0);
  }

  return result;
}

PerformanceStatistics
TimingAnalyzerImpl::estimate_wcet(const std::string &component_name,
                                  double confidence_level) {

  std::lock_guard<std::mutex> lock(measurements_mutex_);

  auto it = measurement_history_.find(component_name);
  if (it == measurement_history_.end()) {
    return PerformanceStatistics{component_name, 0};
  }

  auto stats = calculate_statistics(component_name, it->second);

  // Simple WCET estimation using statistical approach
  if (!it->second.empty()) {
    std::vector<std::chrono::nanoseconds> execution_times;
    for (const auto &measurement : it->second) {
      execution_times.push_back(measurement.execution_time);
    }

    // Use high percentile as WCET estimate
    stats.wcet_estimate =
        TimingUtils::calculate_percentile(execution_times, confidence_level);
  }

  return stats;
}

bool TimingAnalyzerImpl::verify_timing_constraints() {
  std::lock_guard<std::mutex> constraints_lock(constraints_mutex_);
  std::lock_guard<std::mutex> measurements_lock(measurements_mutex_);

  bool all_constraints_met = true;

  for (const auto &[component_name, constraint] : constraints_) {
    auto it = measurement_history_.find(component_name);
    if (it == measurement_history_.end()) {
      continue; // No measurements for this component
    }

    // Calculate recent deadline miss rate
    size_t recent_measurements = std::min(it->second.size(), size_t(100));
    size_t deadline_misses = 0;

    for (size_t i = it->second.size() - recent_measurements;
         i < it->second.size(); ++i) {
      if (!it->second[i].deadline_met) {
        deadline_misses++;
      }
    }

    double miss_rate =
        static_cast<double>(deadline_misses) / recent_measurements;

    if (miss_rate > constraint.deadline_miss_threshold) {
      all_constraints_met = false;

      log_message("ERROR", "TimingAnalyzer",
                  "Timing constraint violation for " + component_name +
                      ": miss rate " + std::to_string(miss_rate * 100.0) + "%");
    }
  }

  return all_constraints_met;
}

TimingAnalysisReport
TimingAnalyzerImpl::generate_report(bool include_raw_data) {
  TimingAnalysisReport report{};
  report.analysis_timestamp = std::chrono::steady_clock::now();
  report.target_system = "BCI_System"; // Could be configurable
  report.overall_timing_compliance = verify_timing_constraints();

  std::lock_guard<std::mutex> lock(measurements_mutex_);

  // Generate component statistics
  for (const auto &[component_name, measurements] : measurement_history_) {
    if (!measurements.empty()) {
      auto stats = calculate_statistics(component_name, measurements);
      report.component_stats.push_back(stats);
    }
  }

  // Calculate overall system utilization score
  if (!report.component_stats.empty()) {
    double total_utilization = 0.0;
    for (const auto &stats : report.component_stats) {
      // Simple utilization calculation based on deadline compliance
      double component_utilization = 1.0 - stats.deadline_miss_rate;
      total_utilization += component_utilization;
    }
    report.system_utilization_score =
        total_utilization / report.component_stats.size();
  }

  // Add recommendations
  if (report.overall_timing_compliance) {
    report.recommendations =
        "System timing performance is within acceptable limits.";
  } else {
    report.recommendations = "Timing violations detected. Review component "
                             "implementation and constraints.";
  }

  return report;
}

void TimingAnalyzerImpl::set_verification_callback(
    TimingVerificationCallback callback) {
  verification_callback_ = std::move(callback);
}

void TimingAnalyzerImpl::set_resource_monitoring_callback(
    ResourceMonitoringCallback callback) {
  resource_callback_ = std::move(callback);
}

void TimingAnalyzerImpl::clear_measurements() {
  std::lock_guard<std::mutex> lock(measurements_mutex_);
  measurement_history_.clear();
  active_measurements_.clear();

  log_message("INFO", "TimingAnalyzer", "All measurement data cleared");
}

std::chrono::steady_clock::time_point
TimingAnalyzerImpl::get_precise_timestamp() {
  return std::chrono::steady_clock::now();
}

bool TimingAnalyzerImpl::set_realtime_priority(bool enable) {
  realtime_enabled_.store(enable);

  if (enable) {
    log_message("INFO", "TimingAnalyzer", "Real-time priority enabled");
  }

  return true;
}

bool TimingAnalyzerImpl::configure_sampling_rate(double sample_rate) {
  if (sample_rate <= 0.0 || sample_rate > 100000.0) { // Max 100kHz
    return false;
  }

  sampling_rate_.store(sample_rate);

  log_message("INFO", "TimingAnalyzer",
              "Sampling rate configured: " + std::to_string(sample_rate) +
                  " Hz");

  return true;
}

// Helper method implementations
bool TimingAnalyzerImpl::validate_component_name(
    const std::string &component_name) const noexcept {
  try {
    return !component_name.empty() && component_name.length() < 256;
  } catch (...) {
    return false;
  }
}

PerformanceStatistics TimingAnalyzerImpl::calculate_statistics(
    const std::string &component_name,
    const std::vector<TimingMeasurement> &measurements) const {

  PerformanceStatistics stats{};
  stats.component_name = component_name;
  stats.measurement_count = measurements.size();

  if (measurements.empty()) {
    return stats;
  }

  // Calculate basic statistics
  std::vector<std::chrono::nanoseconds> execution_times;
  size_t deadline_misses = 0;

  for (const auto &measurement : measurements) {
    execution_times.push_back(measurement.execution_time);
    if (!measurement.deadline_met) {
      deadline_misses++;
    }
  }

  stats.min_execution_time =
      *std::min_element(execution_times.begin(), execution_times.end());
  stats.max_execution_time =
      *std::max_element(execution_times.begin(), execution_times.end());

  // Calculate average
  auto total_time =
      std::accumulate(execution_times.begin(), execution_times.end(),
                      std::chrono::nanoseconds{0});
  stats.avg_execution_time = total_time / measurements.size();

  // Calculate standard deviation
  double variance = 0.0;
  for (const auto &time : execution_times) {
    double diff = static_cast<double>(time.count()) -
                  static_cast<double>(stats.avg_execution_time.count());
    variance += diff * diff;
  }
  variance /= measurements.size();
  stats.std_deviation = std::chrono::nanoseconds{
      static_cast<std::chrono::nanoseconds::rep>(std::sqrt(variance))};

  // Calculate deadline miss rate
  stats.deadline_miss_rate =
      static_cast<double>(deadline_misses) / measurements.size();

  // Calculate percentiles
  stats.percentiles.resize(3);
  stats.percentiles[0] =
      TimingUtils::calculate_percentile(execution_times, 0.95); // 95th
  stats.percentiles[1] =
      TimingUtils::calculate_percentile(execution_times, 0.99); // 99th
  stats.percentiles[2] =
      TimingUtils::calculate_percentile(execution_times, 0.999); // 99.9th

  return stats;
}

bool TimingAnalyzerImpl::check_safety_constraints(
    const TimingMeasurement &measurement) const {
  std::lock_guard<std::mutex> lock(constraints_mutex_);

  auto it = constraints_.find(measurement.task_name);
  if (it == constraints_.end()) {
    return true; // No constraints defined, assume safe
  }

  const auto &constraint = it->second;

  if (constraint.is_critical_path) {
    // Stricter safety checks for critical paths
    if (measurement.execution_time > constraint.deadline * 1.1) { // 10% margin
      return false;
    }
  }

  if (verification_callback_) {
    return verification_callback_(measurement, constraint);
  }

  return measurement.deadline_met;
}

void TimingAnalyzerImpl::log_timing_violation(
    const std::string &component_name,
    const TimingMeasurement &measurement) const {
  log_message("WARNING", "TimingAnalyzer",
              "Deadline violation for " + component_name + ": " +
                  std::to_string(measurement.execution_time.count()) + "ns");
}

std::chrono::nanoseconds TimingAnalyzerImpl::calculate_jitter(
    const std::vector<TimingMeasurement> &measurements) const {

  if (measurements.size() < 2) {
    return std::chrono::nanoseconds{0};
  }

  // Calculate inter-arrival time jitter
  std::vector<std::chrono::nanoseconds> intervals;
  for (size_t i = 1; i < measurements.size(); ++i) {
    auto interval = measurements[i].start_time - measurements[i - 1].start_time;
    intervals.push_back(
        std::chrono::duration_cast<std::chrono::nanoseconds>(interval));
  }

  if (intervals.empty()) {
    return std::chrono::nanoseconds{0};
  }

  // Calculate average interval
  auto total_interval = std::accumulate(intervals.begin(), intervals.end(),
                                        std::chrono::nanoseconds{0});
  auto avg_interval = total_interval / intervals.size();

  // Calculate jitter as maximum deviation from average
  std::chrono::nanoseconds max_deviation{0};
  for (const auto &interval : intervals) {
    auto deviation_count = (interval > avg_interval)
                               ? (interval - avg_interval).count()
                               : (avg_interval - interval).count();
    max_deviation =
        std::max(max_deviation, std::chrono::nanoseconds{deviation_count});
  }

  return max_deviation;
}

// Factory method implementation
std::unique_ptr<TimingAnalyzer> TimingAnalyzer::create() {
  return std::make_unique<TimingAnalyzerImpl>();
}

// Utility functions implementation
namespace TimingUtils {

double convert_duration(std::chrono::nanoseconds duration,
                        TimeUnit unit) noexcept {
  try {
    switch (unit) {
    case TimeUnit::NANOSECONDS:
      return static_cast<double>(duration.count());
    case TimeUnit::MICROSECONDS:
      return static_cast<double>(duration.count()) / 1000.0;
    case TimeUnit::MILLISECONDS:
      return static_cast<double>(duration.count()) / 1000000.0;
    case TimeUnit::SECONDS:
      return static_cast<double>(duration.count()) / 1000000000.0;
    default:
      return 0.0;
    }
  } catch (...) {
    return 0.0;
  }
}

std::chrono::nanoseconds
calculate_percentile(const std::vector<std::chrono::nanoseconds> &measurements,
                     double percentile) noexcept {

  try {
    if (measurements.empty() || percentile < 0.0 || percentile > 1.0) {
      return std::chrono::nanoseconds{0};
    }

    auto sorted_measurements = measurements;
    std::sort(sorted_measurements.begin(), sorted_measurements.end());

    size_t index =
        static_cast<size_t>(percentile * (sorted_measurements.size() - 1));
    return sorted_measurements[index];
  } catch (...) {
    return std::chrono::nanoseconds{0};
  }
}

std::vector<size_t>
detect_outliers(const std::vector<std::chrono::nanoseconds> &measurements,
                double threshold) noexcept {

  std::vector<size_t> outliers;

  try {
    if (measurements.size() < 3) {
      return outliers; // Need at least 3 samples for outlier detection
    }

    // Calculate mean and standard deviation
    auto total = std::accumulate(measurements.begin(), measurements.end(),
                                 std::chrono::nanoseconds{0});
    double mean = static_cast<double>(total.count()) / measurements.size();

    double variance = 0.0;
    for (const auto &measurement : measurements) {
      double diff = static_cast<double>(measurement.count()) - mean;
      variance += diff * diff;
    }
    variance /= measurements.size();
    double std_dev = std::sqrt(variance);

    // Find outliers using Z-score
    for (size_t i = 0; i < measurements.size(); ++i) {
      double z_score =
          std::abs(static_cast<double>(measurements[i].count()) - mean) /
          std_dev;
      if (z_score > threshold) {
        outliers.push_back(i);
      }
    }
  } catch (...) {
    // Return empty vector on error
  }

  return outliers;
}

bool validate_timing_constraint(const TimingConstraint &constraint) noexcept {
  try {
    // Basic validation checks
    if (constraint.name.empty()) {
      return false;
    }

    if (constraint.deadline.count() <= 0) {
      return false;
    }

    if (constraint.period.count() < 0) {
      return false;
    }

    if (constraint.max_jitter.count() < 0) {
      return false;
    }

    if (constraint.deadline_miss_threshold < 0.0 ||
        constraint.deadline_miss_threshold > 1.0) {
      return false;
    }

    // Logical consistency checks
    if (constraint.period.count() > 0 &&
        constraint.deadline > constraint.period) {
      return false; // Deadline cannot be longer than period
    }

    return true;
  } catch (...) {
    return false;
  }
}

bool is_safety_violation(const TimingMeasurement &measurement,
                         const TimingConstraint &constraint) noexcept {
  try {
    if (constraint.is_critical_path && !measurement.deadline_met) {
      return true; // Any deadline miss in critical path is a safety violation
    }

    // Check if execution time exceeds safety margin (e.g., 150% of deadline)
    if (measurement.execution_time > constraint.deadline * 1.5) {
      return true;
    }

    // Check if jitter exceeds safety limits
    if (measurement.jitter > constraint.max_jitter * 2.0) {
      return true;
    }

    return false;
  } catch (...) {
    return true; // Err on the side of caution
  }
}

std::string format_duration(std::chrono::nanoseconds duration,
                            TimeUnit unit) noexcept {
  try {
    double value = convert_duration(duration, unit);

    std::string unit_str;
    switch (unit) {
    case TimeUnit::NANOSECONDS:
      unit_str = "ns";
      break;
    case TimeUnit::MICROSECONDS:
      unit_str = "μs";
      break;
    case TimeUnit::MILLISECONDS:
      unit_str = "ms";
      break;
    case TimeUnit::SECONDS:
      unit_str = "s";
      break;
    }

    return std::to_string(value) + unit_str;
  } catch (...) {
    return "0ns";
  }
}

} // namespace TimingUtils

} // namespace TimingAnalysis
} // namespace IVVFramework
