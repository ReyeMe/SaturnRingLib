import argparse
import re
from collections import defaultdict
from xml.etree import ElementTree as ET


def _normalize_suite_name(raw_suite: str) -> str:
    suite = raw_suite.strip()
    suite = suite.replace("_ERROR(S)", "")
    suite = suite.strip("_")
    return suite.lower()


def parse_uts_log(log_file: str):
    """Parse SaturnRingLib uts.log output.

    Expected patterns (same as tools/scripts/ctrf_converter.py):
    - Passed:  "TESTING : Passed :<testname>"
    - Failed:  "FATAL : <testname> failed:" followed by "<file>:<line>: <message>"

    Returns:
        (testcases, totals, per_suite_counts)
    """
    testcases = []

    totals = {
        "tests": 0,
        "failures": 0,
        "errors": 0,
        "skipped": 0,
    }

    per_suite_counts = defaultdict(lambda: {"tests": 0, "failures": 0, "errors": 0, "skipped": 0})

    current_suite = None

    with open(log_file, "r", encoding="utf-8", errors="replace") as file:
        lines = file.readlines()

    i = 0
    while i < len(lines):
        line = lines[i]

        # Match suite headers like: "TESTING : ****UT_MEMORY_HWRAM****"
        suite_match = re.match(r"(?:TESTING|INFO)\s*:\s*\*\*\*\*UT_(.*?)\*\*\*\*", line)
        if suite_match:
            current_suite = _normalize_suite_name(suite_match.group(1))
            i += 1
            continue

        passed_match = re.match(r"TESTING\s*:\s*Passed\s*:(.*)", line)
        if passed_match:
            test_name = passed_match.group(1).strip()
            classname = current_suite or test_name
            testcases.append(
                {
                    "name": test_name,
                    "classname": classname,
                    "status": "passed",
                    "time": 0.0,
                }
            )
            totals["tests"] += 1
            per_suite_counts[classname]["tests"] += 1
            i += 1
            continue

        failed_match = re.match(r"FATAL\s*:\s*(.*)\s+failed:", line)
        if failed_match and i + 1 < len(lines):
            test_name = failed_match.group(1).strip()
            classname = current_suite or test_name

            details_line = lines[i + 1].strip()
            details_match = re.match(r"(.*):(\d+):\s*(.*)", details_line)

            failure_message = None
            failure_trace = details_line
            failure_file = None
            failure_line = None

            if details_match:
                failure_file = details_match.group(1).strip()
                failure_line = int(details_match.group(2))
                failure_message = details_match.group(3).strip()

            testcases.append(
                {
                    "name": test_name,
                    "classname": classname,
                    "status": "failed",
                    "time": 0.0,
                    "message": failure_message or "Test failed",
                    "trace": failure_trace,
                    "filePath": failure_file,
                    "line": failure_line,
                }
            )

            totals["tests"] += 1
            totals["failures"] += 1
            per_suite_counts[classname]["tests"] += 1
            per_suite_counts[classname]["failures"] += 1

            i += 2
            continue

        i += 1

    return testcases, totals, per_suite_counts


def build_junit_xml(testcases, totals, per_suite_counts, suite_name: str = "SaturnRingLib Unit Tests") -> ET.ElementTree:
    # Root element: testsuites
    root = ET.Element(
        "testsuites",
        {
            "name": suite_name,
            "tests": str(totals["tests"]),
            "failures": str(totals["failures"]),
            "errors": str(totals["errors"]),
            "skipped": str(totals["skipped"]),
            "time": "0",
        },
    )

    # Group testcases per classname (suite)
    cases_by_suite = defaultdict(list)
    for tc in testcases:
        cases_by_suite[tc["classname"]].append(tc)

    # Stable ordering for deterministic output
    for classname in sorted(cases_by_suite.keys()):
        suite_counts = per_suite_counts[classname]
        testsuite_el = ET.SubElement(
            root,
            "testsuite",
            {
                "name": classname,
                "tests": str(suite_counts["tests"]),
                "failures": str(suite_counts["failures"]),
                "errors": str(suite_counts["errors"]),
                "skipped": str(suite_counts["skipped"]),
                "time": "0",
            },
        )

        # If a suite emits duplicate testcase names, de-duplicate them for JUnit consumers
        # that key on (classname, name).
        seen_names = defaultdict(int)
        for tc in cases_by_suite[classname]:
            original_name = tc["name"]
            seen_names[original_name] += 1
            name = original_name
            if seen_names[original_name] > 1:
                name = f"{original_name} #{seen_names[original_name]}"
            testcase_el = ET.SubElement(
                testsuite_el,
                "testcase",
                {
                    "name": name,
                    "classname": tc["classname"],
                    "time": str(tc.get("time", 0.0)),
                },
            )

            if tc["status"] == "failed":
                failure_el = ET.SubElement(
                    testcase_el,
                    "failure",
                    {
                        "message": tc.get("message") or "Test failed",
                        "type": "failure",
                    },
                )
                trace_parts = []
                if tc.get("filePath") and tc.get("line") is not None:
                    trace_parts.append(f"{tc['filePath']}:{tc['line']}: {tc.get('message') or ''}".rstrip())
                if tc.get("trace"):
                    # Keep original trace line as well for fidelity.
                    trace_parts.append(str(tc["trace"]))
                failure_el.text = "\n".join(dict.fromkeys([p for p in trace_parts if p]))

    return ET.ElementTree(root)


def main():
    parser = argparse.ArgumentParser(description="Convert uts.log to JUnit XML.")
    parser.add_argument("log_file", help="Path to the uts.log file")
    parser.add_argument("output_file", help="Path to the output JUnit XML file")
    parser.add_argument(
        "--suite-name",
        default="SaturnRingLib Unit Tests",
        help="Name attribute for the root testsuites element",
    )
    args = parser.parse_args()

    testcases, totals, per_suite_counts = parse_uts_log(args.log_file)
    tree = build_junit_xml(testcases, totals, per_suite_counts, suite_name=args.suite_name)

    # Pretty-print (ElementTree has no native pretty printer pre-3.9; indent exists in 3.9+)
    try:
        ET.indent(tree, space="  ", level=0)  # type: ignore[attr-defined]
    except Exception:
        pass

    tree.write(args.output_file, encoding="utf-8", xml_declaration=True)
    print(f"Conversion complete. JUnit XML report saved to {args.output_file}")


if __name__ == "__main__":
    main()
