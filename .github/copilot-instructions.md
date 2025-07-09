<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# IV&V Framework for BCI Safety-Critical Systems - Copilot Instructions

## Project Context
This is an Independent Verification & Validation (IV&V) framework specifically designed for Brain-Computer Interface (BCI) safety-critical systems. The project follows NASA IV&V methodologies adapted for medical device software validation.

## Architecture Guidelines
- **Core Language**: C++ for performance-critical verification engines
- **Scripting**: Python for test orchestration and analysis
- **Real-time**: QNX RTOS integration for safety-critical timing requirements
- **Safety Standards**: IEC 62304, ISO 13485, FDA Software Validation compliance

## Code Quality Standards
1. **Safety-Critical Code**: All code must follow MISRA C++ guidelines
2. **Testing**: Minimum 95% code coverage for safety-critical components
3. **Documentation**: Comprehensive inline documentation for all public APIs
4. **Error Handling**: Robust error handling with fail-safe defaults
5. **Real-time**: Deterministic execution for timing-critical paths

## Component Responsibilities
- `src/core/`: Framework foundation, configuration management, logging
- `src/fault_injection/`: Systematic fault insertion and propagation analysis
- `src/regression_testing/`: Automated verification of safety functions
- `src/timing_analysis/`: Real-time performance verification
- `src/dsl/`: Custom domain-specific language for test scenarios
- `src/qnx_integration/`: QNX RTOS specific implementations

## Safety Considerations
- Always use RAII for resource management
- Implement defensive programming practices
- Include safety assertions and invariant checks
- Use static analysis friendly code patterns
- Avoid dynamic memory allocation in real-time paths

## Naming Conventions
- **Namespaces**: `IVVFramework::<Component>` (e.g., `IVVFramework::FaultInjection`)
- **Classes**: PascalCase (e.g., `TimingAnalyzer`, `FaultInjector`)
- **Methods**: camelCase (e.g., `injectFault()`, `analyzeTimingConstraints()`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_INJECTION_RATE`)
- **Files**: snake_case (e.g., `timing_analyzer.cpp`, `fault_injector.h`)

## Testing Strategy
- Unit tests for all components using GoogleTest
- Integration tests with real QNX targets
- Performance benchmarks for timing-critical code
- Fault injection testing of the framework itself
- Regression tests against known BCI system behaviors

## Medical Device Compliance
- Maintain traceability from requirements to implementation
- Document all safety-related design decisions
- Include risk analysis for framework modifications
- Follow change control procedures for safety-critical code
- Ensure all external dependencies are validated

## When suggesting code:
1. Prioritize safety and reliability over performance
2. Include appropriate error handling and logging
3. Add timing constraints and real-time considerations
4. Suggest comprehensive test cases
5. Consider fault tolerance and graceful degradation
6. Include relevant safety assertions and preconditions
