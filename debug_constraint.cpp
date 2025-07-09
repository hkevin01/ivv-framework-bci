/**
 * @brief Debug test for timing constraint validation
 */

#include "src/timing_analysis/timing_analyzer.h"
#include <chrono>
#include <iostream>

using namespace IVVFramework::TimingAnalysis;

int main() {
  std::cout << "=== Debug Timing Constraint Validation ===" << std::endl;    // Create constraint exactly like the test
    TimingConstraint constraint;
    constraint.name = "test_component";
    constraint.deadline = std::chrono::milliseconds(50);
    constraint.period = std::chrono::milliseconds(100);
    constraint.max_jitter = std::chrono::microseconds(500);
    constraint.min_separation = std::chrono::nanoseconds(0);
    constraint.is_critical_path = true;
    constraint.deadline_miss_threshold = 0.001;

  std::cout << "Constraint details:" << std::endl;
  std::cout << "  Name: " << constraint.name << std::endl;
  std::cout << "  Deadline: " << constraint.deadline.count() << " ns"
            << std::endl;
  std::cout << "  Period: " << constraint.period.count() << " ns" << std::endl;
  std::cout << "  Max Jitter: " << constraint.max_jitter.count() << " ns"
            << std::endl;
  std::cout << "  Min Separation: " << constraint.min_separation.count()
            << " ns" << std::endl;
  std::cout << "  Critical Path: "
            << (constraint.is_critical_path ? "true" : "false") << std::endl;
  std::cout << "  Miss Threshold: " << constraint.deadline_miss_threshold
            << std::endl;

  auto analyzer = TimingAnalyzer::create();
  if (!analyzer) {
    std::cout << "❌ Failed to create analyzer" << std::endl;
    return 1;
  }

  if (!analyzer->initialize()) {
    std::cout << "❌ Failed to initialize analyzer" << std::endl;
    return 1;
  }    bool result = analyzer->configure_constraints("test_component", constraint);
  std::cout << "Configure result: " << (result ? "SUCCESS" : "FAILED")
            << std::endl;

  return result ? 0 : 1;
}
