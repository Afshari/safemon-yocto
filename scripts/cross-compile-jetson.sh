#!/bin/bash
set -e

# ---------------------------------------------------------------------------
# cross-compile-jetson.sh -- cross-compile a single target for Jetson Orin Nano
#
# Usage:
#   ./scripts/cross-compile-jetson.sh <target>
#
# Targets:
#   safemon-display, safemon-app
#
# Override build dir (default: build-jetson):
#   BUILD_DIR=build-jetson2 ./scripts/cross-compile-jetson.sh safemon-app
# ---------------------------------------------------------------------------

if [ -z "$1" ]; then
    echo "Usage: $0 <target>"
    echo "  targets: safemon-app, safemon-display"
    exit 1
fi

TARGET=$1
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-build-jetson}"

source "$REPO_ROOT/scripts/cross-compile-lib/sources.sh"
source "$REPO_ROOT/scripts/cross-compile-lib/abseil-libs.sh"

WORK_DIR="$REPO_ROOT/$BUILD_DIR/tmp/work/armv8a-poky-linux/safemon-app/1.0"
SYSROOT="$WORK_DIR/recipe-sysroot"
NATIVE_BIN="$WORK_DIR/recipe-sysroot-native/usr/bin/aarch64-poky-linux"

if [ ! -d "$SYSROOT" ]; then
    echo "[cross-compile] ERROR: sysroot not found: $SYSROOT"
    echo "  Run 'KAS_BUILD_DIR=build-jetson kas build kas-jetson.yml' first."
    exit 1
fi

export PATH="$NATIVE_BIN:$PATH"
mkdir -p "$REPO_ROOT/out"

BASE_FLAGS="\
  -mcpu=cortex-a78ae \
  -fstack-protector-strong \
  -O2 \
  --sysroot=$SYSROOT \
  -std=c++17 \
  -DPLATFORM_JETSON \
  -I$SYSROOT/usr/include \
  -I$SYSROOT/usr/include/drm \
  -L$SYSROOT/usr/lib"

XDG_SHELL_XML="$SYSROOT/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
XDG_SHELL_HEADER="$REPO_ROOT/out/xdg-shell-client-protocol.h"
XDG_SHELL_SRC="$REPO_ROOT/out/xdg-shell-protocol.c"
XDG_SHELL_OBJ="$REPO_ROOT/out/xdg-shell-protocol.o"

if [ "$TARGET" = "safemon-display" ]; then
    if [ ! -f "$XDG_SHELL_XML" ]; then
        echo "[cross-compile] ERROR: xdg-shell.xml not found at $XDG_SHELL_XML"
        exit 1
    fi
    wayland-scanner client-header "$XDG_SHELL_XML" "$XDG_SHELL_HEADER"
    wayland-scanner private-code  "$XDG_SHELL_XML" "$XDG_SHELL_SRC"
    aarch64-poky-linux-gcc --sysroot=$SYSROOT -c "$XDG_SHELL_SRC" -I "$REPO_ROOT/out" -o "$XDG_SHELL_OBJ"
fi

case $TARGET in
safemon-display)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -lEGL -lGLESv2 -lwayland-client -lwayland-egl -ldrm -lhiredis -lgmp -lcrypto \
      safemon/src/display/drm_helper.cpp \
      safemon/src/display/egl_helper_wayland.cpp \
      safemon/src/display/glass_panel.cpp \
      safemon/src/display/dashboard.cpp \
      safemon/src/display/text_renderer.cpp \
      safemon/src/display/safemon_display.cpp \
      "$XDG_SHELL_OBJ" \
      $CONFIG_SRC \
      $ECDSA_SRC \
      -I safemon/inc/display \
      -I safemon/inc/third_party \
      -I "$REPO_ROOT/out" \
      $CONFIG_INC \
      $ECDSA_INC \
      -o "$REPO_ROOT/out/$TARGET-jetson"
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
      -o "$REPO_ROOT/out/$TARGET-jetson"
    ;;
  *)
    echo "Unknown target: $TARGET"
    exit 1
    ;;
esac

echo "[cross-compile] Done: out/$TARGET-jetson"