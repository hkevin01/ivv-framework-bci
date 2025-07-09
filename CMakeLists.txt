cmake_minimum_required(VERSION 3.16)

# Project definition
project(IVVFrameworkBCI 
    VERSION 1.0.0
    DESCRIPTION "IV&V Framework for BCI Safety-Critical Systems"
    LANGUAGES CXX C
)

# Safety-critical build configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MISRA C++ compliance flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion -Wsign-conversion -Wcast-qual")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnull-dereference -Wdouble-promotion")

# Safety-critical optimization settings
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fno-strict-aliasing")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -DDEBUG")
endif()

# QNX RTOS support
option(BUILD_QNX_TARGET "Build for QNX RTOS target" OFF)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_STATIC_ANALYSIS "Enable static analysis" ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# QNX specific configuration
if(BUILD_QNX_TARGET)
    set(CMAKE_SYSTEM_NAME QNX)
    set(CMAKE_C_COMPILER qcc)
    set(CMAKE_CXX_COMPILER q++)
    add_definitions(-D__QNX__)
    add_definitions(-DSAFETY_CRITICAL_BUILD)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Testing framework
if(BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
        )
        FetchContent_MakeAvailable(googletest)
    endif()
    enable_testing()
endif()

# Core library components
set(CORE_SOURCES
    src/core/verifier.cpp
    src/core/config_manager.cpp
    src/core/logger.cpp
    src/core/safety_monitor.cpp
)

set(FAULT_INJECTION_SOURCES
    src/fault_injection/fault_injector.cpp
    src/fault_injection/timing_fault.cpp
    src/fault_injection/data_fault.cpp
    src/fault_injection/communication_fault.cpp
)

set(TIMING_ANALYSIS_SOURCES
    src/timing_analysis/timing_analyzer.cpp
    src/timing_analysis/deadline_monitor.cpp
    src/timing_analysis/latency_profiler.cpp
    src/timing_analysis/jitter_analyzer.cpp
)

set(REGRESSION_TESTING_SOURCES
    src/regression_testing/test_executor.cpp
    src/regression_testing/coverage_analyzer.cpp
    src/regression_testing/regression_detector.cpp
)

set(DSL_SOURCES
    src/dsl/dsl_parser.cpp
    src/dsl/scenario_generator.cpp
    src/dsl/ast_nodes.cpp
)

set(QNX_INTEGRATION_SOURCES
    src/qnx_integration/qnx_timer.cpp
    src/qnx_integration/qnx_ipc.cpp
    src/qnx_integration/qnx_scheduler.cpp
)

# Core framework library
add_library(ivv_core STATIC ${CORE_SOURCES})
target_include_directories(ivv_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)
target_compile_definitions(ivv_core PRIVATE IVV_CORE_EXPORTS)

# Fault injection library
add_library(ivv_fault_injection STATIC ${FAULT_INJECTION_SOURCES})
target_link_libraries(ivv_fault_injection PUBLIC ivv_core)
target_compile_definitions(ivv_fault_injection PRIVATE IVV_FAULT_INJECTION_EXPORTS)

# Timing analysis library
add_library(ivv_timing_analysis STATIC ${TIMING_ANALYSIS_SOURCES})
target_link_libraries(ivv_timing_analysis PUBLIC ivv_core)
target_compile_definitions(ivv_timing_analysis PRIVATE IVV_TIMING_ANALYSIS_EXPORTS)

# Regression testing library
add_library(ivv_regression_testing STATIC ${REGRESSION_TESTING_SOURCES})
target_link_libraries(ivv_regression_testing PUBLIC ivv_core)
target_compile_definitions(ivv_regression_testing PRIVATE IVV_REGRESSION_TESTING_EXPORTS)

# DSL library
add_library(ivv_dsl STATIC ${DSL_SOURCES})
target_link_libraries(ivv_dsl PUBLIC ivv_core)
target_compile_definitions(ivv_dsl PRIVATE IVV_DSL_EXPORTS)

# QNX integration library (conditional)
if(BUILD_QNX_TARGET)
    add_library(ivv_qnx_integration STATIC ${QNX_INTEGRATION_SOURCES})
    target_link_libraries(ivv_qnx_integration PUBLIC ivv_core)
    target_compile_definitions(ivv_qnx_integration PRIVATE IVV_QNX_INTEGRATION_EXPORTS)
endif()

# Main framework library (combines all components)
add_library(ivv_framework STATIC)
target_link_libraries(ivv_framework PUBLIC 
    ivv_core
    ivv_fault_injection
    ivv_timing_analysis
    ivv_regression_testing
    ivv_dsl
)

if(BUILD_QNX_TARGET)
    target_link_libraries(ivv_framework PUBLIC ivv_qnx_integration)
endif()

# Example applications
add_subdirectory(examples)

# Unit tests
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Documentation
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in 
                       ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
        add_custom_target(docs
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen" VERBATIM
        )
    endif()
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS ivv_framework ivv_core ivv_fault_injection ivv_timing_analysis 
                ivv_regression_testing ivv_dsl
    EXPORT IVVFrameworkTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(BUILD_QNX_TARGET)
    install(TARGETS ivv_qnx_integration
        EXPORT IVVFrameworkTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

# Install headers
install(DIRECTORY src/ 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ivv_framework
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    IVVFrameworkConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(EXPORT IVVFrameworkTargets
    FILE IVVFrameworkTargets.cmake
    NAMESPACE IVVFramework::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IVVFramework
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/IVVFrameworkConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/IVVFrameworkConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IVVFramework
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/IVVFrameworkConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/IVVFrameworkConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IVVFramework
)

# Static analysis
if(ENABLE_STATIC_ANALYSIS)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=*,-readability-*")
    endif()
    
    find_program(CPPCHECK_EXE NAMES "cppcheck")
    if(CPPCHECK_EXE)
        add_custom_target(cppcheck
            COMMAND ${CPPCHECK_EXE}
                --enable=all
                --std=c++17
                --verbose
                --quiet
                --project=${CMAKE_BINARY_DIR}/compile_commands.json
        )
    endif()
endif()

# Code coverage
option(ENABLE_COVERAGE "Enable code coverage analysis" OFF)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Print configuration summary
message(STATUS "IV&V Framework Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  QNX target: ${BUILD_QNX_TARGET}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "  Documentation: ${BUILD_DOCS}")
message(STATUS "  Static analysis: ${ENABLE_STATIC_ANALYSIS}")
message(STATUS "  Coverage: ${ENABLE_COVERAGE}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
