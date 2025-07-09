#!/bin/bash

# IV&V Framework Build Script for BCI Safety-Critical Systems
# This script handles building the framework for different targets and configurations

set -euo pipefail

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default configuration
BUILD_TYPE="Debug"
TARGET="native"
BUILD_TESTS="ON"
BUILD_DOCS="OFF"
ENABLE_COVERAGE="OFF"
CLEAN_BUILD="false"
VERBOSE="false"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Help message
show_help() {
    cat << EOF
IV&V Framework Build Script

Usage: $0 [OPTIONS]

OPTIONS:
    -t, --target TARGET        Build target: native, qnx (default: native)
    -b, --build-type TYPE      Build type: Debug, Release, RelWithDebInfo (default: Debug)
    -c, --clean                Clean build directory before building
    --no-tests                 Disable building tests
    --with-docs                Enable documentation generation
    --with-coverage            Enable code coverage analysis
    -v, --verbose              Verbose output
    -h, --help                 Show this help message

EXAMPLES:
    $0                         Build debug version for native target
    $0 -t qnx -b Release       Build release version for QNX target
    $0 --clean --with-coverage Build with coverage analysis
    $0 --with-docs             Build with documentation

SAFETY NOTICE:
    This framework is designed for safety-critical BCI systems.
    Always use appropriate safety protocols and follow regulatory
    requirements when deploying to actual medical devices.
EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -t|--target)
                TARGET="$2"
                shift 2
                ;;
            -b|--build-type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            -c|--clean)
                CLEAN_BUILD="true"
                shift
                ;;
            --no-tests)
                BUILD_TESTS="OFF"
                shift
                ;;
            --with-docs)
                BUILD_DOCS="ON"
                shift
                ;;
            --with-coverage)
                ENABLE_COVERAGE="ON"
                shift
                ;;
            -v|--verbose)
                VERBOSE="true"
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# Validate target
validate_target() {
    case $TARGET in
        native|qnx)
            ;;
        *)
            log_error "Invalid target: $TARGET. Supported targets: native, qnx"
            exit 1
            ;;
    esac
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake is required but not installed"
        exit 1
    fi
    
    local cmake_version
    cmake_version=$(cmake --version | head -n1 | cut -d' ' -f3)
    log_info "Found CMake version: $cmake_version"
    
    # Check compiler based on target
    if [[ $TARGET == "qnx" ]]; then
        if ! command -v qcc &> /dev/null; then
            log_error "QNX compiler (qcc) is required for QNX target"
            exit 1
        fi
        log_info "Found QNX compiler"
    else
        if command -v g++ &> /dev/null; then
            local gcc_version
            gcc_version=$(g++ --version | head -n1)
            log_info "Found GCC: $gcc_version"
        elif command -v clang++ &> /dev/null; then
            local clang_version
            clang_version=$(clang++ --version | head -n1)
            log_info "Found Clang: $clang_version"
        else
            log_error "No suitable C++ compiler found"
            exit 1
        fi
    fi
    
    # Check Python for scripts
    if command -v python3 &> /dev/null; then
        local python_version
        python_version=$(python3 --version)
        log_info "Found Python: $python_version"
    else
        log_warn "Python3 not found - some scripts may not work"
    fi
}

# Setup build directory
setup_build_dir() {
    local build_dir="$PROJECT_ROOT/build"
    
    if [[ $CLEAN_BUILD == "true" ]] && [[ -d $build_dir ]]; then
        log_info "Cleaning build directory..."
        rm -rf "$build_dir"
    fi
    
    if [[ ! -d $build_dir ]]; then
        log_info "Creating build directory..."
        mkdir -p "$build_dir"
    fi
    
    echo "$build_dir"
}

# Configure CMake
configure_cmake() {
    local build_dir=$1
    local cmake_args=()
    
    log_info "Configuring CMake for $TARGET target ($BUILD_TYPE)..."
    
    # Basic configuration
    cmake_args+=("-DCMAKE_BUILD_TYPE=$BUILD_TYPE")
    cmake_args+=("-DBUILD_TESTS=$BUILD_TESTS")
    cmake_args+=("-DBUILD_DOCS=$BUILD_DOCS")
    cmake_args+=("-DENABLE_COVERAGE=$ENABLE_COVERAGE")
    
    # Target-specific configuration
    if [[ $TARGET == "qnx" ]]; then
        cmake_args+=("-DBUILD_QNX_TARGET=ON")
        if [[ -f "$PROJECT_ROOT/cmake/qnx.cmake" ]]; then
            cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/cmake/qnx.cmake")
        else
            log_warn "QNX toolchain file not found, using default configuration"
        fi
    fi
    
    # Safety-critical build flags
    cmake_args+=("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
    cmake_args+=("-DENABLE_STATIC_ANALYSIS=ON")
    
    # Configure
    cd "$build_dir"
    if [[ $VERBOSE == "true" ]]; then
        cmake "${cmake_args[@]}" "$PROJECT_ROOT"
    else
        cmake "${cmake_args[@]}" "$PROJECT_ROOT" > cmake_config.log 2>&1
    fi
}

# Build project
build_project() {
    local build_dir=$1
    local cores
    cores=$(nproc 2>/dev/null || echo 4)
    
    log_info "Building project using $cores cores..."
    
    cd "$build_dir"
    if [[ $VERBOSE == "true" ]]; then
        make -j"$cores"
    else
        make -j"$cores" > build.log 2>&1
    fi
}

# Run tests
run_tests() {
    local build_dir=$1
    
    if [[ $BUILD_TESTS == "ON" ]]; then
        log_info "Running tests..."
        cd "$build_dir"
        if [[ $VERBOSE == "true" ]]; then
            ctest --output-on-failure
        else
            ctest --output-on-failure > test.log 2>&1
        fi
    fi
}

# Generate documentation
generate_docs() {
    local build_dir=$1
    
    if [[ $BUILD_DOCS == "ON" ]]; then
        log_info "Generating documentation..."
        cd "$build_dir"
        if [[ $VERBOSE == "true" ]]; then
            make docs
        else
            make docs > docs.log 2>&1
        fi
    fi
}

# Generate coverage report
generate_coverage() {
    local build_dir=$1
    
    if [[ $ENABLE_COVERAGE == "ON" ]]; then
        log_info "Generating coverage report..."
        cd "$build_dir"
        
        if command -v gcov &> /dev/null && command -v lcov &> /dev/null; then
            lcov --capture --directory . --output-file coverage.info
            lcov --remove coverage.info '/usr/*' '*/tests/*' '*/examples/*' --output-file coverage_filtered.info
            genhtml coverage_filtered.info --output-directory coverage_html
            log_info "Coverage report generated in coverage_html/"
        else
            log_warn "gcov/lcov not found - skipping coverage report generation"
        fi
    fi
}

# Main build function
main() {
    parse_args "$@"
    validate_target
    
    log_info "Starting IV&V Framework build..."
    log_info "Target: $TARGET"
    log_info "Build Type: $BUILD_TYPE"
    log_info "Tests: $BUILD_TESTS"
    log_info "Documentation: $BUILD_DOCS"
    log_info "Coverage: $ENABLE_COVERAGE"
    
    check_prerequisites
    
    local build_dir
    build_dir=$(setup_build_dir)
    
    configure_cmake "$build_dir"
    build_project "$build_dir"
    run_tests "$build_dir"
    generate_docs "$build_dir"
    generate_coverage "$build_dir"
    
    log_success "Build completed successfully!"
    log_info "Build artifacts are in: $build_dir"
    
    if [[ $BUILD_TESTS == "ON" ]]; then
        log_info "Test results are available in: $build_dir/Testing/"
    fi
    
    if [[ $BUILD_DOCS == "ON" ]]; then
        log_info "Documentation is available in: $build_dir/docs/"
    fi
    
    if [[ $ENABLE_COVERAGE == "ON" ]]; then
        log_info "Coverage report is available in: $build_dir/coverage_html/"
    fi
}

# Run main function with all arguments
main "$@"
