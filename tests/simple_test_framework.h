/**
 * @file simple_test_framework.h
 * @brief Simple test framework header for IV&V Framework
 *
 * Basic test framework for when GoogleTest is not available.
 *
 * @author IV&V Framework Team
 * @version 1.0.0
 * @date 2025-07-09
 */

#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace SimpleTest {

struct TestCase {
  std::string name;
  std::function<void()> test_func;
};

class TestRunner {
private:
  std::vector<TestCase> tests_;
  int passed_ = 0;
  int failed_ = 0;

public:
  void add_test(const std::string &name, std::function<void()> test_func);
  void run_all();
  int get_exit_code() const;
};

// Simple assertion macros
#define ASSERT_TRUE(condition)                                                 \
  if (!(condition)) {                                                          \
    throw std::runtime_error("Assertion failed: " #condition);                 \
  }

#define ASSERT_FALSE(condition)                                                \
  if (condition) {                                                             \
    throw std::runtime_error("Assertion failed: " #condition                   \
                             " should be false");                              \
  }

#define ASSERT_EQ(expected, actual)                                            \
  if ((expected) != (actual)) {                                                \
    throw std::runtime_error("Assertion failed: expected " #expected           \
                             " == " #actual);                                  \
  }

#define ASSERT_NE(expected, actual)                                            \
  if ((expected) == (actual)) {                                                \
    throw std::runtime_error("Assertion failed: expected " #expected           \
                             " != " #actual);                                  \
  }

} // namespace SimpleTest
