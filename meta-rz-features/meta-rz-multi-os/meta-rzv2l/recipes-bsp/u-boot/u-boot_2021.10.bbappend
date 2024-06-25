FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://0001-disable-reserved-area-check.patch \
    file://0002-cmd-Add-cm33-command.patch \
    file://0003-configs-smarc-rzv2l_defconfig-Enable-CM33-command.patch \
"
