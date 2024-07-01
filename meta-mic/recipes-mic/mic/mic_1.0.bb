DESCRIPTION = "MIC communication application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://mic_com.c \
	   file://mic_com.h"

S = "${WORKDIR}"

do_compile() {
	${CC} mic_com.c ${LDFLAGS} -o mic
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 mic ${D}${bindir}
}
