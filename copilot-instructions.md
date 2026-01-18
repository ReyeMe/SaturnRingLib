# Copilot instructions (SaturnRingLib)

This repository has an emulator-driven unit test suite. When you change code, validate it by running the **test campaign**.

## Test campaign (preferred)

Run from the repo root:

- Full build + run + reports:
  - `bash Tests/test_campaign.sh --emulator mednafen`

This produces/updates:
- `Tests/uts.log` (raw output)
- `Tests/uts.json` (CTRF JSON)
- `Tests/uts.xml` (JUnit XML)

## Fast validation for tooling-only changes

If you changed only the Python report converters or scripts (no C/C++ changes):

- Compile-check Python scripts:
  - `python3 -m compileall tools/scripts`
- Regenerate reports from an existing log:
  - `bash Tests/test_campaign.sh --skip-build --skip-run`

## Notes

- The test runner exits successfully when it sees the `***UT_END***` marker; the log may still contain failing tests. Do **not** change the default behavior to “fail the build on test failures” unless explicitly requested.
- If you need a strict gate locally, the wrapper supports:
  - `bash Tests/test_campaign.sh --skip-build --skip-run --strict`
