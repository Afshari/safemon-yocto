#!/bin/bash
set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <target>"
    echo "  targets: egl-triangle, drm-display, safemon-app, safemon-display"
    exit 1
fi

TARGET=$1
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p $REPO_ROOT/out

export PATH="/home/ubuntu/safemon-yocto/build/tmp/work/cortexa72-poky-linux/safemon-app/1.0/recipe-sysroot-native/usr/bin/aarch64-poky-linux:$PATH"

SYSROOT=/home/ubuntu/safemon-yocto/build/tmp/work/cortexa72-poky-linux/safemon-app/1.0/recipe-sysroot

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
      safemon/src/drm_helper.cpp \
      safemon/src/egl_helper.cpp \
      safemon/src/gl_app.cpp \
      safemon/src/config.cpp \
      safemon/src/egl_triangle.cpp \
      -I safemon/inc \
      -o $REPO_ROOT/out/$TARGET
    ;;
  drm-display)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -ldrm -lhiredis \
      safemon/src/drm_display.cpp \
      safemon/src/framebuffer.cpp \
      -I safemon/inc \
      -o $REPO_ROOT/out/$TARGET
    ;;
  safemon-display)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -lEGL -lGLESv2 -lgbm -ldrm -lhiredis \
      safemon/src/drm_helper.cpp \
      safemon/src/egl_helper.cpp \
      safemon/src/safemon_display.cpp \
      safemon/src/gl_app.cpp \
      safemon/src/config.cpp \
      -I safemon/inc \
      -o $REPO_ROOT/out/$TARGET
    ;;
  safemon-app)
    aarch64-poky-linux-g++ $BASE_FLAGS \
      -lhiredis -lpthread -lgmp -lcrypto \
      -lgrpc++ -lgrpc -lprotobuf \
      safemon/src/main.cpp \
      safemon/src/can_reader.cpp \
      safemon/src/fault_detector.cpp \
      safemon/src/config.cpp \
      safemon/src/grpc_server.cpp \
      safemon/lib/ecdsa/src/bigint.cpp \
      safemon/lib/ecdsa/src/ec_point.cpp \
      safemon/lib/ecdsa/src/ecdsa.cpp \
      safemon/lib/ecdsa/src/ecdsa_verify_file.cpp \
      safemon/proto/fault.pb.cc \
      safemon/proto/fault.grpc.pb.cc \
      -I safemon/inc \
      -I safemon/lib/ecdsa/inc \
      -I safemon/proto \
      -o $REPO_ROOT/out/$TARGET
    ;;
  *)
    echo "Unknown target: $TARGET"
    exit 1
    ;;
esac

echo "[cross-compile] Done: $TARGET"
