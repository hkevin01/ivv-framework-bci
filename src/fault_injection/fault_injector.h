/**
 * @file fault_injector.h
 * @brief Fault Injection Engine for BCI Safety-Critical Systems
 *
 * This file defines the fault injection capabilities for systematic
 * fault insertion and propagation analysis in BCI systems.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 *
 * @copyright Copyright (c) 2025 IV&V Framework for BCI Systems
 *
 * Safety-Critical Notice:
 * Fault injection must be performed in controlled environments only.
 * Never inject faults into production BCI systems connected to patients.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace IVVFramework {
namespace FaultInjection {

/**
 * @brief Types of faults that can be injected
 */
enum class FaultType {
  TIMING_FAULT = 0,    ///< Timing-related faults (deadline violations, jitter)
  DATA_CORRUPTION = 1, ///< Data corruption faults (bit-flips, range violations)
  COMMUNICATION = 2,   ///< Communication faults (packet loss, corruption)
  HARDWARE_FAILURE = 3,    ///< Hardware failure simulation
  RESOURCE_EXHAUSTION = 4, ///< Resource exhaustion (memory, CPU)
  POWER_FAILURE = 5        ///< Power-related faults
};

/**
 * @brief Fault injection timing modes
 */
enum class InjectionTiming {
  IMMEDIATE = 0,  ///< Inject fault immediately
  DELAYED = 1,    ///< Inject fault after specified delay
  PERIODIC = 2,   ///< Inject fault periodically
  CONDITIONAL = 3 ///< Inject fault when condition is met
};

/**
 * @brief Fault injection target specification
 */
struct FaultTarget {
  std::string component_name; ///< Name of target component
  std::string function_name;  ///< Specific function to target (optional)
  std::vector<std::string> parameters; ///< Parameters to affect (optional)
  uint32_t address_range_start = 0;    ///< Memory address range start
  uint32_t address_range_end = 0;      ///< Memory address range end
  bool is_critical_path = false;       ///< Whether target is in critical path
};

/**
 * @brief Timing fault configuration
 */
struct TimingFaultConfig {
  std::chrono::microseconds delay_injection{0};  ///< Additional delay to inject
  std::chrono::microseconds jitter_amplitude{0}; ///< Jitter amplitude
  double deadline_violation_factor = 1.0; ///< Factor to multiply deadline by
  bool cause_timeout = false;             ///< Whether to cause timeout
};

/**
 * @brief Data corruption fault configuration
 */
struct DataCorruptionConfig {
  enum class CorruptionType {
    BIT_FLIP,           ///< Single or multiple bit flips
    VALUE_RANGE,        ///< Value outside expected range
    PATTERN_CORRUPTION, ///< Specific pattern corruption
    CHECKSUM_VIOLATION  ///< Checksum mismatch
  };

  CorruptionType type = CorruptionType::BIT_FLIP;
  std::vector<uint8_t> bit_positions;      ///< Positions of bits to flip
  double corruption_probability = 0.01;    ///< Probability of corruption
  std::vector<uint8_t> corruption_pattern; ///< Pattern for corruption
};

/**
 * @brief Communication fault configuration
 */
struct CommunicationFaultConfig {
  enum class CommFaultType {
    PACKET_LOSS,       ///< Drop packets
    PACKET_DELAY,      ///< Delay packet delivery
    PACKET_CORRUPTION, ///< Corrupt packet contents
    DUPLICATE_PACKETS, ///< Send duplicate packets
    REORDER_PACKETS    ///< Reorder packet sequence
  };

  CommFaultType type = CommFaultType::PACKET_LOSS;
  double fault_probability = 0.01; ///< Probability of fault occurrence
  std::chrono::milliseconds delay_range{100}; ///< Delay range for packet delay
  uint32_t max_packet_size = 1500;            ///< Maximum packet size to affect
};

/**
 * @brief Fault injection configuration
 */
struct FaultInjectionConfig {
  FaultType fault_type;
  FaultTarget target;
  InjectionTiming timing = InjectionTiming::IMMEDIATE;
  std::chrono::milliseconds injection_delay{0};
  std::chrono::milliseconds injection_period{1000};
  uint32_t max_injections = 1;
  bool auto_recovery = true;
  std::chrono::milliseconds recovery_timeout{5000};

  // Fault-specific configuration (use based on fault_type)
  TimingFaultConfig timing_config;
  DataCorruptionConfig data_config;
  CommunicationFaultConfig comm_config;

  // Safety constraints
  bool respect_safety_constraints = true;
  std::vector<std::string> excluded_critical_functions;
  double max_system_impact = 0.1; ///< Maximum allowed system impact (10%)
};

/**
 * @brief Fault injection result
 */
struct FaultInjectionResult {
  enum class Status {
    SUCCESS = 0,
    FAILED = 1,
    BLOCKED_BY_SAFETY = 2,
    TARGET_NOT_FOUND = 3,
    TIMEOUT = 4
  };

  Status status;
  std::string description;
  std::chrono::steady_clock::time_point injection_time;
  std::chrono::steady_clock::time_point recovery_time;
  std::vector<std::string> observed_effects;
  std::vector<std::string> safety_violations;

  // Propagation analysis
  std::vector<std::string> affected_components;
  std::vector<std::string> propagation_path;
  double system_impact_score = 0.0;
};

/**
 * @brief Fault propagation monitor callback
 */
using FaultPropagationCallback =
    std::function<void(const FaultInjectionResult &)>;

/**
 * @brief Safety check callback for fault injection
 *
 * This callback is invoked before injecting a fault to ensure
 * it's safe to proceed in the current system state.
 */
using SafetyCheckCallback = std::function<bool(const FaultInjectionConfig &)>;

/**
 * @class FaultInjector
 * @brief Main fault injection engine for BCI systems
 *
 * The FaultInjector provides systematic fault injection capabilities
 * designed specifically for safety-critical BCI systems. It includes
 * built-in safety mechanisms to prevent dangerous fault injections.
 *
 * Thread Safety: This class is thread-safe for concurrent fault injection.
 *
 * Real-time Constraints: Critical path methods have deterministic timing.
 */
class FaultInjector {
public:
  /**
   * @brief Factory method to create a fault injector instance
   * @return Unique pointer to the created fault injector
   * @throws std::runtime_error if initialization fails
   */
  static std::unique_ptr<FaultInjector> create();

  /**
   * @brief Virtual destructor for proper cleanup
   */
  virtual ~FaultInjector() = default;

  /**
   * @brief Initialize the fault injector
   * @return true if initialization successful, false otherwise
   * @post Fault injector is ready for operation
   */
  virtual bool initialize() = 0;

  /**
   * @brief Configure a fault injection target
   * @param target_name Name of the target component
   * @param target Target specification
   * @return true if target configured successfully, false otherwise
   * @pre Fault injector must be initialized
   * @pre target_name must not be empty
   */
  virtual bool configure_target(const std::string &target_name,
                                const FaultTarget &target) = 0;

  /**
   * @brief Inject a timing fault
   * @param config Timing fault configuration
   * @return Fault injection result
   * @pre Fault injector must be initialized
   * @pre config must be valid
   * @post Timing fault is injected according to configuration
   */
  virtual FaultInjectionResult
  inject_timing_fault(const FaultInjectionConfig &config) = 0;

  /**
   * @brief Inject a data corruption fault
   * @param config Data corruption fault configuration
   * @return Fault injection result
   * @pre Fault injector must be initialized
   * @pre config must be valid
   */
  virtual FaultInjectionResult
  inject_data_corruption(const FaultInjectionConfig &config) = 0;

  /**
   * @brief Inject a communication fault
   * @param config Communication fault configuration
   * @return Fault injection result
   * @pre Fault injector must be initialized
   * @pre config must be valid
   */
  virtual FaultInjectionResult
  inject_communication_fault(const FaultInjectionConfig &config) = 0;

  /**
   * @brief Inject a hardware failure fault
   * @param config Hardware failure configuration
   * @return Fault injection result
   * @pre Fault injector must be initialized
   * @pre config must be valid
   */
  virtual FaultInjectionResult
  inject_hardware_failure(const FaultInjectionConfig &config) = 0;

  /**
   * @brief Start continuous fault injection campaign
   * @param configs Vector of fault configurations to execute
   * @return true if campaign started successfully, false otherwise
   * @pre Fault injector must be initialized
   * @pre configs must not be empty
   */
  virtual bool
  start_fault_campaign(const std::vector<FaultInjectionConfig> &configs) = 0;

  /**
   * @brief Stop current fault injection campaign
   * @return true if campaign stopped successfully, false otherwise
   * @pre Campaign must be running
   * @post All active fault injections are stopped
   */
  virtual bool stop_fault_campaign() = 0;

  /**
   * @brief Register fault propagation monitoring callback
   * @param callback Callback to invoke when fault propagation is detected
   * @pre callback must be valid
   */
  virtual void
  register_propagation_callback(FaultPropagationCallback callback) = 0;

  /**
   * @brief Register safety check callback
   * @param callback Callback to check safety before fault injection
   * @pre callback must be valid
   */
  virtual void register_safety_callback(SafetyCheckCallback callback) = 0;

  /**
   * @brief Get fault injection statistics
   * @return Statistics about fault injections performed
   */
  virtual std::vector<FaultInjectionResult> get_statistics() const = 0;

  /**
   * @brief Check if fault injection campaign is running
   * @return true if campaign is active, false otherwise
   */
  virtual bool is_campaign_active() const noexcept = 0;

  /**
   * @brief Emergency stop all fault injections
   * @return true if emergency stop successful, false otherwise
   * @note This method must complete within 50ms
   * @post All fault injections are immediately stopped
   */
  virtual bool emergency_stop() noexcept = 0;

protected:
  /**
   * @brief Protected constructor to enforce factory pattern
   */
  FaultInjector() = default;

  /**
   * @brief Copy constructor (deleted for safety)
   */
  FaultInjector(const FaultInjector &) = delete;

  /**
   * @brief Assignment operator (deleted for safety)
   */
  FaultInjector &operator=(const FaultInjector &) = delete;
};

/**
 * @brief Utility functions for fault injection
 */
namespace FaultInjectionUtils {
/**
 * @brief Validate fault injection configuration
 * @param config Configuration to validate
 * @return true if configuration is valid and safe, false otherwise
 */
bool validate_fault_config(const FaultInjectionConfig &config) noexcept;

/**
 * @brief Calculate system impact score
 * @param result Fault injection result
 * @return Impact score between 0.0 and 1.0
 */
double calculate_impact_score(const FaultInjectionResult &result) noexcept;

/**
 * @brief Convert fault type to string
 * @param type Fault type to convert
 * @return String representation
 */
std::string fault_type_to_string(FaultType type) noexcept;

/**
 * @brief Check if target is in safety-critical path
 * @param target Target to check
 * @return true if target is safety-critical, false otherwise
 */
bool is_safety_critical_target(const FaultTarget &target) noexcept;
} // namespace FaultInjectionUtils

} // namespace FaultInjection
} // namespace IVVFramework
