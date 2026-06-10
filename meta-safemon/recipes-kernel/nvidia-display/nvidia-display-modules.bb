DESCRIPTION = "Load NVIDIA display modules at boot"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit allarch

do_install() {
    install -d ${D}${sysconfdir}/modules-load.d
    printf 'nvidia\nnvidia-modeset\nnvidia-drm\n' > \
        ${D}${sysconfdir}/modules-load.d/nvidia-display.conf
}

FILES:${PN} += "${sysconfdir}/modules-load.d/nvidia-display.conf"