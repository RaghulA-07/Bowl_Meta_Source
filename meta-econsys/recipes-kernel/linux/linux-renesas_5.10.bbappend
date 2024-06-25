DESCRIPTION = "Econ camera support for RZ/V2L Evaluation Board Kit PMIC version"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}/:"

SRC_URI_append += "\
        file://ecam20_curz_dtb.patch \
"
