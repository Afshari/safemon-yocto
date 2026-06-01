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
           file://safemon.conf \
           file://safemon-app.service \
           file://safemon-display.service \
          "

S = "${WORKDIR}/safemon"

inherit cmake systemd

DEPENDS = "hiredis pkgconfig libdrm virtual/libgles2 virtual/egl mesa"

SYSTEMD_SERVICE:${PN} = "safemon-app.service safemon-display.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}${sysconfdir}/safemon
    install -m 0644 ${WORKDIR}/safemon.conf \
        ${D}${sysconfdir}/safemon/safemon.conf

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
                ${systemd_system_unitdir}/safemon-app.service \
                ${systemd_system_unitdir}/safemon-display.service"