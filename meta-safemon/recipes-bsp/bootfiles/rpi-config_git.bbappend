FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " file://dsi.txt"

do_deploy:append() {
    cat ${WORKDIR}/dsi.txt >> ${DEPLOYDIR}/bootfiles/config.txt
}
