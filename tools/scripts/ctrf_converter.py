import json
import re
import argparse
import os
from collections import Counter


def campaign_name(log_file):
    """Campaign of a run_tests.bat log: uts_<campaign>.log gives <campaign>."""
    name = os.path.splitext(os.path.basename(log_file))[0]
    return name[4:] if name.startswith("uts_") else name


def parse_uts_log(log_file, suite):
    """Tests of one log, each tagged with the given suite (its campaign)."""
    testcases = []

    with open(log_file, 'r') as file:
        lines = file.readlines()
        i = 0
        while i < len(lines):
            line = lines[i]

            # Match passed tests
            passed_match = re.match(r"TESTING : Passed :(.*)", line)
            if passed_match:
                testcases.append({
                    "name": passed_match.group(1).strip(),
                    "classname": passed_match.group(1).split('_')[0],
                    "status": "passed",
                    "rawStatus": "passed",
                    "duration": 0,
                    "retries": 0,
                    "suite": suite
                })

            # Match failed tests (read two lines)
            failed_match = re.match(r"FATAL : (.*) failed:", line)
            if failed_match and i + 1 < len(lines):
                details_line = lines[i + 1].strip()
                details_match = re.match(r"(.*):(\d+): (.*)", details_line)
                if details_match:
                    testcases.append({
                        "name": failed_match.group(1).strip(),
                        "classname": failed_match.group(1).split('_')[0],
                        "status": "failed",
                        "rawStatus": "failed",
                        "message": details_match.group(3).strip(),
                        "trace": details_line,
                        "filePath": details_match.group(1).strip(),
                        "line": int(details_match.group(2)),
                        "duration": 0,
                        "retries": 0,
                        "suite": suite
                    })
                i += 1  # Skip the details line

            i += 1

    return testcases


def build_report(testcases):
    """One CTRF report for all tests; summary.suites counts the tests of each campaign."""
    failed = sum(1 for test in testcases if test["status"] == "failed")
    return {
        "reportFormat": "CTRF",
        "specVersion": "0.0.0",
        "results": {
            "summary": {
                "name": "SaturnRingLib Unit Tests",
                "tests": len(testcases),
                "failed": failed,
                "passed": len(testcases) - failed,
                "skipped": 0,
                "suites": dict(Counter(test["suite"] for test in testcases))
            },
            "tool": {
                "name": "minunit"
            },
            "tests": testcases
        }
    }


def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Convert one or more uts_<campaign>.log files to one CTRF JSON report.")
    parser.add_argument("log_files", nargs="+", help="run_tests.bat logs (uts_<campaign>.log)")
    parser.add_argument("output_file", help="Path to the output JSON file")
    args = parser.parse_args()

    testcases = []
    for log_file in args.log_files:
        testcases += parse_uts_log(log_file, campaign_name(log_file))

    # Write the JSON report
    with open(args.output_file, 'w') as json_file:
        json.dump(build_report(testcases), json_file, indent=4)

    print(f"Conversion complete: {len(testcases)} tests from {len(args.log_files)} log(s) saved to {args.output_file}")


if __name__ == "__main__":
    main()
