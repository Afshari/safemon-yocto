DESCRIPTION = "systemd-networkd config for wlan0 DHCP"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://10-wlan0.network \
           file://99-wlan0-jetson.rules"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${sysconfdir}/systemd/network/
    install -m 0644 ${WORKDIR}/10-wlan0.network ${D}${sysconfdir}/systemd/network/10-wlan0.network

    install -d ${D}${sysconfdir}/udev/rules.d/
    install -m 0644 ${WORKDIR}/99-wlan0-jetson.rules ${D}${sysconfdir}/udev/rules.d/99-wlan0-jetson.rules
}

FILES:${PN} = "${sysconfdir}/systemd/network/ \
               ${sysconfdir}/udev/rules.d/"