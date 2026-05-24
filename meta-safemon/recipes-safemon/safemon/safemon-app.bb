DESCRIPTION = "safemon functional safety monitor"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS:prepend := "${TOPDIR}/../:"

SRC_URI = "file://safemon/CMakeLists.txt \
           file://safemon/src/main.cpp \
           file://safemon/src/can_reader.cpp \
           file://safemon/inc/can_reader.h \
           file://safemon/src/drm_test.cpp \
           file://safemon/src/framebuffer.cpp \
           file://safemon/inc/framebuffer.h \
          "

S = "${WORKDIR}/safemon"

inherit cmake

DEPENDS = "hiredis pkgconfig libdrm"

FILES:${PN} += "${bindir}/safemon-app ${bindir}/drm-test"