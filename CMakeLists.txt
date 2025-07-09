cmake_minimum_required(VERSION 3.16)

# Project de# Core framework sources (Phase 1)
set(CORE_SOURCES
    src/core/verifier.cpp
    src/core/config_manager.cpp
    src/core/logger.cpp
    src/core/safety_monitor.cpp
    src/qnx_integration/qnx_platform.cpp
)

# Fault injection sources (Phase 2)
set(FAULT_INJECTION_SOURCES
    src/fault_injection/fault_injector.cpp
)

# Timing analysis sources (Phase 3)
set(TIMING_ANALYSIS_SOURCES
    src/timing_analysis/timing_analyzer.cpp
)

# Check if fault injection directory exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/fault_injection")
    list(APPEND CORE_SOURCES ${FAULT_INJECTION_SOURCES})
    message(STATUS "Including fault injection module")
endif()

# Check if timing analysis directory exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/timing_analysis")
    list(APPEND CORE_SOURCES ${TIMING_ANALYSIS_SOURCES})
    message(STATUS "Including timing analysis module")
endif()

# Core framework headers
set(CORE_HEADERS
    src/core/verifier.h
    src/core/config_manager.h
    src/core/logger.h
    src/core/safety_monitor.h
    src/qnx_integration/qnx_platform.h
)

# Add fault injection headers if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/fault_injection")
    list(APPEND CORE_HEADERS src/fault_injection/fault_injector.h)
endif()

# Add timing analysis headers if available  
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/timing_analysis")
    list(APPEND CORE_HEADERS src/timing_analysis/timing_analyzer.h)
endif()

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
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

# Add fault injection sources and headers if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/fault_injection")
    list(APPEND CORE_SOURCES ${FAULT_INJECTION_SOURCES})
    list(APPEND CORE_HEADERS src/fault_injection/fault_injector.h)
    message(STATUS "Fault injection module added to build")
endif()

# Add timing analysis sources and headers if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/timing_analysis")
    list(APPEND CORE_SOURCES ${TIMING_ANALYSIS_SOURCES})
    list(APPEND CORE_HEADERS src/timing_analysis/timing_analyzer.h)
    message(STATUS "Timing analysis module added to build")
endif()

# Create core framework library
add_library(ivv_core_framework STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

# Set target properties for core framework
target_link_libraries(ivv_core_framework 
    PRIVATE 
    Threads::Threads
)

target_include_directories(ivv_core_framework
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Main framework library (Phase 1 - Core only)
add_library(ivv_framework STATIC
    ${CORE_SOURCES}
)

target_link_libraries(ivv_framework 
    PUBLIC 
    Threads::Threads
)

target_include_directories(ivv_framework
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Example applications (if directory exists)
if(EXISTS "${CMAKE_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif()

# Unit tests (if directory exists and tests enabled)
if(BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests")
    enable_testing()
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
    endif(DOXYGEN_FOUND)
endif()

# Installation configuration
include(GNUInstallDirs)

# Install targets
install(TARGETS ivv_framework ivv_core_framework
    EXPORT IVVFrameworkTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(FILES ${CORE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ivv_framework
)

# Install fault injection headers if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/fault_injection")
    install(FILES src/fault_injection/fault_injector.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ivv_framework/fault_injection
    )
endif()

# Install timing analysis headers if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/timing_analysis")
    install(FILES src/timing_analysis/timing_analyzer.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ivv_framework/timing_analysis
    )
endif()

# Export configuration
install(EXPORT IVVFrameworkTargets
    FILE IVVFrameworkTargets.cmake
    NAMESPACE IVVFramework::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IVVFramework
)

# Create config files
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/IVVFrameworkConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/IVVFrameworkConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IVVFramework
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/IVVFrameworkConfigVersion.cmake"
    VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/IVVFrameworkConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/IVVFrameworkConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/IVVFramework
)

# Display configuration summary
message(STATUS "IV&V Framework Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  QNX target: ${BUILD_QNX_TARGET}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "  Documentation: ${BUILD_DOCS}")
message(STATUS "  Static analysis: ${ENABLE_STATIC_ANALYSIS}")
message(STATUS "  Coverage: ${ENABLE_CODE_COVERAGE}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
