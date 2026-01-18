#!/usr/bin/env bash
set -euo pipefail

# Compatibility shim: the canonical location is now Tests/test_campaign.sh
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
exec bash "$ROOT_DIR/Tests/test_campaign.sh" "$@"
