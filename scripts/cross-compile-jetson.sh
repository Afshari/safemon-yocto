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

ECDSA_SRC="\
  safemon/lib/ecdsa/src/bigint.cpp \
  safemon/lib/ecdsa/src/ec_point.cpp \
  safemon/lib/ecdsa/src/ecdsa.cpp \
  safemon/lib/ecdsa/src/ecdsa_verify_file.cpp"

ECDSA_INC="-I safemon/lib/ecdsa/inc"

CONFIG_SRC="safemon/lib/config/src/config.cpp"
CONFIG_INC="-I safemon/lib/config/inc"

FAULT_SRC="\
  safemon/lib/fault_detector/src/fault_rules.cpp \
  safemon/lib/fault_detector/src/fault_detector.cpp"
FAULT_INC="-I safemon/lib/fault_detector/inc"

ABSEIL_LIBS="\
  -labsl_log_internal_check_op -labsl_log_initialize \
  -labsl_log_globals -labsl_log_entry -labsl_log_sink \
  -labsl_log_internal_message -labsl_log_internal_format \
  -labsl_log_internal_globals -labsl_log_internal_log_sink_set \
  -labsl_log_internal_conditions -labsl_log_internal_proto \
  -labsl_log_internal_nullguard -labsl_strings -labsl_base \
  -labsl_raw_logging_internal -labsl_spinlock_wait \
  -labsl_synchronization -labsl_status -labsl_statusor \
  -labsl_time -labsl_time_zone -labsl_cord -labsl_cord_internal \
  -labsl_cordz_functions -labsl_cordz_handle -labsl_cordz_info \
  -labsl_crc32c -labsl_crc_cord_state -labsl_crc_cpu_detect \
  -labsl_crc_internal -labsl_str_format_internal \
  -labsl_strings_internal -labsl_string_view \
  -labsl_int128 -labsl_hash -labsl_raw_hash_set \
  -labsl_hashtablez_sampler -labsl_exponential_biased \
  -labsl_bad_optional_access -labsl_bad_variant_access \
  -labsl_malloc_internal -labsl_debugging_internal \
  -labsl_demangle_internal -labsl_stacktrace -labsl_symbolize \
  -labsl_strerror -labsl_throw_delegate"

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
      safemon/src/drm_helper.cpp \
      safemon/src/egl_helper_wayland.cpp \
      safemon/src/safemon_display.cpp \
      safemon/src/gl_app.cpp \
      "$XDG_SHELL_OBJ" \
      $CONFIG_SRC \
      $ECDSA_SRC \
      -I safemon/inc \
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
      safemon/src/main.cpp \
      safemon/src/can_reader.cpp \
      safemon/src/grpc_server.cpp \
      $CONFIG_SRC \
      $FAULT_SRC \
      $ECDSA_SRC \
      safemon/proto/fault.pb.cc \
      safemon/proto/fault.grpc.pb.cc \
      -I safemon/inc \
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