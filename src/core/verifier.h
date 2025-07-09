/**
 * @file verifier.h
 * @brief Core IV&V Framework Verifier Interface
 *
 * This file defines the main verifier interface for the IV&V Framework
 * designed for BCI safety-critical systems. The verifier orchestrates
 * all verification and validation activities.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 *
 * @copyright Copyright (c) 2025 IV&V Framework for BCI Systems
 *
 * Safety-Critical Notice:
 * This software is designed for use in safety-critical BCI systems.
 * All modifications must undergo proper safety analysis and validation.
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
 * @brief Result codes for IV&V operations
 */
enum class VerificationResult {
  SUCCESS = 0,         ///< Operation completed successfully
  FAILURE = 1,         ///< Operation failed
  TIMEOUT = 2,         ///< Operation timed out
  INVALID_INPUT = 3,   ///< Invalid input parameters
  SAFETY_VIOLATION = 4 ///< Safety property violation detected
};

/**
 * @brief Configuration structure for the verifier
 */
struct VerifierConfig {
  std::string device_name;               ///< Name of the BCI device under test
  std::string config_file_path;          ///< Path to configuration file
  bool enable_fault_injection = true;    ///< Enable fault injection
  bool enable_timing_analysis = true;    ///< Enable timing analysis
  bool enable_regression_testing = true; ///< Enable regression testing
  std::chrono::milliseconds timeout{30000}; ///< Default timeout for operations

  // Safety-critical parameters
  bool enforce_safety_constraints = true; ///< Enforce safety constraints
  double max_injection_rate = 0.1; ///< Maximum fault injection rate (10%)
  std::vector<std::string>
      critical_functions; ///< List of safety-critical functions
};

/**
 * @brief Verification result details
 */
struct VerificationReport {
  VerificationResult result;
  std::string description;
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;
  std::vector<std::string> warnings;
  std::vector<std::string> errors;

  // Safety-specific metrics
  size_t safety_violations_detected = 0;
  size_t timing_violations_detected = 0;
  size_t fault_propagations_observed = 0;
};

/**
 * @brief Safety assertion callback type
 *
 * This callback is invoked when a safety assertion is evaluated.
 * It allows the framework to check safety-critical conditions.
 */
using SafetyAssertionCallback =
    std::function<bool(const std::string &assertion_name)>;

/**
 * @class Verifier
 * @brief Main IV&V Framework verifier class
 *
 * The Verifier class is the central component of the IV&V Framework.
 * It coordinates all verification activities including fault injection,
 * timing analysis, and regression testing for BCI safety-critical systems.
 *
 * Thread Safety: This class is not thread-safe. External synchronization
 * is required for concurrent access.
 *
 * Real-time Constraints: Methods marked as real-time safe have deterministic
 * execution times and do not perform dynamic memory allocation.
 */
class Verifier {
public:
  /**
   * @brief Factory method to create a verifier instance
   * @param device_name Name of the BCI device to verify
   * @param config Configuration parameters for the verifier
   * @return Unique pointer to the created verifier instance
   * @throws std::invalid_argument if device_name is empty
   * @throws std::runtime_error if initialization fails
   */
  static std::unique_ptr<Verifier>
  create(const std::string &device_name,
         const VerifierConfig &config = VerifierConfig{});

  /**
   * @brief Virtual destructor for proper cleanup
   */
  virtual ~Verifier() = default;

  /**
   * @brief Initialize the verifier with the given configuration
   * @param config Configuration parameters
   * @return VerificationResult indicating success or failure
   * @pre config.device_name must not be empty
   * @post If successful, verifier is ready for operation
   */
  virtual VerificationResult initialize(const VerifierConfig &config) = 0;

  /**
   * @brief Execute a verification scenario from a DSL file
   * @param scenario_file Path to the scenario file
   * @return Verification report with detailed results
   * @pre Verifier must be initialized
   * @pre scenario_file must exist and be readable
   * @post Verification scenario is executed and report is generated
   */
  virtual VerificationReport
  execute_scenario(const std::string &scenario_file) = 0;

  /**
   * @brief Execute a verification scenario from DSL content
   * @param scenario_content DSL content as string
   * @return Verification report with detailed results
   * @pre Verifier must be initialized
   * @pre scenario_content must be valid DSL syntax
   */
  virtual VerificationReport
  execute_scenario_content(const std::string &scenario_content) = 0;

  /**
   * @brief Register a safety assertion callback
   * @param name Name of the assertion
   * @param callback Callback function to evaluate the assertion
   * @pre name must not be empty
   * @pre callback must be valid
   * @post Assertion is registered and will be evaluated during verification
   */
  virtual void register_safety_assertion(const std::string &name,
                                         SafetyAssertionCallback callback) = 0;

  /**
   * @brief Start continuous monitoring mode
   * @return VerificationResult indicating success or failure
   * @pre Verifier must be initialized
   * @post Continuous monitoring is active (real-time safe)
   */
  virtual VerificationResult start_monitoring() = 0;

  /**
   * @brief Stop continuous monitoring mode
   * @return VerificationResult indicating success or failure
   * @pre Monitoring must be active
   * @post Monitoring is stopped and resources are released
   */
  virtual VerificationResult stop_monitoring() = 0;

  /**
   * @brief Check if the verifier is currently monitoring
   * @return true if monitoring is active, false otherwise
   * @note This method is real-time safe
   */
  virtual bool is_monitoring() const noexcept = 0;

  /**
   * @brief Get the current configuration
   * @return Current verifier configuration
   * @note This method is real-time safe
   */
  virtual const VerifierConfig &get_config() const noexcept = 0;

  /**
   * @brief Get verification statistics
   * @return Verification statistics since initialization
   * @note This method is real-time safe
   */
  virtual VerificationReport get_statistics() const noexcept = 0;

  /**
   * @brief Emergency shutdown of all verification activities
   * @return VerificationResult indicating shutdown status
   * @note This method is real-time safe and must complete within 100ms
   * @post All verification activities are stopped immediately
   */
  virtual VerificationResult emergency_shutdown() noexcept = 0;

protected:
  /**
   * @brief Protected constructor to enforce factory pattern
   */
  Verifier() = default;

  /**
   * @brief Copy constructor (deleted for safety)
   */
  Verifier(const Verifier &) = delete;

  /**
   * @brief Assignment operator (deleted for safety)
   */
  Verifier &operator=(const Verifier &) = delete;
};

/**
 * @brief Utility functions for the verifier
 */
namespace VerifierUtils {
/**
 * @brief Convert VerificationResult to string
 * @param result The result to convert
 * @return String representation of the result
 */
std::string result_to_string(VerificationResult result) noexcept;

/**
 * @brief Validate configuration parameters
 * @param config Configuration to validate
 * @return true if configuration is valid, false otherwise
 */
bool validate_config(const VerifierConfig &config) noexcept;

/**
 * @brief Calculate verification duration
 * @param report Verification report
 * @return Duration in milliseconds
 */
std::chrono::milliseconds
calculate_duration(const VerificationReport &report) noexcept;
} // namespace VerifierUtils

} // namespace Core
} // namespace IVVFramework
