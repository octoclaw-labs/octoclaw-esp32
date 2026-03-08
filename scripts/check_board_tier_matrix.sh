#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

python3 "${PROJECT_ROOT}/scripts/gen_board_tier_matrix.py" --check
echo "[OK] board tier matrix check passed"
