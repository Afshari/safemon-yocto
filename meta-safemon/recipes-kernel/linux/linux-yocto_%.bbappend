FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append:qemuarm64 = " file://vcan.cfg"

KERNEL_CONFIG_FRAGMENTS:append:qemuarm64 = " ${WORKDIR}/vcan.cfg"