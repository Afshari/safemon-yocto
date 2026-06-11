#!/bin/bash
set -e

# ---------------------------------------------------------------------------
# cross-compile.sh -- cross-compile a single target for Raspberry Pi 4
#
# Usage:
#   ./scripts/cross-compile.sh <target>
#
# Targets:
#   egl-triangle, drm-display, safemon-display, safemon-app
#
# Override build dir (default: build):
#   BUILD_DIR=build-rpi4 ./scripts/cross-compile.sh safemon-app
# ---------------------------------------------------------------------------

if [ -z "$1" ]; then
    echo "Usage: $0 <target>"
    echo "  targets: egl-triangle, drm-display, safemon-app, safemon-display"
    exit 1
fi

TARGET=$1
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-build}"

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

ECDSA_SRC="\
  safemon/lib/ecdsa/src/bigint.cpp \
  safemon/lib/ecdsa/src/ec_point.cpp \
  safemon/lib/ecdsa/src/ecdsa.cpp \
  safemon/lib/ecdsa/src/ecdsa_verify_file.cpp"

ECDSA_INC="-I safemon/lib/ecdsa/inc"

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

case $TARGET in
  egl-triangle)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -lEGL -lGLESv2 -lgbm -ldrm \
      safemon/src/drm_helper.cpp \
      safemon/src/egl_helper.cpp \
      safemon/src/gl_app.cpp \
      safemon/src/config.cpp \
      safemon/src/egl_triangle.cpp \
      -I safemon/inc \
      -o "$REPO_ROOT/out/$TARGET"
    ;;
  drm-display)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -ldrm -lhiredis \
      safemon/src/drm_display.cpp \
      safemon/src/framebuffer.cpp \
      -I safemon/inc \
      -o "$REPO_ROOT/out/$TARGET"
    ;;
  safemon-display)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -lEGL -lGLESv2 -lgbm -ldrm -lhiredis -lgmp -lcrypto \
      safemon/src/drm_helper.cpp \
      safemon/src/egl_helper.cpp \
      safemon/src/safemon_display.cpp \
      safemon/src/gl_app.cpp \
      safemon/src/config.cpp \
      $ECDSA_SRC \
      -I safemon/inc \
      $ECDSA_INC \
      -o "$REPO_ROOT/out/$TARGET"
    ;;
  safemon-app)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -lhiredis -lpthread -lgmp -lcrypto \
      -lgrpc++ -lgrpc -lgpr -lprotobuf \
      $ABSEIL_LIBS \
      safemon/src/main.cpp \
      safemon/src/can_reader.cpp \
      safemon/src/fault_detector.cpp \
      safemon/src/config.cpp \
      safemon/src/grpc_server.cpp \
      $ECDSA_SRC \
      safemon/proto/fault.pb.cc \
      safemon/proto/fault.grpc.pb.cc \
      -I safemon/inc \
      $ECDSA_INC \
      -I safemon/proto \
      -o "$REPO_ROOT/out/$TARGET"
    ;;
  *)
    echo "Unknown target: $TARGET"
    exit 1
    ;;
esac

echo "[cross-compile] Done: out/$TARGET"