cmake_minimum_required(VERSION 3.16)

# Test configuration
enable_testing()

# Always use simple test runner for now
message(STATUS "Using simple test framework")

# Create simple test runner
add_executable(simple_test_runner
    simple_test_runner.cpp
    fault_injection/test_fault_injector_simple.cpp
)

# Add timing analysis test if available
if(EXISTS "${CMAKE_SOURCE_DIR}/src/timing_analysis")
    add_executable(timing_analysis_test
        timing_analysis/test_timing_analyzer_simple.cpp
    )
    
    target_include_directories(timing_analysis_test
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    
    target_link_libraries(timing_analysis_test
        ivv_framework
        Threads::Threads
    )
    
    add_test(NAME TimingAnalysisTests COMMAND timing_analysis_test)
    message(STATUS "Adding timing analysis tests")
endif()

target_include_directories(simple_test_runner
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(simple_test_runner
    ivv_framework
    Threads::Threads
)

add_test(NAME SimpleTests COMMAND simple_test_runner)
