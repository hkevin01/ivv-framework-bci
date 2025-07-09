#include "src/fault_injection/fault_injector.h"
#include <iostream>

int main() {
  using namespace IVVFramework::FaultInjection;

  // Create fault injector
  auto injector = FaultInjector::create();

  if (!injector->initialize()) {
    std::cout << "Failed to initialize fault injector!" << std::endl;
    return 1;
  }

  std::cout << "Fault injector initialized successfully!" << std::endl;

  // Configure a test target
  FaultTarget target;
  target.component_name = "TestComponent";
  target.function_name = "test_function";
  target.is_critical_path = false;

  if (!injector->configure_target("test_target", target)) {
    std::cout << "Failed to configure target!" << std::endl;
    return 1;
  }

  std::cout << "Target configured successfully!" << std::endl;

  // Create a fault injection configuration
  FaultInjectionConfig config;
  config.fault_type = FaultType::TIMING_FAULT;
  config.target = target;
  config.timing_config.delay_injection = std::chrono::microseconds(100);
  config.max_system_impact = 0.1;

  // Inject a timing fault
  auto result = injector->inject_timing_fault(config);

  std::cout << "Fault injection result: ";
  switch (result.status) {
  case FaultInjectionResult::Status::SUCCESS:
    std::cout << "SUCCESS" << std::endl;
    break;
  case FaultInjectionResult::Status::FAILED:
    std::cout << "FAILED" << std::endl;
    break;
  case FaultInjectionResult::Status::BLOCKED_BY_SAFETY:
    std::cout << "BLOCKED_BY_SAFETY" << std::endl;
    break;
  default:
    std::cout << "OTHER" << std::endl;
  }

  std::cout << "Description: " << result.description << std::endl;
  std::cout << "Observed effects: " << result.observed_effects.size()
            << std::endl;

  // Get statistics
  auto stats = injector->get_statistics();
  std::cout << "Total fault injections performed: " << stats.size()
            << std::endl;

  return 0;
}
