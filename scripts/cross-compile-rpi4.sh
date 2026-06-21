#!/bin/bash
set -e

# ---------------------------------------------------------------------------
# cross-compile-rpi4.sh -- cross-compile a single target for Raspberry Pi 4
#
# Usage:
#   ./scripts/cross-compile-rpi4.sh <target>
#
# Targets:
#   egl-triangle, drm-display, safemon-display, safemon-app, safemon-chart
#
# Override build dir (default: build):
#   BUILD_DIR=build-rpi4 ./scripts/cross-compile-rpi4.sh safemon-app
# ---------------------------------------------------------------------------

if [ -z "$1" ]; then
    echo "Usage: $0 <target>"
    echo "  targets: egl-triangle, drm-display, safemon-app, safemon-display, safemon-chart"
    exit 1
fi

TARGET=$1
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-build}"

source "$REPO_ROOT/scripts/cross-compile-lib/sources.sh"
source "$REPO_ROOT/scripts/cross-compile-lib/abseil-libs.sh"

WORK_DIR="$REPO_ROOT/$BUILD_DIR/tmp/work/cortexa72-poky-linux/safemon-app/1.0"
SYSROOT="$WORK_DIR/recipe-sysroot"
NATIVE_BIN="$WORK_DIR/recipe-sysroot-native/usr/bin/aarch64-poky-linux"

if [ ! -d "$SYSROOT" ]; then
    echo "[cross-compile] ERROR: sysroot not found: $SYSROOT"
    echo "  Run 'kas build kas-rpi4.yml' first, or set BUILD_DIR to your build directory."
    exit 1
fi

export PATH="$NATIVE_BIN:$PATH"
mkdir -p "$REPO_ROOT/out"

BASE_FLAGS="\
  -mcpu=cortex-a72+crc \
  -mbranch-protection=standard \
  -fstack-protector-strong \
  -O2 \
  --sysroot=$SYSROOT \
  -std=c++17 \
  -I$SYSROOT/usr/include \
  -I$SYSROOT/usr/include/drm \
  -L$SYSROOT/usr/lib"

case $TARGET in
  egl-triangle)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -lEGL -lGLESv2 -lgbm -ldrm \
      safemon/src/display/drm_helper.cpp \
      safemon/src/display/egl_helper_gbm.cpp \
      safemon/src/display/gl_app.cpp \
      safemon/src/legacy/egl_triangle.cpp \
      $CONFIG_SRC \
      -I safemon/inc/display \
      $CONFIG_INC \
      -o "$REPO_ROOT/out/$TARGET"
    ;;
  drm-display)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -ldrm -lhiredis \
      safemon/src/legacy/drm_display.cpp \
      safemon/src/legacy/framebuffer.cpp \
      -I safemon/inc/legacy \
      -o "$REPO_ROOT/out/$TARGET"
    ;;
  safemon-display)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -lEGL -lGLESv2 -lgbm -ldrm -lhiredis -lgmp -lcrypto \
      safemon/src/display/drm_helper.cpp \
      safemon/src/display/egl_helper_gbm.cpp \
      safemon/src/display/glass_panel.cpp \
      safemon/src/display/dashboard.cpp \
      safemon/src/display/text_renderer.cpp \
      safemon/src/display/safemon_display.cpp \
      $CONFIG_SRC \
      $ECDSA_SRC \
      -I safemon/inc/display \
      -I safemon/inc/third_party \
      $CONFIG_INC \
      $ECDSA_INC \
      -o "$REPO_ROOT/out/$TARGET"
    ;;
  safemon-app)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -lhiredis -lpthread -lgmp -lcrypto \
      -lgrpc++ -lgrpc -lgpr -lprotobuf \
      $ABSEIL_LIBS \
      safemon/src/services/safemon_app.cpp \
      safemon/src/services/can_reader.cpp \
      safemon/src/services/grpc_server.cpp \
      $CONFIG_SRC \
      $FAULT_SRC \
      $ECDSA_SRC \
      safemon/proto/fault.pb.cc \
      safemon/proto/fault.grpc.pb.cc \
      -I safemon/inc/services \
      $CONFIG_INC \
      $FAULT_INC \
      $ECDSA_INC \
      -I safemon/proto \
      -o "$REPO_ROOT/out/$TARGET"
    ;;
  safemon-chart)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -lEGL -lGLESv2 -lgbm -ldrm \
      safemon/src/display/drm_helper.cpp \
      safemon/src/display/egl_helper_gbm.cpp \
      safemon/src/display/waterfall_data.cpp \
      safemon/src/display/waterfall_chart.cpp \
      safemon/src/display/camera.cpp \
      safemon/src/display/text_renderer.cpp \
      safemon/src/display/axis_gizmo.cpp \
      safemon/src/display/safemon_chart.cpp \
      $CONFIG_SRC \
      -I safemon/inc/display \
      -I safemon/inc/third_party \
      $CONFIG_INC \
      -o "$REPO_ROOT/out/$TARGET"
    ;;
  *)
    echo "Unknown target: $TARGET"
    exit 1
    ;;
esac

echo "[cross-compile] Done: out/$TARGET"