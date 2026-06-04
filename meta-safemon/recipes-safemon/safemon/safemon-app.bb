DESCRIPTION = "safemon functional safety monitor"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS:prepend := "${TOPDIR}/../:"

SRC_URI = "file://safemon/CMakeLists.txt \
           file://safemon/src/main.cpp \
           file://safemon/src/can_reader.cpp \
           file://safemon/inc/can_reader.h \
           file://safemon/src/drm_display.cpp \
           file://safemon/src/framebuffer.cpp \
           file://safemon/inc/framebuffer.h \
           file://safemon/src/egl_triangle.cpp \
           file://safemon/src/gl_app.cpp \
           file://safemon/src/safemon_display.cpp \
           file://safemon/src/drm_helper.cpp \
           file://safemon/src/egl_helper.cpp \
           file://safemon/src/fault_detector.cpp \
           file://safemon/src/config.cpp \
           file://safemon/inc/drm_helper.h \
           file://safemon/inc/egl_helper.h \
           file://safemon/inc/gl_app.h \
           file://safemon/inc/fault_detector.h \
           file://safemon/inc/config.h \
           file://safemon/lib/ecdsa/src/bigint.cpp \
           file://safemon/lib/ecdsa/src/ec_point.cpp \
           file://safemon/lib/ecdsa/src/ecdsa.cpp \
           file://safemon/lib/ecdsa/src/ecdsa_verify_file.cpp \
           file://safemon/lib/ecdsa/inc/bigint.h \
           file://safemon/lib/ecdsa/inc/ec_point.h \
           file://safemon/lib/ecdsa/inc/ecdsa.h \
           file://safemon/lib/ecdsa/inc/ecdsa_verify_file.h \
           file://safemon/lib/ecdsa/CMakeLists.txt \
           file://safemon.conf \
           file://safemon.conf.sig \
           file://safemon.pub \
           file://safemon-app.service \
           file://safemon-display.service \
           file://safemon/src/grpc_server.cpp \
           file://safemon/inc/grpc_server.h \
           file://safemon/proto/fault.proto \
           file://safemon/proto/fault.pb.cc \
           file://safemon/proto/fault.pb.h \
           file://safemon/proto/fault.grpc.pb.cc \
           file://safemon/proto/fault.grpc.pb.h \
          "

S = "${WORKDIR}/safemon"

inherit cmake systemd

DEPENDS = "hiredis pkgconfig libdrm virtual/libgles2 virtual/egl mesa gmp openssl grpc protobuf"

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
    install -m 0644 ${WORKDIR}/safemon-display.service \
        ${D}${systemd_system_unitdir}/safemon-display.service
}

FILES:${PN} += "${bindir}/safemon-app \
                ${bindir}/drm-display \
                ${bindir}/egl-triangle \
                ${bindir}/safemon-display \
                ${sysconfdir}/safemon/safemon.conf \
                ${sysconfdir}/safemon/safemon.conf.sig \
                ${sysconfdir}/safemon/pki/safemon.pub \
                ${systemd_system_unitdir}/safemon-app.service \
                ${systemd_system_unitdir}/safemon-display.service"