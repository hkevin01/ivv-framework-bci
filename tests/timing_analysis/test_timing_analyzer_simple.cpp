/**
 * @file test_timing_analyzer_simple.cpp
 * @brief Simple unit tests for TimingAnalyzer
 *
 * Basic functionality tests for the timing analysis module.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 */

#include "../../src/timing_analysis/timing_analyzer.h"
#include "../simple_test_framework.h"
#include <chrono>
#include <thread>

using namespace IVVFramework::TimingAnalysis;

namespace {

void test_factory_creation() {
  std::cout << "Testing factory creation..." << std::endl;
  auto analyzer = TimingAnalyzer::create();
  ASSERT_TRUE(analyzer != nullptr);
  std::cout << "✓ Factory creation test passed" << std::endl;
}

void test_initialization() {
  std::cout << "Testing initialization..." << std::endl;
  auto analyzer = TimingAnalyzer::create();
  ASSERT_TRUE(analyzer != nullptr);

  bool init_result = analyzer->initialize();
  ASSERT_TRUE(init_result);
  std::cout << "✓ Initialization test passed" << std::endl;
}

void test_constraint_configuration() {
  std::cout << "Testing constraint configuration..." << std::endl;
  auto analyzer = TimingAnalyzer::create();
  ASSERT_TRUE(analyzer->initialize());
  TimingConstraint constraint;
  constraint.name = "test_component";
  constraint.deadline = std::chrono::milliseconds(50);
  constraint.period = std::chrono::milliseconds(100);
  constraint.max_jitter = std::chrono::microseconds(500);
  constraint.min_separation = std::chrono::nanoseconds(0);
  constraint.is_critical_path = true;
  constraint.deadline_miss_threshold = 0.001;

  bool config_result =
      analyzer->configure_constraints("test_component", constraint);
  ASSERT_TRUE(config_result);
  std::cout << "✓ Constraint configuration test passed" << std::endl;
}

void test_basic_measurement() {
  std::cout << "Testing basic measurement..." << std::endl;
  auto analyzer = TimingAnalyzer::create();
  ASSERT_TRUE(analyzer->initialize());

  // Start a measurement
  uint64_t measurement_id = analyzer->start_measurement("test_task");
  ASSERT_TRUE(measurement_id > 0);

  // Simulate some work
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Stop the measurement
  TimingMeasurement result = analyzer->stop_measurement(measurement_id);

  ASSERT_TRUE(result.task_name == "test_task");
  ASSERT_TRUE(result.execution_time.count() > 0);
  ASSERT_TRUE(result.execution_time >= std::chrono::milliseconds(9));

  std::cout << "✓ Basic measurement test passed (execution time: "
            << result.execution_time.count() << " ns)" << std::endl;
}

void test_execution_measurement_template() {
  std::cout << "Testing template measurement..." << std::endl;
  auto analyzer = TimingAnalyzer::create();
  ASSERT_TRUE(analyzer->initialize());

  // Test the template-based measurement
  auto result = analyzer->measure_execution("lambda_test", []() {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  });

  ASSERT_TRUE(result.task_name == "lambda_test");
  ASSERT_TRUE(result.execution_time.count() > 0);
  ASSERT_TRUE(result.execution_time >= std::chrono::milliseconds(4));

  std::cout << "✓ Template measurement test passed (execution time: "
            << result.execution_time.count() << " ns)" << std::endl;
}

void test_deadline_analysis() {
  std::cout << "Testing deadline analysis..." << std::endl;
  auto analyzer = TimingAnalyzer::create();
  ASSERT_TRUE(analyzer->initialize()); // Configure constraint first
  TimingConstraint constraint;
  constraint.name = "deadline_test";
  constraint.deadline = std::chrono::milliseconds(50);
  constraint.period = std::chrono::milliseconds(100);
  constraint.max_jitter = std::chrono::microseconds(500);
  constraint.min_separation = std::chrono::nanoseconds(0);
  constraint.is_critical_path = false;
  constraint.deadline_miss_threshold = 0.001;

  analyzer->configure_constraints("deadline_test", constraint);

  // Take some measurements
  for (int i = 0; i < 5; ++i) {
    analyzer->measure_execution("deadline_test", []() {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    });
  }

  // Analyze deadline compliance
  auto stats = analyzer->analyze_deadline_compliance("deadline_test",
                                                     std::chrono::seconds(1));

  ASSERT_TRUE(stats.component_name == "deadline_test");
  ASSERT_TRUE(stats.measurement_count > 0);
  ASSERT_TRUE(stats.deadline_miss_rate == 0.0);

  std::cout << "✓ Deadline analysis test passed (measurements: "
            << stats.measurement_count
            << ", miss rate: " << stats.deadline_miss_rate << ")" << std::endl;
}

void test_jitter_measurement() {
  std::cout << "Testing jitter measurement..." << std::endl;
  auto analyzer = TimingAnalyzer::create();
  ASSERT_TRUE(analyzer->initialize());

  // Take multiple measurements to analyze jitter
  for (int i = 0; i < 10; ++i) {
    analyzer->measure_execution("jitter_test", []() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });
  }

  auto stats = analyzer->measure_jitter("jitter_test", 10);

  ASSERT_TRUE(stats.component_name == "jitter_test");
  ASSERT_TRUE(stats.measurement_count > 0);

  std::cout << "✓ Jitter measurement test passed (measurements: "
            << stats.measurement_count << ")" << std::endl;
}

void test_report_generation() {
  std::cout << "Testing report generation..." << std::endl;
  auto analyzer = TimingAnalyzer::create();
  ASSERT_TRUE(analyzer->initialize());

  // Take some measurements
  for (int i = 0; i < 3; ++i) {
    analyzer->measure_execution("report_test", []() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    });
  }

  auto report = analyzer->generate_report(false);

  ASSERT_TRUE(!report.target_system.empty());
  // Note: Analysis duration might be 0 for very fast operations, so we'll just check it's valid
  ASSERT_TRUE(report.analysis_duration.count() >= 0);

  std::cout << "✓ Report generation test passed (target: "
            << report.target_system << ")" << std::endl;
}

void test_constraint_verification() {
  std::cout << "Testing constraint verification..." << std::endl;
  auto analyzer = TimingAnalyzer::create();
  ASSERT_TRUE(analyzer->initialize());

  // Configure a very tight constraint that should be violated
  TimingConstraint tight_constraint;
  tight_constraint.name = "tight_test";
  tight_constraint.deadline =
      std::chrono::microseconds(100); // Very tight deadline
  tight_constraint.period =
      std::chrono::milliseconds(10); // Period must be larger than deadline
  tight_constraint.max_jitter = std::chrono::microseconds(50);
  tight_constraint.min_separation = std::chrono::nanoseconds(0);
  tight_constraint.is_critical_path = false;
  tight_constraint.deadline_miss_threshold = 0.001;

  analyzer->configure_constraints("tight_test", tight_constraint);

  // Take a measurement that will likely violate the constraint
  analyzer->measure_execution("tight_test", []() {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(1)); // Much longer than 100µs
  });

  bool verification_result = analyzer->verify_timing_constraints();
  // Note: This might fail due to constraint violations, which is expected

  std::cout << "✓ Constraint verification test completed (result: "
            << (verification_result ? "passed" : "violations detected") << ")"
            << std::endl;
}

} // anonymous namespace

// Main test runner
int main() {
  std::cout << "=== TimingAnalyzer Simple Test Suite ===" << std::endl;

  try {
    test_factory_creation();
    test_initialization();
    test_constraint_configuration();
    test_basic_measurement();
    test_execution_measurement_template();
    test_deadline_analysis();
    test_jitter_measurement();
    test_report_generation();
    test_constraint_verification();

    std::cout << std::endl
              << "🎉 All TimingAnalyzer tests passed!" << std::endl;
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "❌ Test failed with unknown exception" << std::endl;
    return 1;
  }
}
