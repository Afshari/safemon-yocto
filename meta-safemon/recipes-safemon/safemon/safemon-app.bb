DESCRIPTION = "safemon functional safety monitor"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS:prepend := "${TOPDIR}/../:"

SRC_URI = "file://safemon/CMakeLists.txt \
           file://safemon/cmake/dependencies.cmake \
           file://safemon/cmake/platform.cmake \
           file://safemon/src/services/safemon_app.cpp \
           file://safemon/src/services/can_reader.cpp \
           file://safemon/inc/services/can_reader.h \
           file://safemon/src/services/grpc_server.cpp \
           file://safemon/inc/services/grpc_server.h \
           file://safemon/src/legacy/drm_display.cpp \
           file://safemon/src/legacy/framebuffer.cpp \
           file://safemon/inc/legacy/framebuffer.h \
           file://safemon/src/legacy/egl_triangle.cpp \
           file://safemon/src/display/gl_app.cpp \
           file://safemon/src/display/safemon_display.cpp \
           file://safemon/src/display/drm_helper.cpp \
           file://safemon/src/display/egl_helper_gbm.cpp \
           file://safemon/src/display/egl_helper_wayland.cpp \
           file://safemon/inc/display/drm_helper.h \
           file://safemon/inc/display/egl_helper.h \
           file://safemon/inc/display/gl_app.h \
           file://safemon/lib/CMakeLists.txt \
           file://safemon/lib/ecdsa/src/bigint.cpp \
           file://safemon/lib/ecdsa/src/ec_point.cpp \
           file://safemon/lib/ecdsa/src/ecdsa.cpp \
           file://safemon/lib/ecdsa/src/ecdsa_verify_file.cpp \
           file://safemon/lib/ecdsa/inc/bigint.h \
           file://safemon/lib/ecdsa/inc/ec_point.h \
           file://safemon/lib/ecdsa/inc/ecdsa.h \
           file://safemon/lib/ecdsa/inc/ecdsa_verify_file.h \
           file://safemon/lib/ecdsa/CMakeLists.txt \
           file://safemon/lib/config/src/config.cpp \
           file://safemon/lib/config/inc/config.h \
           file://safemon/lib/config/CMakeLists.txt \
           file://safemon/lib/fault_detector/src/fault_rules.cpp \
           file://safemon/lib/fault_detector/src/fault_detector.cpp \
           file://safemon/lib/fault_detector/inc/fault_rules.h \
           file://safemon/lib/fault_detector/inc/fault_detector.h \
           file://safemon/lib/fault_detector/CMakeLists.txt \
           file://safemon.conf \
           file://safemon.conf.sig \
           file://safemon.pub \
           file://safemon-app.service \
           file://safemon-display.service \
           file://safemon-display-jetson.service \
           file://safemon/proto/fault.proto \
           file://safemon/proto/fault.pb.cc \
           file://safemon/proto/fault.pb.h \
           file://safemon/proto/fault.grpc.pb.cc \
           file://safemon/proto/fault.grpc.pb.h \
          "

S = "${WORKDIR}/safemon"

inherit cmake systemd
# Default platform is rpi4
EXTRA_OECMAKE = " -DPLATFORM=rpi4"
# Override for Qemu
EXTRA_OECMAKE:qemuarm64 = " -DPLATFORM=rpi4"
DEPENDS:append:qemuarm64 = " mesa"
# Override for Jetson
EXTRA_OECMAKE:jetson-orin-nano-devkit = " -DPLATFORM=jetson"

DEPENDS = "hiredis pkgconfig libdrm virtual/libgles2 virtual/egl gmp openssl grpc protobuf"
# RPi4 needs Mesa/GBM
DEPENDS:append:raspberrypi4-64 = " mesa"
# Jetson needs Wayland
DEPENDS:append:jetson-orin-nano-devkit = " wayland wayland-native wayland-protocols"

SYSTEMD_SERVICE:${PN} = "safemon-app.service safemon-display.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}${sysconfdir}/safemon
    install -m 0644 ${WORKDIR}/safemon.conf \
        ${D}${sysconfdir}/safemon/safemon.conf

    install -m 0644 ${WORKDIR}/safemon.conf.sig \
        ${D}${sysconfdir}/safemon/safemon.conf.sig

    install -d ${D}${sysconfdir}/safemon/pki
    install -m 0644 ${WORKDIR}/safemon.pub \
        ${D}${sysconfdir}/safemon/pki/safemon.pub

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/safemon-app.service \
        ${D}${systemd_system_unitdir}/safemon-app.service

    # Install platform-specific display service
    if [ "${MACHINE}" = "jetson-orin-nano-devkit" ]; then
        install -m 0644 ${WORKDIR}/safemon-display-jetson.service \
            ${D}${systemd_system_unitdir}/safemon-display.service
    else
        install -m 0644 ${WORKDIR}/safemon-display.service \
            ${D}${systemd_system_unitdir}/safemon-display.service
    fi

    # Build info
    echo "Build date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')" > ${D}${sysconfdir}/safemon-build-info
    echo "Machine: ${MACHINE}" >> ${D}${sysconfdir}/safemon-build-info
}


FILES:${PN} += "${bindir}/safemon-app \
                ${bindir}/drm-display \
                ${bindir}/egl-triangle \
                ${bindir}/safemon-display \
                ${sysconfdir}/safemon/safemon.conf \
                ${sysconfdir}/safemon/safemon.conf.sig \
                ${sysconfdir}/safemon/pki/safemon.pub \
                ${sysconfdir}/safemon-build-info \
                ${systemd_system_unitdir}/safemon-app.service \
                ${systemd_system_unitdir}/safemon-display.service"