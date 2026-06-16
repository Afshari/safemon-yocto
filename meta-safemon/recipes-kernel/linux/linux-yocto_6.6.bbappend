FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append:qemuarm64 = " file://vcan.cfg;type=kmeta;destsuffix=vcan"