DESCRIPTION = "Video stream main application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://video_main.c \
	   file://video_main.h"

S = "${WORKDIR}"

do_compile() {
	${CC} video_main.c ${LDFLAGS} -o videomain
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 videomain ${D}${bindir}
}
