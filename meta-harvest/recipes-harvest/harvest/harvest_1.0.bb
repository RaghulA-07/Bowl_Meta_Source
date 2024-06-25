DESCRIPTION = "Harvest main application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://harvest_main.c \
	   file://harvest_main.h"

S = "${WORKDIR}"

do_compile() {
	${CC} harvest_main.c ${LDFLAGS} -o harvestmain
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 harvestmain ${D}${bindir}
}
