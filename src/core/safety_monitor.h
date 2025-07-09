/**
 * @file safety_monitor.h
 * @brief Safety Monitoring System for IV&V Framework
 *
 * Provides continuous safety monitoring and constraint verification
 * for BCI safety-critical systems with real-time violation detection
 * and emergency response capabilities.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 *
 * @copyright Copyright (c) 2025 IV&V Framework for BCI Systems
 *
 * Safety-Critical Notice:
 * The safety monitor is a critical component that must maintain
 * continuous operation and respond to safety violations within
 * bounded time constraints.
 */

#pragma once

#include "verifier.h"
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace IVVFramework {
namespace Core {

/**
 * @brief Safety monitoring result codes
 */
enum class SafetyResult {
  SAFE = 0,               ///< System is operating safely
  WARNING = 1,            ///< Warning condition detected
  VIOLATION = 2,          ///< Safety violation detected
  CRITICAL_VIOLATION = 3, ///< Critical safety violation
  SYSTEM_FAILURE = 4      ///< System failure detected
};

/**
 * @brief Safety constraint types
 */
enum class SafetyConstraintType {
  TIMING_CONSTRAINT = 0,        ///< Real-time timing constraints
  RESOURCE_CONSTRAINT = 1,      ///< Resource utilization constraints
  SIGNAL_CONSTRAINT = 2,        ///< Signal processing constraints
  COMMUNICATION_CONSTRAINT = 3, ///< Communication safety constraints
  PATIENT_SAFETY = 4,           ///< Direct patient safety constraints
  SYSTEM_INTEGRITY = 5          ///< Overall system integrity constraints
};

/**
 * @brief Safety constraint definition
 */
struct SafetyConstraint {
  std::string name;
  SafetyConstraintType type;
  std::string description;
  bool is_critical = false;
  std::chrono::milliseconds check_interval{100};
  std::chrono::milliseconds violation_timeout{1000};
  std::function<SafetyResult()> check_function;
  std::function<void(SafetyResult)> violation_handler;
};

/**
 * @brief Safety violation details
 */
struct SafetyViolation {
  std::chrono::system_clock::time_point timestamp;
  std::string constraint_name;
  SafetyConstraintType constraint_type;
  SafetyResult severity;
  std::string description;
  std::string context;
  bool is_critical;
  bool requires_emergency_stop;
  std::vector<std::string> affected_components;
  std::string mitigation_action;
};

/**
 * @brief Safety system status
 */
struct SafetyStatus {
  bool is_monitoring_active = false;
  std::chrono::system_clock::time_point last_check_time;
  size_t active_constraints = 0;
  size_t total_violations = 0;
  size_t critical_violations = 0;
  std::vector<SafetyViolation> recent_violations;
  std::chrono::milliseconds max_check_duration{0};
  std::chrono::milliseconds avg_check_duration{0};
};

/**
 * @brief Safety violation callback
 */
using SafetyViolationCallback = std::function<void(const SafetyViolation &)>;

/**
 * @brief Emergency stop callback
 */
using EmergencyStopCallback = std::function<bool()>;

/**
 * @class SafetyMonitor
 * @brief Continuous safety monitoring system
 *
 * The SafetyMonitor provides real-time safety constraint monitoring
 * for BCI systems with bounded response times and automatic
 * emergency procedures.
 *
 * Thread Safety: This class is thread-safe for concurrent access.
 * Real-time Constraints: All monitoring operations have deterministic timing.
 */
class SafetyMonitor {
public:
  /**
   * @brief Constructor
   */
  SafetyMonitor();

  /**
   * @brief Destructor
   */
  ~SafetyMonitor();

  /**
   * @brief Initialize safety monitor
   * @param config Verifier configuration with safety parameters
   * @return true if initialization successful, false otherwise
   * @pre config must be valid
   * @post Safety monitor is ready for operation
   */
  bool initialize(const VerifierConfig &config);

  /**
   * @brief Register safety constraint
   * @param constraint Safety constraint definition
   * @return true if registered successfully, false otherwise
   * @pre constraint must be valid with proper check function
   * @post Constraint is registered and will be monitored
   */
  bool register_constraint(const SafetyConstraint &constraint);

  /**
   * @brief Start continuous safety monitoring
   * @return SafetyResult indicating startup status
   * @pre Safety monitor must be initialized
   * @post Continuous monitoring is active
   */
  SafetyResult start_monitoring();

  /**
   * @brief Stop continuous safety monitoring
   * @return SafetyResult indicating shutdown status
   * @pre Monitoring must be active
   * @post Monitoring is stopped and resources are released
   */
  SafetyResult stop_monitoring();

  /**
   * @brief Check all safety constraints once
   * @return Overall safety result
   * @note This method is real-time safe and completes within 50ms
   */
  SafetyResult check_system_safety();

  /**
   * @brief Check specific safety constraint
   * @param constraint_name Name of constraint to check
   * @return Safety result for the constraint
   * @note This method is real-time safe
   */
  SafetyResult check_constraint(const std::string &constraint_name);

  /**
   * @brief Check scenario safety before execution
   * @param scenario_content DSL scenario content to validate
   * @return Safety result for the scenario
   * @pre scenario_content must be valid DSL syntax
   */
  SafetyResult check_scenario_safety(const std::string &scenario_content);

  /**
   * @brief Register safety violation callback
   * @param callback Callback to invoke on safety violations
   * @pre callback must be valid
   */
  void register_violation_callback(SafetyViolationCallback callback);

  /**
   * @brief Register emergency stop callback
   * @param callback Callback to invoke for emergency stops
   * @pre callback must be valid
   */
  void register_emergency_stop_callback(EmergencyStopCallback callback);

  /**
   * @brief Get current safety status
   * @return Current safety system status
   * @note This method is real-time safe
   */
  SafetyStatus get_safety_status() const;

  /**
   * @brief Get recent safety violations
   * @param max_count Maximum number of violations to return
   * @return Vector of recent violations
   */
  std::vector<SafetyViolation>
  get_recent_violations(size_t max_count = 10) const;

  /**
   * @brief Check if system is in safe state
   * @return true if system is safe, false otherwise
   * @note This method is real-time safe
   */
  bool is_system_safe() const;

  /**
   * @brief Acknowledge safety violation
   * @param violation_id ID of violation to acknowledge
   * @param acknowledgment_reason Reason for acknowledgment
   * @return true if acknowledged successfully, false otherwise
   */
  bool acknowledge_violation(const std::string &violation_id,
                             const std::string &acknowledgment_reason);

  /**
   * @brief Force emergency stop of all monitored systems
   * @return true if emergency stop successful, false otherwise
   * @note This method must complete within 100ms
   */
  bool emergency_stop() noexcept;

  /**
   * @brief Reset safety monitor after emergency stop
   * @return true if reset successful, false otherwise
   * @pre System must be in emergency stop state
   * @post Safety monitor is ready for normal operation
   */
  bool reset_after_emergency();

  /**
   * @brief Enable or disable specific safety constraint
   * @param constraint_name Name of constraint to modify
   * @param enabled Whether to enable or disable
   * @return true if modified successfully, false otherwise
   */
  bool set_constraint_enabled(const std::string &constraint_name, bool enabled);

  /**
   * @brief Update safety constraint check interval
   * @param constraint_name Name of constraint to modify
   * @param interval New check interval
   * @return true if updated successfully, false otherwise
   * @pre interval must be within valid range (10ms - 10s)
   */
  bool update_constraint_interval(const std::string &constraint_name,
                                  std::chrono::milliseconds interval);

  /**
   * @brief Generate safety monitoring report
   * @return Comprehensive safety monitoring report
   */
  std::string generate_safety_report() const;

  /**
   * @brief Check if monitoring is active
   * @return true if monitoring is active, false otherwise
   * @note This method is real-time safe
   */
  bool is_monitoring_active() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

/**
 * @brief Safety monitoring utility functions
 */
namespace SafetyUtils {
/**
 * @brief Convert safety result to string
 * @param result Safety result to convert
 * @return String representation
 */
std::string safety_result_to_string(SafetyResult result);

/**
 * @brief Convert constraint type to string
 * @param type Constraint type to convert
 * @return String representation
 */
std::string constraint_type_to_string(SafetyConstraintType type);

/**
 * @brief Create default BCI safety constraints
 * @return Vector of default safety constraints for BCI systems
 */
std::vector<SafetyConstraint> create_default_bci_constraints();

/**
 * @brief Validate safety constraint definition
 * @param constraint Constraint to validate
 * @return true if constraint is valid, false otherwise
 */
bool validate_safety_constraint(const SafetyConstraint &constraint);

/**
 * @brief Calculate constraint priority based on criticality
 * @param constraint Constraint to evaluate
 * @return Priority value (higher number = higher priority)
 */
int calculate_constraint_priority(const SafetyConstraint &constraint);

/**
 * @brief Check if violation requires immediate emergency stop
 * @param violation Violation to evaluate
 * @return true if emergency stop required, false otherwise
 */
bool requires_emergency_stop(const SafetyViolation &violation);
} // namespace SafetyUtils

} // namespace Core
} // namespace IVVFramework
