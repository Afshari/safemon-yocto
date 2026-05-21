DESCRIPTION = "Linux kernel for Raspberry Pi 4 - 6.12 with PREEMPT_RT"
SECTION = "kernel"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

inherit kernel

LINUX_VERSION = "6.12"
PV = "${LINUX_VERSION}+git"

SRCREV = "998e64044d4eae1897eb6011c202d5385db8d3d3"

SRC_URI = " \
    git://github.com/raspberrypi/linux.git;branch=rpi-6.12.y;protocol=https \
    file://preempt_rt.cfg;subdir=fragments \
"

KERNEL_CONFIG_FRAGMENTS = "${WORKDIR}/fragments/preempt_rt.cfg"

S = "${WORKDIR}/git"

COMPATIBLE_MACHINE = "^(raspberrypi4-64)$"

KERNEL_DEVICETREE = " \
    broadcom/bcm2711-rpi-4-b.dtb \
"

KCONFIG_MODE = "--alldefconfig"
KBUILD_DEFCONFIG = "bcm2711_defconfig"
