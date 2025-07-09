/**
 * @file config_manager.cpp
 * @brief Configuration Management Implementation
 *
 * Implementation of the configuration management system for the IV&V Framework
 * with safety-critical parameter validation and real-time updates.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 */

#include "config_manager.h"
#include <algorithm>
#include <fstream>
#include <mutex>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace IVVFramework {
namespace Core {

/**
 * @brief Private implementation class
 */
class ConfigManager::Impl {
public:
  mutable std::mutex config_mutex_;
  std::unordered_map<std::string, std::string> parameters_;
  std::unordered_map<std::string, ConfigParameter> parameter_definitions_;
  std::vector<ConfigValidationCallback> validation_callbacks_;
  std::string component_name_;
  bool initialized_ = false;

  bool load_from_file(const std::string &file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
      return false;
    }

    std::string line;
    while (std::getline(file, line)) {
      // Skip comments and empty lines
      if (line.empty() || line[0] == '#' || line[0] == ';') {
        continue;
      }

      // Parse key=value pairs
      auto equals_pos = line.find('=');
      if (equals_pos != std::string::npos) {
        std::string key = line.substr(0, equals_pos);
        std::string value = line.substr(equals_pos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        parameters_[key] = value;
      }
    }

    return true;
  }

  bool validate_parameter(const std::string &name,
                          const std::string &value) const {
    // Check parameter definition
    auto def_it = parameter_definitions_.find(name);
    if (def_it != parameter_definitions_.end()) {
      const auto &def = def_it->second;

      // Custom validator
      if (def.validator && !def.validator(value)) {
        return false;
      }

      // Type and range validation
      try {
        switch (def.type) {
        case ConfigType::INTEGER: {
          int val = std::stoi(value);
          if (!def.min_value.empty()) {
            int min_val = std::stoi(def.min_value);
            if (val < min_val)
              return false;
          }
          if (!def.max_value.empty()) {
            int max_val = std::stoi(def.max_value);
            if (val > max_val)
              return false;
          }
          break;
        }
        case ConfigType::DOUBLE: {
          double val = std::stod(value);
          if (!def.min_value.empty()) {
            double min_val = std::stod(def.min_value);
            if (val < min_val)
              return false;
          }
          if (!def.max_value.empty()) {
            double max_val = std::stod(def.max_value);
            if (val > max_val)
              return false;
          }
          break;
        }
        case ConfigType::BOOLEAN: {
          if (value != "true" && value != "false" && value != "1" &&
              value != "0") {
            return false;
          }
          break;
        }
        case ConfigType::DURATION: {
          ConfigUtils::parse_duration(value);
          break;
        }
        case ConfigType::STRING:
        default:
          // String validation handled by custom validator
          break;
        }
      } catch (...) {
        return false;
      }
    }

    // Custom validation callbacks
    for (const auto &callback : validation_callbacks_) {
      auto result = callback(name, value);
      if (result != ConfigValidationResult::VALID) {
        return false;
      }
    }

    return true;
  }
};

ConfigManager::ConfigManager() : pimpl_(std::make_unique<Impl>()) {}

ConfigManager::~ConfigManager() = default;

bool ConfigManager::initialize(const std::string &config_file_path) {
  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);

  // Register default safety-critical parameters
  auto default_params = ConfigUtils::create_default_safety_parameters();
  for (const auto &param : default_params) {
    pimpl_->parameter_definitions_[param.name] = param;
    if (!param.default_value.empty()) {
      pimpl_->parameters_[param.name] = param.default_value;
    }
  }

  // Load configuration file if provided
  if (!config_file_path.empty()) {
    if (!pimpl_->load_from_file(config_file_path)) {
      return false;
    }
  }

  // Validate all parameters
  if (!validate_all_parameters()) {
    return false;
  }

  pimpl_->initialized_ = true;
  return true;
}

bool ConfigManager::load_config_file(const std::string &file_path) {
  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);
  return pimpl_->load_from_file(file_path);
}

std::string ConfigManager::get_string(const std::string &name,
                                      const std::string &default_value) const {
  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);
  auto it = pimpl_->parameters_.find(name);
  return (it != pimpl_->parameters_.end()) ? it->second : default_value;
}

int ConfigManager::get_int(const std::string &name, int default_value) const {
  std::string str_value = get_string(name);
  if (str_value.empty()) {
    return default_value;
  }

  try {
    return std::stoi(str_value);
  } catch (...) {
    return default_value;
  }
}

double ConfigManager::get_double(const std::string &name,
                                 double default_value) const {
  std::string str_value = get_string(name);
  if (str_value.empty()) {
    return default_value;
  }

  try {
    return std::stod(str_value);
  } catch (...) {
    return default_value;
  }
}

bool ConfigManager::get_bool(const std::string &name,
                             bool default_value) const {
  std::string str_value = get_string(name);
  if (str_value.empty()) {
    return default_value;
  }

  return (str_value == "true" || str_value == "1");
}

std::chrono::milliseconds
ConfigManager::get_duration(const std::string &name,
                            std::chrono::milliseconds default_value) const {
  std::string str_value = get_string(name);
  if (str_value.empty()) {
    return default_value;
  }

  try {
    return ConfigUtils::parse_duration(str_value);
  } catch (...) {
    return default_value;
  }
}

bool ConfigManager::set_string(const std::string &name,
                               const std::string &value) {
  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);

  if (!pimpl_->validate_parameter(name, value)) {
    return false;
  }

  pimpl_->parameters_[name] = value;
  return true;
}

bool ConfigManager::set_int(const std::string &name, int value) {
  return set_string(name, std::to_string(value));
}

bool ConfigManager::set_double(const std::string &name, double value) {
  return set_string(name, std::to_string(value));
}

bool ConfigManager::set_bool(const std::string &name, bool value) {
  return set_string(name, value ? "true" : "false");
}

bool ConfigManager::register_parameter(const ConfigParameter &param) {
  if (param.name.empty()) {
    return false;
  }

  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);
  pimpl_->parameter_definitions_[param.name] = param;

  // Set default value if provided and parameter doesn't exist
  if (!param.default_value.empty() &&
      pimpl_->parameters_.find(param.name) == pimpl_->parameters_.end()) {
    pimpl_->parameters_[param.name] = param.default_value;
  }

  return true;
}

void ConfigManager::register_validation_callback(
    ConfigValidationCallback callback) {
  if (callback) {
    std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);
    pimpl_->validation_callbacks_.push_back(callback);
  }
}

bool ConfigManager::validate_all_parameters() const {
  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);

  // Check all required parameters are present
  for (const auto &def_pair : pimpl_->parameter_definitions_) {
    const auto &def = def_pair.second;
    if (def.is_required) {
      auto param_it = pimpl_->parameters_.find(def.name);
      if (param_it == pimpl_->parameters_.end()) {
        return false; // Required parameter missing
      }
    }
  }

  // Validate all existing parameters
  for (const auto &param_pair : pimpl_->parameters_) {
    if (!pimpl_->validate_parameter(param_pair.first, param_pair.second)) {
      return false;
    }
  }

  return true;
}

bool ConfigManager::has_parameter(const std::string &name) const {
  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);
  return pimpl_->parameters_.find(name) != pimpl_->parameters_.end();
}

std::vector<std::string> ConfigManager::get_parameter_names() const {
  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);
  std::vector<std::string> names;
  names.reserve(pimpl_->parameters_.size());

  for (const auto &pair : pimpl_->parameters_) {
    names.push_back(pair.first);
  }

  return names;
}

bool ConfigManager::save_config_file(const std::string &file_path) const {
  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);

  std::ofstream file(file_path);
  if (!file.is_open()) {
    return false;
  }

  file << "# IV&V Framework Configuration\n";
  file << "# Generated at: "
       << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";

  for (const auto &pair : pimpl_->parameters_) {
    file << pair.first << "=" << pair.second << "\n";
  }

  return file.good();
}

bool ConfigManager::reset_to_defaults() {
  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);

  pimpl_->parameters_.clear();

  // Set default values from parameter definitions
  for (const auto &def_pair : pimpl_->parameter_definitions_) {
    const auto &def = def_pair.second;
    if (!def.default_value.empty()) {
      pimpl_->parameters_[def.name] = def.default_value;
    }
  }

  return validate_all_parameters();
}

bool ConfigManager::is_safety_compliant() const {
  std::lock_guard<std::mutex> lock(pimpl_->config_mutex_);

  // Check all safety-critical parameters are valid
  for (const auto &def_pair : pimpl_->parameter_definitions_) {
    const auto &def = def_pair.second;
    if (def.is_safety_critical) {
      auto param_it = pimpl_->parameters_.find(def.name);
      if (param_it == pimpl_->parameters_.end()) {
        return false; // Safety-critical parameter missing
      }

      if (!pimpl_->validate_parameter(def.name, param_it->second)) {
        return false; // Safety-critical parameter invalid
      }
    }
  }

  return true;
}

// Utility functions implementation
namespace ConfigUtils {

std::chrono::milliseconds parse_duration(const std::string &duration_str) {
  std::regex duration_regex(R"((\d+)(ms|s|m|h))");
  std::smatch matches;

  if (!std::regex_match(duration_str, matches, duration_regex)) {
    throw std::invalid_argument("Invalid duration format: " + duration_str);
  }

  int value = std::stoi(matches[1].str());
  std::string unit = matches[2].str();

  if (unit == "ms") {
    return std::chrono::milliseconds(value);
  } else if (unit == "s") {
    return std::chrono::milliseconds(value * 1000);
  } else if (unit == "m") {
    return std::chrono::milliseconds(value * 60 * 1000);
  } else if (unit == "h") {
    return std::chrono::milliseconds(value * 60 * 60 * 1000);
  }

  throw std::invalid_argument("Unknown duration unit: " + unit);
}

std::string duration_to_string(std::chrono::milliseconds duration) {
  auto ms = duration.count();

  if (ms % 1000 == 0) {
    auto seconds = ms / 1000;
    if (seconds % 60 == 0) {
      auto minutes = seconds / 60;
      if (minutes % 60 == 0) {
        auto hours = minutes / 60;
        return std::to_string(hours) + "h";
      }
      return std::to_string(minutes) + "m";
    }
    return std::to_string(seconds) + "s";
  }

  return std::to_string(ms) + "ms";
}

bool is_valid_parameter_name(const std::string &name) {
  if (name.empty()) {
    return false;
  }

  // Check for valid characters (alphanumeric, underscore, dot)
  std::regex valid_name_regex(R"([a-zA-Z][a-zA-Z0-9_.]*)");
  return std::regex_match(name, valid_name_regex);
}

std::vector<ConfigParameter> create_default_safety_parameters() {
  std::vector<ConfigParameter> params;

  // Safety monitoring parameters
  params.push_back({"safety.monitor.enabled", ConfigType::BOOLEAN,
                    "Enable safety monitoring",
                    true, // is_safety_critical
                    true, // is_required
                    "true"});

  params.push_back({"safety.monitor.check_interval", ConfigType::DURATION,
                    "Safety check interval", true, true, "100ms", "10ms",
                    "1s"});

  params.push_back({"safety.fault_injection.max_rate", ConfigType::DOUBLE,
                    "Maximum fault injection rate", true, true, "0.1", "0.0",
                    "0.5"});

  params.push_back({"safety.timing.max_deviation", ConfigType::DURATION,
                    "Maximum timing deviation allowed", true, true, "1000ms",
                    "1ms", "10s"});

  return params;
}

} // namespace ConfigUtils

} // namespace Core
} // namespace IVVFramework
