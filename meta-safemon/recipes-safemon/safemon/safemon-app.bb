DESCRIPTION = "safemon functional safety monitor"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS:prepend := "${TOPDIR}/../:"

SRC_URI = "file://safemon/CMakeLists.txt \
           file://safemon/src/main.cpp \
           file://safemon/include \
          "

S = "${WORKDIR}"

inherit cmake

DEPENDS = "hiredis"

FILES:${PN} += "${bindir}/safemon-app"