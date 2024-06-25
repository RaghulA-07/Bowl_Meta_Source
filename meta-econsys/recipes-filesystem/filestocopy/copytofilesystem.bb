LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://gstreamer_cam_test.sh \
           file://econ.conf \
          "

inherit allarch

do_install() {
install -d ${D}/home/root/
install -d ${D}/etc/modules-load.d/
install -m 0766 ${WORKDIR}/gstreamer_cam_test.sh ${D}/home/root/
install -m 0766 ${WORKDIR}/econ.conf ${D}/etc/modules-load.d/
}

FILES_${PN}="/*"
