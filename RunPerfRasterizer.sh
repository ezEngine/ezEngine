#!/bin/bash
# Profiles the rasterizer performance test using Linux perf and opens the result in hotspot.
# Usage: ./RunPerfRasterizer.sh [build-dir]
#   build-dir: path to the build output directory (default: Workspace/copilot-output)

set -e

BUILD_DIR="${1:-Workspace/copilot-output/Bin/LinuxNinjaGccDebug64}"
TEST_BIN="$BUILD_DIR/RendererTest"

if [ ! -f "$TEST_BIN" ]; then
  echo "Error: $TEST_BIN not found. Build RendererTest first."
  exit 1
fi

PERF_DATA="perf.data"

echo "=== Recording perf profile ==="
perf record -g --call-graph dwarf -F 9999 -o "$PERF_DATA" -- "$TEST_BIN" -run -noGui -filter "Performance"

echo ""
echo "=== Profile recorded to $PERF_DATA ==="
echo ""

# Try hotspot first (best GUI), fall back to perf report
if command -v hotspot &> /dev/null; then
  echo "Opening hotspot..."
  hotspot "$PERF_DATA"
elif command -v perf &> /dev/null; then
  echo "hotspot not found, using perf report..."
  perf report -g --no-children -i "$PERF_DATA"
fi
