DESCRIPTION = "Radar communication application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://radar_com.c \
	   file://radar_com.h"

S = "${WORKDIR}"

do_compile() {
	${CC} radar_com.c ${LDFLAGS} -o radar
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 radar ${D}${bindir}
}
