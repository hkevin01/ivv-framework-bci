/**
 * @file fault_injector.cpp
 * @brief Safety-Critical Fault Injection Engine Implementation
 *
 * Implementation of the fault injection capabilities for the IV&V Framework.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 */

#include "fault_injector.h"
#include "../core/logger.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace IVVFramework {
namespace FaultInjection {

/**
 * @brief Concrete implementation of FaultInjector
 */
class FaultInjectorImpl : public FaultInjector {
public:
  FaultInjectorImpl();
  virtual ~FaultInjectorImpl();

  // Override virtual methods from base class
  bool initialize() override;
  bool configure_target(const std::string &target_name,
                        const FaultTarget &target) override;
  FaultInjectionResult
  inject_timing_fault(const FaultInjectionConfig &config) override;
  FaultInjectionResult
  inject_data_corruption(const FaultInjectionConfig &config) override;
  FaultInjectionResult
  inject_communication_fault(const FaultInjectionConfig &config) override;
  FaultInjectionResult
  inject_hardware_failure(const FaultInjectionConfig &config) override;
  bool start_fault_campaign(
      const std::vector<FaultInjectionConfig> &configs) override;
  bool stop_fault_campaign() override;
  void
  register_propagation_callback(FaultPropagationCallback callback) override;
  void register_safety_callback(SafetyCheckCallback callback) override;
  std::vector<FaultInjectionResult> get_statistics() const override;
  bool is_campaign_active() const noexcept override;
  bool emergency_stop() noexcept override;

private:
  // Core state
  std::shared_ptr<Core::Logger> logger_;
  std::atomic<bool> initialized_{false};
  std::atomic<bool> campaign_active_{false};
  std::atomic<bool> emergency_stopped_{false};

  // Targets
  std::unordered_map<std::string, FaultTarget> configured_targets_;
  mutable std::mutex targets_mutex_;

  // Statistics
  mutable std::vector<FaultInjectionResult> injection_results_;
  mutable std::mutex results_mutex_;

  // Campaign management
  std::vector<FaultInjectionConfig> campaign_configs_;
  std::unique_ptr<std::thread> campaign_thread_;
  std::atomic<bool> should_stop_campaign_{false};
  std::condition_variable campaign_cv_;
  std::mutex campaign_mutex_;

  // Callbacks
  std::vector<FaultPropagationCallback> propagation_callbacks_;
  std::vector<SafetyCheckCallback> safety_callbacks_;
  mutable std::mutex callbacks_mutex_;

  // Random number generation
  std::random_device rd_;
  std::mt19937 rng_;

  // Helper methods
  void campaign_execution_loop();
  FaultInjectionResult
  execute_fault_injection(const FaultInjectionConfig &config);
  FaultInjectionResult execute_timing_fault(const FaultInjectionConfig &config);
  FaultInjectionResult
  execute_data_corruption(const FaultInjectionConfig &config);
  FaultInjectionResult
  execute_communication_fault(const FaultInjectionConfig &config);
  FaultInjectionResult
  execute_hardware_failure(const FaultInjectionConfig &config);
  FaultInjectionResult
  execute_resource_exhaustion(const FaultInjectionConfig &config);
  FaultInjectionResult
  execute_power_failure(const FaultInjectionConfig &config);
  bool perform_safety_checks(const FaultInjectionConfig &config) const;
  void notify_propagation_callbacks(const FaultInjectionResult &result);
  void add_result(const FaultInjectionResult &result);
  FaultInjectionResult
  create_error_result(FaultInjectionResult::Status status,
                      const std::string &description) const;
};

FaultInjectorImpl::FaultInjectorImpl() : rng_(rd_()) {
  // Initialize logger with default configuration
  logger_ = std::make_shared<Core::Logger>();
  Core::LogConfig config;
  config.min_level = Core::LogLevel::INFO;
  config.destinations.push_back(Core::LogDestination::CONSOLE);
  logger_->initialize("FaultInjector", config);
}

FaultInjectorImpl::~FaultInjectorImpl() { emergency_stop(); }

bool FaultInjectorImpl::initialize() {
  if (initialized_.load()) {
    return true;
  }

  logger_->log_info("Initializing FaultInjector");

  // Perform initialization tasks
  initialized_.store(true);

  logger_->log_info("FaultInjector initialized successfully");
  return true;
}

bool FaultInjectorImpl::configure_target(const std::string &target_name,
                                         const FaultTarget &target) {
  if (!initialized_.load()) {
    logger_->log_error("FaultInjector not initialized");
    return false;
  }

  if (target_name.empty()) {
    logger_->log_error("Target name cannot be empty");
    return false;
  }

  {
    std::lock_guard<std::mutex> lock(targets_mutex_);
    configured_targets_[target_name] = target;
  }

  logger_->log_info("Configured target: " + target_name);
  return true;
}

FaultInjectionResult
FaultInjectorImpl::inject_timing_fault(const FaultInjectionConfig &config) {
  if (!initialized_.load()) {
    return create_error_result(FaultInjectionResult::Status::FAILED,
                               "FaultInjector not initialized");
  }

  if (emergency_stopped_.load()) {
    return create_error_result(FaultInjectionResult::Status::BLOCKED_BY_SAFETY,
                               "Emergency stop active");
  }

  if (!perform_safety_checks(config)) {
    return create_error_result(FaultInjectionResult::Status::BLOCKED_BY_SAFETY,
                               "Safety check failed");
  }

  return execute_fault_injection(config);
}

FaultInjectionResult
FaultInjectorImpl::inject_data_corruption(const FaultInjectionConfig &config) {
  if (!initialized_.load()) {
    return create_error_result(FaultInjectionResult::Status::FAILED,
                               "FaultInjector not initialized");
  }

  if (emergency_stopped_.load()) {
    return create_error_result(FaultInjectionResult::Status::BLOCKED_BY_SAFETY,
                               "Emergency stop active");
  }

  if (!perform_safety_checks(config)) {
    return create_error_result(FaultInjectionResult::Status::BLOCKED_BY_SAFETY,
                               "Safety check failed");
  }

  return execute_fault_injection(config);
}

FaultInjectionResult FaultInjectorImpl::inject_communication_fault(
    const FaultInjectionConfig &config) {
  if (!initialized_.load()) {
    return create_error_result(FaultInjectionResult::Status::FAILED,
                               "FaultInjector not initialized");
  }

  if (emergency_stopped_.load()) {
    return create_error_result(FaultInjectionResult::Status::BLOCKED_BY_SAFETY,
                               "Emergency stop active");
  }

  if (!perform_safety_checks(config)) {
    return create_error_result(FaultInjectionResult::Status::BLOCKED_BY_SAFETY,
                               "Safety check failed");
  }

  return execute_fault_injection(config);
}

FaultInjectionResult
FaultInjectorImpl::inject_hardware_failure(const FaultInjectionConfig &config) {
  if (!initialized_.load()) {
    return create_error_result(FaultInjectionResult::Status::FAILED,
                               "FaultInjector not initialized");
  }

  if (emergency_stopped_.load()) {
    return create_error_result(FaultInjectionResult::Status::BLOCKED_BY_SAFETY,
                               "Emergency stop active");
  }

  if (!perform_safety_checks(config)) {
    return create_error_result(FaultInjectionResult::Status::BLOCKED_BY_SAFETY,
                               "Safety check failed");
  }

  return execute_fault_injection(config);
}

bool FaultInjectorImpl::start_fault_campaign(
    const std::vector<FaultInjectionConfig> &configs) {
  if (!initialized_.load()) {
    logger_->log_error("FaultInjector not initialized");
    return false;
  }

  if (configs.empty()) {
    logger_->log_error("Campaign configurations cannot be empty");
    return false;
  }

  if (campaign_active_.load()) {
    logger_->log_warning("Campaign already active, stopping previous campaign");
    stop_fault_campaign();
  }

  {
    std::lock_guard<std::mutex> lock(campaign_mutex_);
    campaign_configs_ = configs;
    should_stop_campaign_.store(false);
  }

  // Start campaign thread
  campaign_thread_ = std::make_unique<std::thread>(
      &FaultInjectorImpl::campaign_execution_loop, this);
  campaign_active_.store(true);

  logger_->log_info("Fault injection campaign started with " +
                    std::to_string(configs.size()) + " configurations");
  return true;
}

bool FaultInjectorImpl::stop_fault_campaign() {
  if (!campaign_active_.load()) {
    return true;
  }

  should_stop_campaign_.store(true);
  campaign_cv_.notify_all();

  if (campaign_thread_ && campaign_thread_->joinable()) {
    campaign_thread_->join();
  }

  campaign_active_.store(false);
  logger_->log_info("Fault injection campaign stopped");
  return true;
}

void FaultInjectorImpl::register_propagation_callback(
    FaultPropagationCallback callback) {
  if (callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    propagation_callbacks_.push_back(callback);
  }
}

void FaultInjectorImpl::register_safety_callback(SafetyCheckCallback callback) {
  if (callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    safety_callbacks_.push_back(callback);
  }
}

std::vector<FaultInjectionResult> FaultInjectorImpl::get_statistics() const {
  std::lock_guard<std::mutex> lock(results_mutex_);
  return injection_results_;
}

bool FaultInjectorImpl::is_campaign_active() const noexcept {
  return campaign_active_.load();
}

bool FaultInjectorImpl::emergency_stop() noexcept {
  try {
    emergency_stopped_.store(true);

    // Stop campaign
    should_stop_campaign_.store(true);
    campaign_cv_.notify_all();

    if (campaign_thread_ && campaign_thread_->joinable()) {
      campaign_thread_->join();
    }

    campaign_active_.store(false);

    if (logger_) {
      logger_->log_critical("Emergency stop activated",
                            "FAULT_INJECTION_EMERGENCY");
    }

    return true;
  } catch (...) {
    return false;
  }
}

void FaultInjectorImpl::campaign_execution_loop() {
  logger_->log_info("Campaign execution loop started");

  for (const auto &config : campaign_configs_) {
    if (should_stop_campaign_.load() || emergency_stopped_.load()) {
      break;
    }

    // Execute fault injection
    auto result = execute_fault_injection(config);
    add_result(result);

    // Notify callbacks
    notify_propagation_callbacks(result);

    // Wait for next injection if period is specified
    if (config.injection_period.count() > 0) {
      std::unique_lock<std::mutex> lock(campaign_mutex_);
      campaign_cv_.wait_for(lock, config.injection_period, [this] {
        return should_stop_campaign_.load() || emergency_stopped_.load();
      });
    }
  }

  logger_->log_info("Campaign execution loop completed");
}

FaultInjectionResult
FaultInjectorImpl::execute_fault_injection(const FaultInjectionConfig &config) {
  FaultInjectionResult result;
  result.injection_time = std::chrono::steady_clock::now();
  result.status = FaultInjectionResult::Status::SUCCESS;
  result.description = "Fault injection executed successfully";

  try {
    // Check if target exists
    {
      std::lock_guard<std::mutex> lock(targets_mutex_);
      if (configured_targets_.find(config.target.component_name) ==
          configured_targets_.end()) {
        result.status = FaultInjectionResult::Status::TARGET_NOT_FOUND;
        result.description =
            "Target component not found: " + config.target.component_name;
        return result;
      }
    }

    // Apply injection delay
    if (config.injection_delay.count() > 0) {
      std::this_thread::sleep_for(config.injection_delay);
    }

    // Execute fault based on type
    switch (config.fault_type) {
    case FaultType::TIMING_FAULT:
      result = execute_timing_fault(config);
      break;
    case FaultType::DATA_CORRUPTION:
      result = execute_data_corruption(config);
      break;
    case FaultType::COMMUNICATION:
      result = execute_communication_fault(config);
      break;
    case FaultType::HARDWARE_FAILURE:
      result = execute_hardware_failure(config);
      break;
    case FaultType::RESOURCE_EXHAUSTION:
      result = execute_resource_exhaustion(config);
      break;
    case FaultType::POWER_FAILURE:
      result = execute_power_failure(config);
      break;
    default:
      result.status = FaultInjectionResult::Status::FAILED;
      result.description = "Unsupported fault type";
    }

    result.recovery_time = std::chrono::steady_clock::now();

    // Calculate system impact
    result.system_impact_score =
        FaultInjectionUtils::calculate_impact_score(result);

    logger_->log_info(
        "Fault injection completed: " +
        FaultInjectionUtils::fault_type_to_string(config.fault_type) + " on " +
        config.target.component_name);

  } catch (const std::exception &e) {
    result.status = FaultInjectionResult::Status::FAILED;
    result.description =
        "Exception during fault injection: " + std::string(e.what());
    logger_->log_error("Fault injection failed: " + result.description);
  }

  return result;
}

FaultInjectionResult
FaultInjectorImpl::execute_timing_fault(const FaultInjectionConfig &config) {
  FaultInjectionResult result;
  result.status = FaultInjectionResult::Status::SUCCESS;
  result.description = "Timing fault executed";

  // Simulate timing fault by introducing delay
  auto delay = config.timing_config.delay_injection;
  if (delay.count() > 0) {
    std::this_thread::sleep_for(delay);
    result.observed_effects.push_back(
        "Timing delay of " + std::to_string(delay.count()) + " microseconds");
  }

  // Simulate jitter
  if (config.timing_config.jitter_amplitude.count() > 0) {
    std::uniform_int_distribution<int> dist(
        -static_cast<int>(config.timing_config.jitter_amplitude.count()),
        static_cast<int>(config.timing_config.jitter_amplitude.count()));
    auto jitter = std::chrono::microseconds(dist(rng_));
    if (jitter.count() > 0) {
      std::this_thread::sleep_for(jitter);
    }
    result.observed_effects.push_back("Timing jitter applied");
  }

  return result;
}

FaultInjectionResult
FaultInjectorImpl::execute_data_corruption(const FaultInjectionConfig &config) {
  FaultInjectionResult result;
  result.status = FaultInjectionResult::Status::SUCCESS;
  result.description = "Data corruption fault executed";

  // For simulation purposes, we'll just log the operation
  // In a real implementation, this would corrupt data at the target location
  switch (config.data_config.type) {
  case DataCorruptionConfig::CorruptionType::BIT_FLIP:
    result.observed_effects.push_back("Bit flip simulation in " +
                                      config.target.component_name);
    break;
  case DataCorruptionConfig::CorruptionType::VALUE_RANGE:
    result.observed_effects.push_back("Value range violation in " +
                                      config.target.component_name);
    break;
  case DataCorruptionConfig::CorruptionType::PATTERN_CORRUPTION:
    result.observed_effects.push_back("Pattern corruption in " +
                                      config.target.component_name);
    break;
  case DataCorruptionConfig::CorruptionType::CHECKSUM_VIOLATION:
    result.observed_effects.push_back("Checksum violation in " +
                                      config.target.component_name);
    break;
  }

  return result;
}

FaultInjectionResult FaultInjectorImpl::execute_communication_fault(
    const FaultInjectionConfig &config) {
  FaultInjectionResult result;
  result.status = FaultInjectionResult::Status::SUCCESS;
  result.description = "Communication fault executed";

  // Simulate communication fault
  switch (config.comm_config.type) {
  case CommunicationFaultConfig::CommFaultType::PACKET_LOSS:
    result.observed_effects.push_back("Packet loss simulation");
    break;
  case CommunicationFaultConfig::CommFaultType::PACKET_DELAY:
    std::this_thread::sleep_for(config.comm_config.delay_range);
    result.observed_effects.push_back("Packet delay simulation");
    break;
  case CommunicationFaultConfig::CommFaultType::PACKET_CORRUPTION:
    result.observed_effects.push_back("Packet corruption simulation");
    break;
  case CommunicationFaultConfig::CommFaultType::DUPLICATE_PACKETS:
    result.observed_effects.push_back("Duplicate packet simulation");
    break;
  case CommunicationFaultConfig::CommFaultType::REORDER_PACKETS:
    result.observed_effects.push_back("Packet reordering simulation");
    break;
  }

  return result;
}

FaultInjectionResult FaultInjectorImpl::execute_hardware_failure(
    const FaultInjectionConfig &config) {
  FaultInjectionResult result;
  result.status = FaultInjectionResult::Status::SUCCESS;
  result.description = "Hardware failure simulation executed";

  result.observed_effects.push_back("Hardware failure simulation in " +
                                    config.target.component_name);

  // If this is a critical component, mark as safety violation
  if (config.target.is_critical_path) {
    result.safety_violations.push_back("Critical hardware component failure");
  }

  return result;
}

FaultInjectionResult FaultInjectorImpl::execute_resource_exhaustion(
    const FaultInjectionConfig &config) {
  FaultInjectionResult result;
  result.status = FaultInjectionResult::Status::SUCCESS;
  result.description = "Resource exhaustion simulation executed";

  result.observed_effects.push_back("Resource exhaustion simulation in " +
                                    config.target.component_name);
  return result;
}

FaultInjectionResult
FaultInjectorImpl::execute_power_failure(const FaultInjectionConfig &config) {
  FaultInjectionResult result;
  result.status = FaultInjectionResult::Status::SUCCESS;
  result.description = "Power failure simulation executed";

  result.observed_effects.push_back("Power failure simulation");
  result.safety_violations.push_back("Power supply interruption");

  // Use config to avoid unused parameter warning
  if (!config.target.component_name.empty()) {
    result.affected_components.push_back(config.target.component_name);
  }

  return result;
}

bool FaultInjectorImpl::perform_safety_checks(
    const FaultInjectionConfig &config) const {
  // Built-in safety checks
  if (config.respect_safety_constraints) {
    // Check if target is in excluded functions
    for (const auto &excluded_func : config.excluded_critical_functions) {
      if (config.target.function_name == excluded_func) {
        logger_->log_warning(
            "Fault injection blocked: target function is excluded");
        return false;
      }
    }

    // Check maximum system impact
    if (config.max_system_impact >
        0.5) { // Don't allow more than 50% system impact
      logger_->log_warning("Fault injection blocked: system impact too high");
      return false;
    }
  }

  // Call registered safety callbacks
  {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    for (const auto &callback : safety_callbacks_) {
      try {
        if (!callback(config)) {
          logger_->log_warning("Fault injection blocked by safety callback");
          return false;
        }
      } catch (...) {
        logger_->log_error(
            "Safety callback threw exception, blocking injection");
        return false;
      }
    }
  }

  return true;
}

void FaultInjectorImpl::notify_propagation_callbacks(
    const FaultInjectionResult &result) {
  std::lock_guard<std::mutex> lock(callbacks_mutex_);
  for (const auto &callback : propagation_callbacks_) {
    try {
      callback(result);
    } catch (...) {
      // Callbacks must not throw
    }
  }
}

void FaultInjectorImpl::add_result(const FaultInjectionResult &result) {
  std::lock_guard<std::mutex> lock(results_mutex_);
  injection_results_.push_back(result);
}

FaultInjectionResult
FaultInjectorImpl::create_error_result(FaultInjectionResult::Status status,
                                       const std::string &description) const {
  FaultInjectionResult result;
  result.status = status;
  result.description = description;
  result.injection_time = std::chrono::steady_clock::now();
  result.recovery_time = result.injection_time;
  return result;
}

// Factory method implementation
std::unique_ptr<FaultInjector> FaultInjector::create() {
  return std::make_unique<FaultInjectorImpl>();
}

// Utility functions implementation
namespace FaultInjectionUtils {

bool validate_fault_config(const FaultInjectionConfig &config) noexcept {
  try {
    // Basic validation
    if (config.target.component_name.empty()) {
      return false;
    }

    if (config.max_injections == 0) {
      return false;
    }

    if (config.max_system_impact < 0.0 || config.max_system_impact > 1.0) {
      return false;
    }

    return true;
  } catch (...) {
    return false;
  }
}

double calculate_impact_score(const FaultInjectionResult &result) noexcept {
  try {
    double score = 0.0;

    // Base score based on status
    switch (result.status) {
    case FaultInjectionResult::Status::SUCCESS:
      score += 0.1;
      break;
    case FaultInjectionResult::Status::FAILED:
      score += 0.3;
      break;
    case FaultInjectionResult::Status::TIMEOUT:
      score += 0.5;
      break;
    default:
      score += 0.2;
    }

    // Add score for observed effects
    score += static_cast<double>(result.observed_effects.size()) * 0.1;

    // Add significant score for safety violations
    score += static_cast<double>(result.safety_violations.size()) * 0.3;

    // Clamp to [0.0, 1.0]
    return std::min(1.0, std::max(0.0, score));
  } catch (...) {
    return 0.0;
  }
}

std::string fault_type_to_string(FaultType type) noexcept {
  switch (type) {
  case FaultType::TIMING_FAULT:
    return "TIMING_FAULT";
  case FaultType::DATA_CORRUPTION:
    return "DATA_CORRUPTION";
  case FaultType::COMMUNICATION:
    return "COMMUNICATION";
  case FaultType::HARDWARE_FAILURE:
    return "HARDWARE_FAILURE";
  case FaultType::RESOURCE_EXHAUSTION:
    return "RESOURCE_EXHAUSTION";
  case FaultType::POWER_FAILURE:
    return "POWER_FAILURE";
  default:
    return "UNKNOWN";
  }
}

bool is_safety_critical_target(const FaultTarget &target) noexcept {
  try {
    return target.is_critical_path;
  } catch (...) {
    return true; // Err on the side of caution
  }
}

} // namespace FaultInjectionUtils

} // namespace FaultInjection
} // namespace IVVFramework
