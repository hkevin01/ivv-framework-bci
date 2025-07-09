# IV&V Framework Copilot Configuration

## Overview
This directory contains configuration files for GitHub Copilot to enhance
code generation quality for the IV&V Framework BCI Safety-Critical Systems project.

## Files

### copilot-instructions.md
Contains workspace-specific instructions for Copilot to generate safer,
more compliant code for safety-critical BCI systems.

### Project-Specific Guidelines
- All code must follow MISRA C++ guidelines
- Safety-critical functions require comprehensive error handling
- Real-time constraints must be considered
- Medical device compliance (IEC 62304, ISO 13485, FDA) requirements
- QNX RTOS integration best practices

## Safety Notices

⚠️ **CRITICAL SAFETY NOTICE** ⚠️

This framework is designed for safety-critical BCI systems. When using
Copilot suggestions:

1. **Always review generated code** for safety implications
2. **Verify MISRA compliance** before committing
3. **Add appropriate safety assertions** and error handling
4. **Consider real-time constraints** in timing-critical paths
5. **Document safety-related design decisions**

## Code Quality Standards

- **MISRA C++**: All code must be MISRA C++ compliant
- **Coverage**: Minimum 95% code coverage for safety-critical components
- **Testing**: Comprehensive unit and integration tests
- **Documentation**: Detailed API documentation with safety considerations
- **Static Analysis**: Regular static analysis with safety-focused tools

## Usage

When working with Copilot in this project:

1. Copilot will automatically use the instructions in `copilot-instructions.md`
2. Generated code will prioritize safety and reliability
3. Suggestions will include appropriate error handling
4. Real-time and safety constraints will be considered
5. Medical device compliance patterns will be preferred

## Customization

To customize Copilot behavior for specific components:

1. Update `copilot-instructions.md` with component-specific guidelines
2. Add safety constraints relevant to your BCI system
3. Include regulatory requirements specific to your region
4. Specify real-time constraints for your target platform

## Compliance

This configuration helps ensure generated code meets:

- **IEC 62304**: Medical device software lifecycle
- **ISO 13485**: Quality management for medical devices
- **FDA Software Validation**: FDA guidance compliance
- **MISRA C++**: Safety-critical C++ coding standards
- **DO-178C**: Software considerations (adapted for medical devices)
