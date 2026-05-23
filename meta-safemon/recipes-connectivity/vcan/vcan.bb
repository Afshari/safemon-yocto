DESCRIPTION = "Virtual CAN interface setup"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://vcan0.service"

inherit systemd

SYSTEMD_SERVICE:${PN} = "vcan0.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/vcan0.service \
        ${D}${systemd_system_unitdir}/vcan0.service
}

FILES:${PN} += "${systemd_system_unitdir}/vcan0.service"