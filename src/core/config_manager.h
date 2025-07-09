/**
 * @file config_manager.h
 * @brief Configuration Management for IV&V Framework
 * 
 * Provides centralized configuration management for the IV&V Framework
 * with support for safety-critical parameter validation and real-time
 * configuration updates.
 * 
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 * 
 * @copyright Copyright (c) 2025 IV&V Framework for BCI Systems
 * 
 * Safety-Critical Notice:
 * Configuration changes must be validated against safety constraints
 * before being applied to prevent unsafe system states.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <functional>

namespace IVVFramework {
namespace Core {

/**
 * @brief Configuration parameter types
 */
enum class ConfigType {
    STRING = 0,
    INTEGER = 1,
    DOUBLE = 2,
    BOOLEAN = 3,
    DURATION = 4
};

/**
 * @brief Configuration validation result
 */
enum class ConfigValidationResult {
    VALID = 0,
    INVALID_TYPE = 1,
    OUT_OF_RANGE = 2,
    SAFETY_VIOLATION = 3,
    MISSING_REQUIRED = 4
};

/**
 * @brief Configuration parameter definition
 */
struct ConfigParameter {
    std::string name;
    ConfigType type;
    std::string description;
    bool is_safety_critical = false;
    bool is_required = false;
    std::string default_value;
    std::string min_value;
    std::string max_value;
    std::function<bool(const std::string&)> validator;
};

/**
 * @brief Configuration validation callback
 */
using ConfigValidationCallback = std::function<ConfigValidationResult(
    const std::string& name, 
    const std::string& value
)>;

/**
 * @class ConfigManager
 * @brief Centralized configuration management system
 * 
 * The ConfigManager provides safe configuration parameter management
 * with validation, safety checking, and real-time updates for the
 * IV&V Framework.
 * 
 * Thread Safety: This class is thread-safe for concurrent access.
 */
class ConfigManager {
public:
    /**
     * @brief Constructor
     */
    ConfigManager();

    /**
     * @brief Destructor
     */
    ~ConfigManager();

    /**
     * @brief Initialize configuration manager
     * @param config_file_path Path to configuration file
     * @return true if initialization successful, false otherwise
     * @pre config_file_path must be valid and readable
     * @post Configuration manager is ready for use
     */
    bool initialize(const std::string& config_file_path);

    /**
     * @brief Load configuration from file
     * @param file_path Path to configuration file
     * @return true if loaded successfully, false otherwise
     * @pre file_path must exist and be readable
     * @post Configuration is loaded and validated
     */
    bool load_config_file(const std::string& file_path);

    /**
     * @brief Get string configuration parameter
     * @param name Parameter name
     * @param default_value Default value if parameter not found
     * @return Parameter value or default
     * @note This method is real-time safe
     */
    std::string get_string(const std::string& name, const std::string& default_value = "") const;

    /**
     * @brief Get integer configuration parameter
     * @param name Parameter name
     * @param default_value Default value if parameter not found
     * @return Parameter value or default
     * @note This method is real-time safe
     */
    int get_int(const std::string& name, int default_value = 0) const;

    /**
     * @brief Get double configuration parameter
     * @param name Parameter name
     * @param default_value Default value if parameter not found
     * @return Parameter value or default
     * @note This method is real-time safe
     */
    double get_double(const std::string& name, double default_value = 0.0) const;

    /**
     * @brief Get boolean configuration parameter
     * @param name Parameter name
     * @param default_value Default value if parameter not found
     * @return Parameter value or default
     * @note This method is real-time safe
     */
    bool get_bool(const std::string& name, bool default_value = false) const;

    /**
     * @brief Get duration configuration parameter
     * @param name Parameter name
     * @param default_value Default value if parameter not found
     * @return Parameter value or default
     * @note This method is real-time safe
     */
    std::chrono::milliseconds get_duration(
        const std::string& name, 
        std::chrono::milliseconds default_value = std::chrono::milliseconds{0}
    ) const;

    /**
     * @brief Set string configuration parameter
     * @param name Parameter name
     * @param value Parameter value
     * @return true if set successfully, false otherwise
     * @pre Parameter must pass validation
     * @post Parameter is updated if validation passes
     */
    bool set_string(const std::string& name, const std::string& value);

    /**
     * @brief Set integer configuration parameter
     * @param name Parameter name
     * @param value Parameter value
     * @return true if set successfully, false otherwise
     */
    bool set_int(const std::string& name, int value);

    /**
     * @brief Set double configuration parameter
     * @param name Parameter name
     * @param value Parameter value
     * @return true if set successfully, false otherwise
     */
    bool set_double(const std::string& name, double value);

    /**
     * @brief Set boolean configuration parameter
     * @param name Parameter name
     * @param value Parameter value
     * @return true if set successfully, false otherwise
     */
    bool set_bool(const std::string& name, bool value);

    /**
     * @brief Register configuration parameter definition
     * @param param Parameter definition
     * @return true if registered successfully, false otherwise
     * @pre param must be valid
     * @post Parameter definition is registered
     */
    bool register_parameter(const ConfigParameter& param);

    /**
     * @brief Register validation callback
     * @param callback Validation callback function
     * @pre callback must be valid
     * @post Callback is registered for validation
     */
    void register_validation_callback(ConfigValidationCallback callback);

    /**
     * @brief Validate all configuration parameters
     * @return true if all parameters are valid, false otherwise
     */
    bool validate_all_parameters() const;

    /**
     * @brief Check if parameter exists
     * @param name Parameter name
     * @return true if parameter exists, false otherwise
     * @note This method is real-time safe
     */
    bool has_parameter(const std::string& name) const;

    /**
     * @brief Get all parameter names
     * @return Vector of parameter names
     */
    std::vector<std::string> get_parameter_names() const;

    /**
     * @brief Save current configuration to file
     * @param file_path Output file path
     * @return true if saved successfully, false otherwise
     */
    bool save_config_file(const std::string& file_path) const;

    /**
     * @brief Reset to default configuration
     * @return true if reset successfully, false otherwise
     * @post All parameters are set to their default values
     */
    bool reset_to_defaults();

    /**
     * @brief Check if configuration is valid for safety-critical operation
     * @return true if configuration is safe, false otherwise
     */
    bool is_safety_compliant() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * @brief Configuration utility functions
 */
namespace ConfigUtils {
    /**
     * @brief Parse duration string (e.g., "100ms", "5s", "2m")
     * @param duration_str Duration string to parse
     * @return Parsed duration in milliseconds
     * @throws std::invalid_argument if format is invalid
     */
    std::chrono::milliseconds parse_duration(const std::string& duration_str);

    /**
     * @brief Convert duration to string
     * @param duration Duration to convert
     * @return String representation
     */
    std::string duration_to_string(std::chrono::milliseconds duration);

    /**
     * @brief Validate parameter name
     * @param name Parameter name to validate
     * @return true if name is valid, false otherwise
     */
    bool is_valid_parameter_name(const std::string& name);

    /**
     * @brief Create default safety-critical parameters
     * @return Vector of default safety parameters
     */
    std::vector<ConfigParameter> create_default_safety_parameters();
}

} // namespace Core
} // namespace IVVFramework
