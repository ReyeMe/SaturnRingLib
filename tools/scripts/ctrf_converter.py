import json
import re
import argparse
from collections import defaultdict

def parse_uts_log(log_file):
    testcases = []
    total_tests = 0
    total_failures = 0
    classname_counts = defaultdict(int)  # Counter for unique classnames

    with open(log_file, 'r') as file:
        lines = file.readlines()
        i = 0
        while i < len(lines):
            line = lines[i]

            # Match passed tests
            passed_match = re.match(r"TESTING : Passed :(.*)", line)
            if passed_match:
                classname = passed_match.group(1).split('_')[0]
                classname_counts[classname] += 1  # Increment classname counter
                testcases.append({
                    "name": passed_match.group(1).strip(),
                    "classname": classname,
                    "status": "passed",
                    "rawStatus": "passed",
                    "duration": 0,
                    "retries": 0,
                    "suite": classname
                })
                total_tests += 1

            # Match failed tests (read two lines)
            failed_match = re.match(r"FATAL : (.*) failed:", line)
            if failed_match and i + 1 < len(lines):
                classname = failed_match.group(1).split('_')[0]
                classname_counts[classname] += 1  # Increment classname counter
                details_line = lines[i + 1].strip()
                details_match = re.match(r"(.*):(\d+): (.*)", details_line)
                if details_match:
                    testcases.append({
                        "name": failed_match.group(1).strip(),
                        "classname": classname,
                        "status": "failed",
                        "rawStatus": "failed",
                        "message": details_match.group(3).strip(),
                        "trace": details_line,
                        "filePath": details_match.group(1).strip(),
                        "line": int(details_match.group(2)),
                        "duration": 0,
                        "retries": 0,
                        "suite": classname
                    })
                    total_tests += 1
                    total_failures += 1
                i += 1  # Skip the details line

            i += 1

    testsuites = {
        "summary": {
            "name": "SaturnRingLib Unit Tests",
            "tests": total_tests,
            "failed": total_failures,
            "passed": total_tests - total_failures,
            "skipped": 0,
            "suites": dict(classname_counts) 
        },
        "tool": {
            "name": "minunit"
        },
        "tests": testcases
    }

    return {
        "reportFormat": "CTRF",
        "specVersion": "0.0.0",
        "results": testsuites
    }


def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Convert uts.log to JSON format.")
    parser.add_argument("log_file", help="Path to the uts.log file")
    parser.add_argument("output_file", help="Path to the output JSON file")
    args = parser.parse_args()

    # Parse the log file
    report = parse_uts_log(args.log_file)

    # Write the JSON report
    with open(args.output_file, 'w') as json_file:
        json.dump(report, json_file, indent=4)

    print(f"Conversion complete. JSON report saved to {args.output_file}")


if __name__ == "__main__":
    main()