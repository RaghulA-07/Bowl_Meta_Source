SUMMARY = "GStreamer VSPM filter plugin"
SECTION = "multimedia"
LICENSE = "LGPLv2"
DEPENDS = "gstreamer1.0 gstreamer1.0-plugins-base pkgconfig vspmif-user-module kernel-module-mmngr kernel-module-mmngrbuf mmngr-user-module mmngrbuf-user-module"
LIC_FILES_CHKSUM = "file://COPYING.LIB;md5=6762ed442b3822387a51c92d928ead0d"
inherit autotools pkgconfig

GST_PLUGIN_VSPMFILTER_URL = "git://github.com/renesas-rz/rzg_gstreamer_vspmfilter"

BRANCH_rzg2h = "rz_g2"
BRANCH_rzg2l = "rz_g2l"

SRC_URI = " \
    ${GST_PLUGIN_VSPMFILTER_URL};protocol=https;branch=${BRANCH} \
    file://0001-Update-correct-base-number-of-VTOP-ioctl.patch \
"

SRCREV_rzg2h = "b63d0bbbe61494c9dd7501875c1943e322f224e6"
SRCREV_rzg2l = "b4f58d1a824d3eddece4857122beba7144064ef2"

S = "${WORKDIR}/git"
PV = "1.16.3"

FILES_${PN} = " \
    ${libdir}/gstreamer-1.0/libgstvspmfilter.so \
"

FILES_${PN}-dev = "${libdir}/gstreamer-1.0/libgstvspmfilter.la"
FILES_${PN}-staticdev = "${libdir}/gstreamer-1.0/libgstvspmfilter.a"
FILES_${PN}-dbg = " \
    ${libdir}/gstreamer-1.0/.debug \
    ${prefix}/src"
