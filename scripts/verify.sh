#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
LOCAL_CONFIG="$ROOT/firmware/include/wifi_config.h"
CREATED_CONFIG=0

cleanup() {
  if [[ "$CREATED_CONFIG" == 1 ]]; then
    rm -f -- "$LOCAL_CONFIG"
  fi
}
trap cleanup EXIT

python3 "$ROOT/scripts/secret_scan.py" --root "$ROOT"
python3 "$ROOT/scripts/check_repo.py" --root "$ROOT"

if [[ ! -e "$LOCAL_CONFIG" ]]; then
  cp -- "$ROOT/firmware/include/wifi_config.example.h" "$LOCAL_CONFIG"
  CREATED_CONFIG=1
fi

PYTHONPYCACHEPREFIX="$(mktemp -d)"
export PYTHONPYCACHEPREFIX
PIO_WORK_DIR="$(mktemp -d)"
trap 'rm -rf -- "$PYTHONPYCACHEPREFIX" "$PIO_WORK_DIR"; cleanup' EXIT

python3 -m py_compile \
  "$ROOT/tools/tcp_client.py" \
  "$ROOT/tools/web/server.py"

cp -a -- "$ROOT/firmware/." "$PIO_WORK_DIR/"
pio run -d "$PIO_WORK_DIR"

echo "Verification: PASS"
