#!/bin/bash
# app-build.sh - Compile a safemon binary on the VPS using bitbake
#
# Usage: ./app-build.sh <binary-name>
# Example: ./app-build.sh egl-triangle
#          ./app-build.sh safemon-app
#          ./app-build.sh drm-display
#
# Run this on the VPS from the repo root directory.

set -e

# --- Config ---
# Override this by setting REPO_DIR in your environment, e.g.:
# export REPO_DIR=/new/path/to/repo
REPO_DIR="${REPO_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
KAS_CONFIG="kas-config.yml"
RECIPE="safemon-app"

# --- Validate argument ---
if [ -z "$1" ]; then
    echo "Error: no binary specified."
    echo "Usage: ./app-build.sh <binary-name>"
    echo "Example: ./app-build.sh egl-triangle"
    exit 1
fi

BINARY="$1"

# --- Build ---
echo "Building $BINARY from recipe $RECIPE..."
echo "Repo: $REPO_DIR"
echo ""

cd "$REPO_DIR"

kas shell "$KAS_CONFIG" --command "bitbake $RECIPE -c compile -f"

echo ""
echo "Done. Binary should be at:"
echo "  $REPO_DIR/build/tmp/work/cortexa72-poky-linux/$RECIPE/1.0/image/usr/bin/$BINARY"