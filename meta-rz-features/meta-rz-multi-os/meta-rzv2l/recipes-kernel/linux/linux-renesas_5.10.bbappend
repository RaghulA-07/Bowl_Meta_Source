FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

# List of kernel patches
SRC_URI += " \
    file://0001-arm64-dts-renesas-rz-smarc-Add-uio-support.patch \
    file://0002-arm64-dts-renesas-rz-smarc-Disable-OSTM2.patch \
    file://0003-clk-renesas-rzv2l-Set-SCIF2-OSTM2-as-critical.patch \
"

# Kernel confguration update
SRC_URI += "file://uio.cfg"
