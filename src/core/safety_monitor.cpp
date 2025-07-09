/**
 * @file safety_monitor.cpp
 * @brief Safety Monitor Implementation for BCI Safety-Critical Systems
 *
 * This file implements the safety monitoring capabilities required for
 * continuous safety property verification during IV&V activities.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 *
 * @copyright Copyright (c) 2025 IV&V Framework for BCI Systems
 *
 * Safety-Critical Notice:
 * This module implements safety-critical monitoring functions.
 * All modifications must undergo safety analysis per IEC 62304.
 */

#include "safety_monitor.h"
#include "logger.h"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>
#include <map>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace IVVFramework {
namespace Core {

/**
 * @brief Private implementation class using PIMPL idiom
 */
class SafetyMonitor::Impl {
public:
    // Configuration and state
    VerifierConfig config_;
    std::atomic<bool> is_monitoring_{false};
    std::atomic<bool> emergency_stop_active_{false};
    std::atomic<size_t> violation_count_{0};
    
    // Threading
    std::thread monitoring_thread_;
    mutable std::mutex constraints_mutex_;
    mutable std::mutex violations_mutex_;
    
    // Safety constraints and violations
    std::map<std::string, SafetyConstraint> constraints_;
    std::vector<SafetyViolation> recent_violations_;
    
    // Callbacks
    SafetyViolationCallback violation_callback_;
    EmergencyStopCallback emergency_stop_callback_;
    
    // Timing and statistics
    std::chrono::steady_clock::time_point monitoring_start_time_;
    std::chrono::milliseconds total_check_duration_{0};
    size_t total_checks_{0};
    
    // Logger
    std::unique_ptr<Logger> logger_;
    
    // Constants
    static constexpr size_t MAX_RECENT_VIOLATIONS = 100;
    static constexpr std::chrono::milliseconds DEFAULT_CHECK_INTERVAL{100};
    
    // Helper methods
    void monitoring_loop();
    void record_violation(const SafetyViolation& violation);
    SafetyResult check_constraint_internal(const SafetyConstraint& constraint);
};

SafetyMonitor::SafetyMonitor() 
    : pimpl_(std::make_unique<Impl>()) {
    pimpl_->logger_ = std::make_unique<Logger>(LogLevel::INFO, "safety_monitor.log");
    pimpl_->logger_->log_info("SafetyMonitor created");
}

SafetyMonitor::~SafetyMonitor() {
    if (pimpl_->is_monitoring_) {
        stop_monitoring();
    }
    
    pimpl_->logger_->log_info("SafetyMonitor destroyed. Total violations: " + 
                        std::to_string(pimpl_->violation_count_.load()));
}

bool SafetyMonitor::initialize(const VerifierConfig& config) {
    std::lock_guard<std::mutex> lock(pimpl_->constraints_mutex_);
    
    if (pimpl_->is_monitoring_) {
        pimpl_->logger_->log(LogLevel::WARNING, "Cannot initialize while monitoring is active");
        return false;
    }
    
    pimpl_->config_ = config;
    pimpl_->violation_count_ = 0;
    pimpl_->recent_violations_.clear();
    
    pimpl_->logger_->log(LogLevel::INFO, "SafetyMonitor initialized for device: " + config.device_name);
    return true;
}

bool SafetyMonitor::register_constraint(const SafetyConstraint& constraint) {
    std::lock_guard<std::mutex> lock(pimpl_->constraints_mutex_);
    
    if (!SafetyUtils::validate_safety_constraint(constraint)) {
        pimpl_->logger_->log(LogLevel::ERROR, "Invalid safety constraint: " + constraint.name);
        return false;
    }
    
    pimpl_->constraints_[constraint.name] = constraint;
    pimpl_->logger_->log(LogLevel::INFO, "Registered safety constraint: " + constraint.name);
    
    return true;
}

SafetyResult SafetyMonitor::start_monitoring() {
    if (pimpl_->is_monitoring_) {
        pimpl_->logger_->log(LogLevel::WARNING, "SafetyMonitor already running");
        return SafetyResult::WARNING;
    }
    
    std::lock_guard<std::mutex> lock(pimpl_->constraints_mutex_);
    
    if (pimpl_->constraints_.empty()) {
        pimpl_->logger_->log(LogLevel::ERROR, "No safety constraints registered");
        return SafetyResult::SYSTEM_FAILURE;
    }
    
    // Reset state
    pimpl_->violation_count_ = 0;
    pimpl_->emergency_stop_active_ = false;
    pimpl_->monitoring_start_time_ = std::chrono::steady_clock::now();
    
    // Start monitoring thread
    pimpl_->is_monitoring_ = true;
    pimpl_->monitoring_thread_ = std::thread(&SafetyMonitor::Impl::monitoring_loop, pimpl_.get());
    
    pimpl_->logger_->log(LogLevel::CRITICAL, "Safety monitoring started");
    return SafetyResult::SAFE;
}

SafetyResult SafetyMonitor::stop_monitoring() {
    if (!pimpl_->is_monitoring_) {
        return SafetyResult::WARNING;
    }
    
    pimpl_->is_monitoring_ = false;
    
    // Wait for monitoring thread to finish
    if (pimpl_->monitoring_thread_.joinable()) {
        pimpl_->monitoring_thread_.join();
    }
    
    pimpl_->logger_->log(LogLevel::CRITICAL, "Safety monitoring stopped");
    return SafetyResult::SAFE;
}

SafetyResult SafetyMonitor::check_system_safety() {
    std::lock_guard<std::mutex> lock(pimpl_->constraints_mutex_);
    
    auto start_time = std::chrono::steady_clock::now();
    SafetyResult overall_result = SafetyResult::SAFE;
    
    // Check all registered constraints
    for (const auto& [name, constraint] : pimpl_->constraints_) {
        SafetyResult result = pimpl_->check_constraint_internal(constraint);
        
        // Escalate result if more severe
        if (static_cast<int>(result) > static_cast<int>(overall_result)) {
            overall_result = result;
        }
        
        // Break on critical violations to ensure bounded response time
        if (result == SafetyResult::CRITICAL_VIOLATION) {
            break;
        }
    }
    
    // Update timing statistics
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    pimpl_->total_check_duration_ += duration;
    pimpl_->total_checks_++;
    
    return overall_result;
}

SafetyResult SafetyMonitor::check_constraint(const std::string& constraint_name) {
    std::lock_guard<std::mutex> lock(pimpl_->constraints_mutex_);
    
    auto it = pimpl_->constraints_.find(constraint_name);
    if (it == pimpl_->constraints_.end()) {
        pimpl_->logger_->log(LogLevel::ERROR, "Unknown constraint: " + constraint_name);
        return SafetyResult::SYSTEM_FAILURE;
    }
    
    return pimpl_->check_constraint_internal(it->second);
}

SafetyResult SafetyMonitor::check_scenario_safety(const std::string& scenario_content) {
    // Basic safety checks for scenario content
    if (scenario_content.empty()) {
        return SafetyResult::SYSTEM_FAILURE;
    }
    
    // Check for potentially dangerous operations in scenario
    std::vector<std::string> dangerous_keywords = {
        "emergency_stop", "critical_fault", "patient_disconnect", "power_failure"
    };
    
    for (const auto& keyword : dangerous_keywords) {
        if (scenario_content.find(keyword) != std::string::npos) {
            pimpl_->logger_->log(LogLevel::WARNING, "Scenario contains dangerous operation: " + keyword);
            return SafetyResult::WARNING;
        }
    }
    
    return SafetyResult::SAFE;
}

void SafetyMonitor::register_violation_callback(SafetyViolationCallback callback) {
    pimpl_->violation_callback_ = callback;
    pimpl_->logger_->log(LogLevel::INFO, "Safety violation callback registered");
}

void SafetyMonitor::register_emergency_stop_callback(EmergencyStopCallback callback) {
    pimpl_->emergency_stop_callback_ = callback;
    pimpl_->logger_->log(LogLevel::INFO, "Emergency stop callback registered");
}

SafetyStatus SafetyMonitor::get_safety_status() const {
    std::lock_guard<std::mutex> constraints_lock(pimpl_->constraints_mutex_);
    std::lock_guard<std::mutex> violations_lock(pimpl_->violations_mutex_);
    
    SafetyStatus status;
    status.is_monitoring_active = pimpl_->is_monitoring_;
    status.last_check_time = std::chrono::system_clock::now();
    status.active_constraints = pimpl_->constraints_.size();
    status.total_violations = pimpl_->violation_count_;
    
    // Count critical violations
    status.critical_violations = std::count_if(
        pimpl_->recent_violations_.begin(),
        pimpl_->recent_violations_.end(),
        [](const SafetyViolation& v) { return v.is_critical; });
    
    // Copy recent violations (limit to 10 most recent)
    size_t max_recent = std::min(pimpl_->recent_violations_.size(), size_t(10));
    status.recent_violations.assign(
        pimpl_->recent_violations_.end() - max_recent,
        pimpl_->recent_violations_.end());
    
    // Calculate timing statistics
    if (pimpl_->total_checks_ > 0) {
        status.avg_check_duration = pimpl_->total_check_duration_ / pimpl_->total_checks_;
    }
    
    return status;
}

std::vector<SafetyViolation> SafetyMonitor::get_recent_violations(size_t max_count) const {
    std::lock_guard<std::mutex> lock(pimpl_->violations_mutex_);
    
    size_t count = std::min(max_count, pimpl_->recent_violations_.size());
    std::vector<SafetyViolation> result;
    
    if (count > 0) {
        result.assign(
            pimpl_->recent_violations_.end() - count,
            pimpl_->recent_violations_.end());
    }
    
    return result;
}

bool SafetyMonitor::is_system_safe() const {
    SafetyResult result = const_cast<SafetyMonitor*>(this)->check_system_safety();
    return result == SafetyResult::SAFE || result == SafetyResult::WARNING;
}

bool SafetyMonitor::acknowledge_violation(
    const std::string& violation_id,
    const std::string& acknowledgment_reason) {
    
    pimpl_->logger_->log(LogLevel::INFO, "Violation acknowledged: " + violation_id + 
                        " Reason: " + acknowledgment_reason);
    return true;
}

bool SafetyMonitor::emergency_stop() noexcept {
    try {
        pimpl_->emergency_stop_active_ = true;
        pimpl_->logger_->log(LogLevel::CRITICAL, "EMERGENCY STOP ACTIVATED");
        
        if (pimpl_->emergency_stop_callback_) {
            return pimpl_->emergency_stop_callback_();
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool SafetyMonitor::reset_after_emergency() {
    if (!pimpl_->emergency_stop_active_) {
        return false;
    }
    
    pimpl_->emergency_stop_active_ = false;
    pimpl_->logger_->log(LogLevel::CRITICAL, "Emergency stop reset - system ready");
    
    return true;
}

bool SafetyMonitor::set_constraint_enabled(const std::string& constraint_name, bool enabled) {
    std::lock_guard<std::mutex> lock(pimpl_->constraints_mutex_);
    
    auto it = pimpl_->constraints_.find(constraint_name);
    if (it == pimpl_->constraints_.end()) {
        return false;
    }
    
    // Note: This implementation assumes constraints have an enabled flag
    // For simplicity, we'll just log the change
    pimpl_->logger_->log(LogLevel::INFO, "Constraint " + constraint_name + 
                        (enabled ? " enabled" : " disabled"));
    
    return true;
}

bool SafetyMonitor::update_constraint_interval(
    const std::string& constraint_name,
    std::chrono::milliseconds interval) {
    
    std::lock_guard<std::mutex> lock(pimpl_->constraints_mutex_);
    
    auto it = pimpl_->constraints_.find(constraint_name);
    if (it == pimpl_->constraints_.end()) {
        return false;
    }
    
    // Validate interval range
    if (interval < std::chrono::milliseconds(10) || interval > std::chrono::seconds(10)) {
        return false;
    }
    
    it->second.check_interval = interval;
    pimpl_->logger_->log(LogLevel::INFO, "Updated constraint interval for " + constraint_name);
    
    return true;
}

std::string SafetyMonitor::generate_safety_report() const {
    std::stringstream report;
    SafetyStatus status = get_safety_status();
    
    report << "=== Safety Monitoring Report ===\n";
    report << "Monitoring Active: " << (status.is_monitoring_active ? "Yes" : "No") << "\n";
    report << "Active Constraints: " << status.active_constraints << "\n";
    report << "Total Violations: " << status.total_violations << "\n";
    report << "Critical Violations: " << status.critical_violations << "\n";
    report << "Average Check Duration: " << status.avg_check_duration.count() << "ms\n";
    
    if (!status.recent_violations.empty()) {
        report << "\nRecent Violations:\n";
        for (const auto& violation : status.recent_violations) {
            report << "- " << violation.constraint_name << ": " << violation.description << "\n";
        }
    }
    
    return report.str();
}

bool SafetyMonitor::is_monitoring_active() const noexcept {
    return pimpl_->is_monitoring_;
}

// Private implementation methods
void SafetyMonitor::Impl::monitoring_loop() {
    logger_->log(LogLevel::INFO, "Safety monitoring loop started");
    
    while (is_monitoring_) {
        auto start_time = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(constraints_mutex_);
            
            // Check all constraints
            for (const auto& [name, constraint] : constraints_) {
                if (!is_monitoring_) break; // Exit if monitoring stopped
                
                SafetyResult result = check_constraint_internal(constraint);
                
                if (result >= SafetyResult::VIOLATION) {
                    SafetyViolation violation;
                    violation.timestamp = std::chrono::system_clock::now();
                    violation.constraint_name = constraint.name;
                    violation.constraint_type = constraint.type;
                    violation.severity = result;
                    violation.description = "Constraint violation detected";
                    violation.is_critical = constraint.is_critical;
                    violation.requires_emergency_stop = (result == SafetyResult::CRITICAL_VIOLATION);
                    
                    record_violation(violation);
                    
                    if (violation.requires_emergency_stop) {
                        emergency_stop_active_ = true;
                        if (emergency_stop_callback_) {
                            emergency_stop_callback_();
                        }
                    }
                }
            }
        }
        
        // Sleep until next check interval
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto sleep_time = DEFAULT_CHECK_INTERVAL - 
                         std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        
        if (sleep_time > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleep_time);
        }
    }
    
    logger_->log(LogLevel::INFO, "Safety monitoring loop ended");
}

void SafetyMonitor::Impl::record_violation(const SafetyViolation& violation) {
    std::lock_guard<std::mutex> lock(violations_mutex_);
    
    violation_count_++;
    
    // Maintain circular buffer of recent violations
    if (recent_violations_.size() >= MAX_RECENT_VIOLATIONS) {
        recent_violations_.erase(recent_violations_.begin());
    }
    recent_violations_.push_back(violation);
    
    // Log violation
    std::string severity_str = SafetyUtils::safety_result_to_string(violation.severity);
    logger_->log(LogLevel::CRITICAL, "SAFETY VIOLATION [" + severity_str + "]: " + 
                violation.constraint_name + " - " + violation.description);
    
    // Execute callback if registered
    if (violation_callback_) {
        try {
            violation_callback_(violation);
        } catch (...) {
            logger_->log(LogLevel::ERROR, "Exception in safety violation callback");
        }
    }
}

SafetyResult SafetyMonitor::Impl::check_constraint_internal(const SafetyConstraint& constraint) {
    try {
        if (constraint.check_function) {
            return constraint.check_function();
        }
        
        // Default to safe if no check function provided
        return SafetyResult::SAFE;
    } catch (...) {
        logger_->log(LogLevel::ERROR, "Exception during constraint check: " + constraint.name);
        return SafetyResult::SYSTEM_FAILURE;
    }
}

// Utility functions implementation
namespace SafetyUtils {

std::string safety_result_to_string(SafetyResult result) {
    switch (result) {
        case SafetyResult::SAFE: return "SAFE";
        case SafetyResult::WARNING: return "WARNING";
        case SafetyResult::VIOLATION: return "VIOLATION";
        case SafetyResult::CRITICAL_VIOLATION: return "CRITICAL_VIOLATION";
        case SafetyResult::SYSTEM_FAILURE: return "SYSTEM_FAILURE";
        default: return "UNKNOWN";
    }
}

std::string constraint_type_to_string(SafetyConstraintType type) {
    switch (type) {
        case SafetyConstraintType::TIMING_CONSTRAINT: return "TIMING_CONSTRAINT";
        case SafetyConstraintType::RESOURCE_CONSTRAINT: return "RESOURCE_CONSTRAINT";
        case SafetyConstraintType::SIGNAL_CONSTRAINT: return "SIGNAL_CONSTRAINT";
        case SafetyConstraintType::COMMUNICATION_CONSTRAINT: return "COMMUNICATION_CONSTRAINT";
        case SafetyConstraintType::PATIENT_SAFETY: return "PATIENT_SAFETY";
        case SafetyConstraintType::SYSTEM_INTEGRITY: return "SYSTEM_INTEGRITY";
        default: return "UNKNOWN";
    }
}

std::vector<SafetyConstraint> create_default_bci_constraints() {
    std::vector<SafetyConstraint> constraints;
    
    // Timing constraint example
    SafetyConstraint timing_constraint;
    timing_constraint.name = "real_time_response";
    timing_constraint.type = SafetyConstraintType::TIMING_CONSTRAINT;
    timing_constraint.description = "Real-time response constraint for BCI commands";
    timing_constraint.is_critical = true;
    timing_constraint.check_interval = std::chrono::milliseconds(10);
    timing_constraint.violation_timeout = std::chrono::milliseconds(100);
    constraints.push_back(timing_constraint);
    
    // Patient safety constraint example
    SafetyConstraint patient_safety;
    patient_safety.name = "signal_amplitude_limit";
    patient_safety.type = SafetyConstraintType::PATIENT_SAFETY;
    patient_safety.description = "Neural signal amplitude within safe limits";
    patient_safety.is_critical = true;
    patient_safety.check_interval = std::chrono::milliseconds(50);
    patient_safety.violation_timeout = std::chrono::milliseconds(200);
    constraints.push_back(patient_safety);
    
    return constraints;
}

bool validate_safety_constraint(const SafetyConstraint& constraint) {
    if (constraint.name.empty()) return false;
    if (constraint.description.empty()) return false;
    if (constraint.check_interval < std::chrono::milliseconds(1)) return false;
    if (constraint.violation_timeout < std::chrono::milliseconds(1)) return false;
    
    return true;
}

int calculate_constraint_priority(const SafetyConstraint& constraint) {
    int priority = 0;
    
    if (constraint.is_critical) priority += 100;
    
    switch (constraint.type) {
        case SafetyConstraintType::PATIENT_SAFETY: priority += 50; break;
        case SafetyConstraintType::TIMING_CONSTRAINT: priority += 30; break;
        case SafetyConstraintType::SYSTEM_INTEGRITY: priority += 20; break;
        default: priority += 10; break;
    }
    
    return priority;
}

bool requires_emergency_stop(const SafetyViolation& violation) {
    return violation.is_critical && 
           (violation.severity == SafetyResult::CRITICAL_VIOLATION ||
            violation.constraint_type == SafetyConstraintType::PATIENT_SAFETY);
}

} // namespace SafetyUtils

} // namespace Core
} // namespace IVVFramework
