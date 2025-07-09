#!/usr/bin/env python3
"""
IV&V Framework Test Orchestrator for BCI Safety-Critical Systems

This script orchestrates the execution of verification and validation tests
for BCI systems, integrating fault injection, timing analysis, and regression testing.

Author: IV&V Framework Team
Version: 1.0.0
Date: 2025-07-09

Safety Notice:
This test orchestrator is designed for controlled testing environments only.
Do not run against production BCI systems connected to patients.
"""

import argparse
import asyncio
import json
import logging
import os
import subprocess
import sys
import time
import traceback
from dataclasses import asdict, dataclass
from enum import Enum
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import yaml


class TestResult(Enum):
    """Test execution results"""
    PASS = "PASS"
    FAIL = "FAIL"
    TIMEOUT = "TIMEOUT"
    SAFETY_VIOLATION = "SAFETY_VIOLATION"
    SKIPPED = "SKIPPED"


class TestSeverity(Enum):
    """Test severity levels for safety-critical systems"""
    CRITICAL = "CRITICAL"      # Safety-critical functionality
    HIGH = "HIGH"             # Important functionality
    MEDIUM = "MEDIUM"         # Standard functionality
    LOW = "LOW"               # Nice-to-have functionality


@dataclass
class TestCase:
    """Individual test case definition"""
    name: str
    description: str
    test_type: str  # 'fault_injection', 'timing_analysis', 'regression'
    severity: TestSeverity
    dsl_scenario: Optional[str] = None
    config_file: Optional[str] = None
    timeout_seconds: int = 300
    safety_critical: bool = False
    expected_result: TestResult = TestResult.PASS
    prerequisites: List[str] = None
    
    def __post_init__(self):
        if self.prerequisites is None:
            self.prerequisites = []


@dataclass
class TestExecution:
    """Test execution result"""
    test_case: TestCase
    result: TestResult
    start_time: float
    end_time: float
    duration_seconds: float
    error_message: Optional[str] = None
    safety_violations: List[str] = None
    output_files: List[str] = None
    
    def __post_init__(self):
        if self.safety_violations is None:
            self.safety_violations = []
        if self.output_files is None:
            self.output_files = []


@dataclass
class TestSuite:
    """Collection of related test cases"""
    name: str
    description: str
    test_cases: List[TestCase]
    setup_script: Optional[str] = None
    teardown_script: Optional[str] = None


class SafetyMonitor:
    """Safety monitoring for test execution"""
    
    def __init__(self):
        self.safety_violations = []
        self.critical_thresholds = {
            'max_fault_injection_rate': 0.1,  # 10% max fault injection rate
            'max_timing_deviation': 1000,     # 1000ms max timing deviation
            'max_test_duration': 3600,        # 1 hour max test duration
        }
    
    def check_safety_constraints(self, test_case: TestCase) -> Tuple[bool, List[str]]:
        """Check if test case violates safety constraints"""
        violations = []
        
        # Check if safety-critical test has appropriate safeguards
        if test_case.safety_critical and test_case.test_type == 'fault_injection':
            if 'safety_check' not in test_case.description.lower():
                violations.append(f"Safety-critical fault injection test {test_case.name} lacks safety checks")
        
        # Check timeout constraints
        if test_case.timeout_seconds > self.critical_thresholds['max_test_duration']:
            violations.append(f"Test {test_case.name} timeout exceeds safety threshold")
        
        return len(violations) == 0, violations
    
    def log_safety_violation(self, violation: str):
        """Log a safety violation"""
        self.safety_violations.append(violation)
        logging.critical(f"SAFETY VIOLATION: {violation}")


class TestOrchestrator:
    """Main test orchestrator for IV&V Framework"""
    
    def __init__(self, framework_path: str, output_dir: str):
        self.framework_path = Path(framework_path)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.safety_monitor = SafetyMonitor()
        self.test_results: List[TestExecution] = []
        
        # Setup logging
        self._setup_logging()
        
    def _setup_logging(self):
        """Setup logging configuration"""
        log_file = self.output_dir / "test_execution.log"
        
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler(log_file),
                logging.StreamHandler(sys.stdout)
            ]
        )
        
        # Separate safety log
        safety_log = self.output_dir / "safety_violations.log"
        safety_handler = logging.FileHandler(safety_log)
        safety_handler.setLevel(logging.CRITICAL)
        safety_formatter = logging.Formatter('%(asctime)s - SAFETY - %(message)s')
        safety_handler.setFormatter(safety_formatter)
        
        safety_logger = logging.getLogger('safety')
        safety_logger.addHandler(safety_handler)
        safety_logger.setLevel(logging.CRITICAL)
    
    async def load_test_suite(self, suite_file: str) -> TestSuite:
        """Load test suite from configuration file"""
        suite_path = Path(suite_file)
        
        if not suite_path.exists():
            raise FileNotFoundError(f"Test suite file not found: {suite_file}")
        
        with open(suite_path, 'r') as f:
            if suite_path.suffix.lower() == '.yaml' or suite_path.suffix.lower() == '.yml':
                suite_data = yaml.safe_load(f)
            else:
                suite_data = json.load(f)
        
        # Convert to TestSuite object
        test_cases = []
        for tc_data in suite_data.get('test_cases', []):
            test_case = TestCase(
                name=tc_data['name'],
                description=tc_data['description'],
                test_type=tc_data['test_type'],
                severity=TestSeverity(tc_data.get('severity', 'MEDIUM')),
                dsl_scenario=tc_data.get('dsl_scenario'),
                config_file=tc_data.get('config_file'),
                timeout_seconds=tc_data.get('timeout_seconds', 300),
                safety_critical=tc_data.get('safety_critical', False),
                expected_result=TestResult(tc_data.get('expected_result', 'PASS')),
                prerequisites=tc_data.get('prerequisites', [])
            )
            test_cases.append(test_case)
        
        return TestSuite(
            name=suite_data['name'],
            description=suite_data['description'],
            test_cases=test_cases,
            setup_script=suite_data.get('setup_script'),
            teardown_script=suite_data.get('teardown_script')
        )
    
    async def execute_test_case(self, test_case: TestCase) -> TestExecution:
        """Execute a single test case"""
        logging.info(f"Executing test case: {test_case.name}")
        
        # Safety check before execution
        is_safe, violations = self.safety_monitor.check_safety_constraints(test_case)
        if not is_safe:
            for violation in violations:
                self.safety_monitor.log_safety_violation(violation)
            
            return TestExecution(
                test_case=test_case,
                result=TestResult.SAFETY_VIOLATION,
                start_time=time.time(),
                end_time=time.time(),
                duration_seconds=0,
                error_message="Safety constraints violated",
                safety_violations=violations
            )
        
        start_time = time.time()
        
        try:
            # Build command based on test type
            if test_case.test_type == 'fault_injection':
                result = await self._execute_fault_injection_test(test_case)
            elif test_case.test_type == 'timing_analysis':
                result = await self._execute_timing_analysis_test(test_case)
            elif test_case.test_type == 'regression':
                result = await self._execute_regression_test(test_case)
            else:
                raise ValueError(f"Unknown test type: {test_case.test_type}")
            
            end_time = time.time()
            duration = end_time - start_time
            
            execution = TestExecution(
                test_case=test_case,
                result=result,
                start_time=start_time,
                end_time=end_time,
                duration_seconds=duration
            )
            
            logging.info(f"Test case {test_case.name} completed with result: {result.value}")
            return execution
            
        except asyncio.TimeoutError:
            end_time = time.time()
            duration = end_time - start_time
            
            logging.warning(f"Test case {test_case.name} timed out after {duration:.2f} seconds")
            
            return TestExecution(
                test_case=test_case,
                result=TestResult.TIMEOUT,
                start_time=start_time,
                end_time=end_time,
                duration_seconds=duration,
                error_message=f"Test timed out after {test_case.timeout_seconds} seconds"
            )
            
        except Exception as e:
            end_time = time.time()
            duration = end_time - start_time
            
            logging.error(f"Test case {test_case.name} failed with exception: {str(e)}")
            logging.debug(traceback.format_exc())
            
            return TestExecution(
                test_case=test_case,
                result=TestResult.FAIL,
                start_time=start_time,
                end_time=end_time,
                duration_seconds=duration,
                error_message=str(e)
            )
    
    async def _execute_fault_injection_test(self, test_case: TestCase) -> TestResult:
        """Execute fault injection test"""
        cmd = [
            str(self.framework_path / "build" / "examples" / "fault_injection_example"),
            "--scenario", test_case.dsl_scenario or "",
            "--output", str(self.output_dir / f"{test_case.name}_fault_injection.json")
        ]
        
        if test_case.config_file:
            cmd.extend(["--config", test_case.config_file])
        
        process = await asyncio.create_subprocess_exec(
            *cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )
        
        try:
            stdout, stderr = await asyncio.wait_for(
                process.communicate(),
                timeout=test_case.timeout_seconds
            )
            
            if process.returncode == 0:
                return TestResult.PASS
            else:
                logging.error(f"Fault injection test failed: {stderr.decode()}")
                return TestResult.FAIL
                
        except asyncio.TimeoutError:
            process.kill()
            raise
    
    async def _execute_timing_analysis_test(self, test_case: TestCase) -> TestResult:
        """Execute timing analysis test"""
        cmd = [
            str(self.framework_path / "build" / "examples" / "timing_analysis_example"),
            "--scenario", test_case.dsl_scenario or "",
            "--output", str(self.output_dir / f"{test_case.name}_timing.json")
        ]
        
        if test_case.config_file:
            cmd.extend(["--config", test_case.config_file])
        
        process = await asyncio.create_subprocess_exec(
            *cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )
        
        try:
            stdout, stderr = await asyncio.wait_for(
                process.communicate(),
                timeout=test_case.timeout_seconds
            )
            
            if process.returncode == 0:
                return TestResult.PASS
            else:
                logging.error(f"Timing analysis test failed: {stderr.decode()}")
                return TestResult.FAIL
                
        except asyncio.TimeoutError:
            process.kill()
            raise
    
    async def _execute_regression_test(self, test_case: TestCase) -> TestResult:
        """Execute regression test"""
        cmd = [
            str(self.framework_path / "build" / "examples" / "regression_test_example"),
            "--scenario", test_case.dsl_scenario or "",
            "--output", str(self.output_dir / f"{test_case.name}_regression.json")
        ]
        
        if test_case.config_file:
            cmd.extend(["--config", test_case.config_file])
        
        process = await asyncio.create_subprocess_exec(
            *cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )
        
        try:
            stdout, stderr = await asyncio.wait_for(
                process.communicate(),
                timeout=test_case.timeout_seconds
            )
            
            if process.returncode == 0:
                return TestResult.PASS
            else:
                logging.error(f"Regression test failed: {stderr.decode()}")
                return TestResult.FAIL
                
        except asyncio.TimeoutError:
            process.kill()
            raise
    
    async def execute_test_suite(self, test_suite: TestSuite) -> List[TestExecution]:
        """Execute complete test suite"""
        logging.info(f"Starting execution of test suite: {test_suite.name}")
        
        # Execute setup script if provided
        if test_suite.setup_script:
            await self._execute_script(test_suite.setup_script, "setup")
        
        # Sort test cases by severity (critical first)
        severity_order = {
            TestSeverity.CRITICAL: 0,
            TestSeverity.HIGH: 1,
            TestSeverity.MEDIUM: 2,
            TestSeverity.LOW: 3
        }
        
        sorted_tests = sorted(
            test_suite.test_cases,
            key=lambda tc: severity_order[tc.severity]
        )
        
        # Execute tests
        executions = []
        for test_case in sorted_tests:
            execution = await self.execute_test_case(test_case)
            executions.append(execution)
            self.test_results.append(execution)
            
            # Stop on critical safety violations
            if (execution.result == TestResult.SAFETY_VIOLATION and 
                test_case.severity == TestSeverity.CRITICAL):
                logging.critical("Critical safety violation detected. Stopping test suite execution.")
                break
        
        # Execute teardown script if provided
        if test_suite.teardown_script:
            await self._execute_script(test_suite.teardown_script, "teardown")
        
        logging.info(f"Completed execution of test suite: {test_suite.name}")
        return executions
    
    async def _execute_script(self, script_path: str, script_type: str):
        """Execute setup or teardown script"""
        logging.info(f"Executing {script_type} script: {script_path}")
        
        process = await asyncio.create_subprocess_exec(
            script_path,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )
        
        stdout, stderr = await process.communicate()
        
        if process.returncode != 0:
            logging.error(f"{script_type.capitalize()} script failed: {stderr.decode()}")
        else:
            logging.info(f"{script_type.capitalize()} script completed successfully")
    
    def generate_report(self) -> Dict[str, Any]:
        """Generate comprehensive test report"""
        total_tests = len(self.test_results)
        passed_tests = sum(1 for r in self.test_results if r.result == TestResult.PASS)
        failed_tests = sum(1 for r in self.test_results if r.result == TestResult.FAIL)
        timeout_tests = sum(1 for r in self.test_results if r.result == TestResult.TIMEOUT)
        safety_violations = sum(1 for r in self.test_results if r.result == TestResult.SAFETY_VIOLATION)
        
        # Calculate pass rate
        pass_rate = (passed_tests / total_tests * 100) if total_tests > 0 else 0
        
        # Safety metrics
        critical_tests = sum(1 for r in self.test_results if r.test_case.severity == TestSeverity.CRITICAL)
        critical_passed = sum(1 for r in self.test_results 
                            if r.test_case.severity == TestSeverity.CRITICAL and r.result == TestResult.PASS)
        critical_pass_rate = (critical_passed / critical_tests * 100) if critical_tests > 0 else 0
        
        report = {
            'summary': {
                'total_tests': total_tests,
                'passed': passed_tests,
                'failed': failed_tests,
                'timeout': timeout_tests,
                'safety_violations': safety_violations,
                'pass_rate': round(pass_rate, 2),
                'critical_pass_rate': round(critical_pass_rate, 2)
            },
            'safety_analysis': {
                'total_safety_violations': len(self.safety_monitor.safety_violations),
                'violations': self.safety_monitor.safety_violations,
                'critical_tests_executed': critical_tests,
                'critical_tests_passed': critical_passed
            },
            'detailed_results': [
                {
                    'test_name': execution.test_case.name,
                    'test_type': execution.test_case.test_type,
                    'severity': execution.test_case.severity.value,
                    'result': execution.result.value,
                    'duration_seconds': round(execution.duration_seconds, 3),
                    'safety_critical': execution.test_case.safety_critical,
                    'error_message': execution.error_message,
                    'safety_violations': execution.safety_violations
                }
                for execution in self.test_results
            ]
        }
        
        return report
    
    def save_report(self, report: Dict[str, Any], format: str = 'json'):
        """Save test report to file"""
        if format.lower() == 'json':
            report_file = self.output_dir / 'test_report.json'
            with open(report_file, 'w') as f:
                json.dump(report, f, indent=2)
        elif format.lower() == 'yaml':
            report_file = self.output_dir / 'test_report.yaml'
            with open(report_file, 'w') as f:
                yaml.dump(report, f, default_flow_style=False)
        
        logging.info(f"Test report saved to: {report_file}")


async def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='IV&V Framework Test Orchestrator for BCI Safety-Critical Systems'
    )
    parser.add_argument(
        'test_suite',
        help='Path to test suite configuration file (JSON or YAML)'
    )
    parser.add_argument(
        '--framework-path',
        default='.',
        help='Path to IV&V Framework root directory'
    )
    parser.add_argument(
        '--output-dir',
        default='./test_results',
        help='Directory for test output files'
    )
    parser.add_argument(
        '--report-format',
        choices=['json', 'yaml'],
        default='json',
        help='Test report format'
    )
    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Enable verbose logging'
    )
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Create test orchestrator
    orchestrator = TestOrchestrator(args.framework_path, args.output_dir)
    
    try:
        # Load test suite
        test_suite = await orchestrator.load_test_suite(args.test_suite)
        
        # Execute tests
        executions = await orchestrator.execute_test_suite(test_suite)
        
        # Generate and save report
        report = orchestrator.generate_report()
        orchestrator.save_report(report, args.report_format)
        
        # Print summary
        summary = report['summary']
        print(f"\n{'='*60}")
        print(f"IV&V FRAMEWORK TEST EXECUTION SUMMARY")
        print(f"{'='*60}")
        print(f"Total Tests: {summary['total_tests']}")
        print(f"Passed: {summary['passed']}")
        print(f"Failed: {summary['failed']}")
        print(f"Timeout: {summary['timeout']}")
        print(f"Safety Violations: {summary['safety_violations']}")
        print(f"Pass Rate: {summary['pass_rate']}%")
        print(f"Critical Test Pass Rate: {summary['critical_pass_rate']}%")
        
        if summary['safety_violations'] > 0:
            print(f"\n⚠️  WARNING: {summary['safety_violations']} safety violations detected!")
            print("Review safety_violations.log for details.")
        
        # Exit with appropriate code
        if summary['safety_violations'] > 0:
            sys.exit(2)  # Safety violations
        elif summary['failed'] > 0 or summary['timeout'] > 0:
            sys.exit(1)  # Test failures
        else:
            sys.exit(0)  # Success
            
    except Exception as e:
        logging.error(f"Test orchestration failed: {str(e)}")
        logging.debug(traceback.format_exc())
        sys.exit(3)


if __name__ == '__main__':
    asyncio.run(main())
