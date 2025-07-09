/**
 * @file simple_test_runner.cpp
 * @brief Simple test runner for IV&V Framework
 *
 * Basic test framework for when GoogleTest is not available.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 */

#include "simple_test_framework.h"

namespace SimpleTest {

void TestRunner::add_test(const std::string &name,
                          std::function<void()> test_func) {
  tests_.push_back({name, std::move(test_func)});
}

void TestRunner::run_all() {
  std::cout << "Running " << tests_.size() << " tests...\n\n";

  for (const auto &test : tests_) {
    try {
      std::cout << "[ RUN      ] " << test.name << std::endl;
      test.test_func();
      std::cout << "[       OK ] " << test.name << std::endl;
      passed_++;
    } catch (const std::exception &e) {
      std::cout << "[  FAILED  ] " << test.name << " - " << e.what()
                << std::endl;
      failed_++;
    } catch (...) {
      std::cout << "[  FAILED  ] " << test.name << " - Unknown exception"
                << std::endl;
      failed_++;
    }
  }

  std::cout << "\n[==========] " << tests_.size() << " test(s) ran."
            << std::endl;
  std::cout << "[  PASSED  ] " << passed_ << " test(s)." << std::endl;
  if (failed_ > 0) {
    std::cout << "[  FAILED  ] " << failed_ << " test(s)." << std::endl;
  }
}

int TestRunner::get_exit_code() const { return failed_ > 0 ? 1 : 0; }

} // namespace SimpleTest

// External test function declarations
extern void register_fault_injection_tests(SimpleTest::TestRunner &runner);

int main() {
  SimpleTest::TestRunner runner;

  // Register all test modules
  register_fault_injection_tests(runner);

  // Run tests
  runner.run_all();

  return runner.get_exit_code();
}
